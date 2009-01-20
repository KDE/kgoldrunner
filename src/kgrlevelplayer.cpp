/****************************************************************************
 *    Copyright 2009  Ian Wadham <ianwau@gmail.com>                         *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

#include <KDebug>

#include "kgrcanvas.h"
#include "kgrlevelplayer.h"
#include "kgrrulebook.h"
#include "kgrlevelgrid.h"
#include "kgrrunner.h"

KGrLevelPlayer::KGrLevelPlayer (QObject      * parent,
                                KGrGameData  * theGameData,
                                KGrLevelData * theLevelData)
    :
    QObject     (parent),
    gameData    (theGameData),
    levelData   (theLevelData),
    grid        (new KGrLevelGrid (this, theLevelData)),
    hero        (0),
    nuggets     (0),
    pointer     (true),
    started     (false),
    targetI     (1),
    targetJ     (1),
    direction   (STAND)
{
}

KGrLevelPlayer::~KGrLevelPlayer()
{
    // TODO - Do we need this delete?
    // while (! enemies.isEmpty()) {
        // delete enemies.takeFirst();
    // }
}

void KGrLevelPlayer::init (KGrCanvas * view)
{
    // Set the rules of this game.
    switch (gameData->rules) {
    case TraditionalRules:
        rules = new KGrTraditionalRules (this);
        break;
    case KGoldrunnerRules:
        rules = new KGrKGoldrunnerRules (this);
        break;
    case ScavengerRules:
        rules = new KGrScavengerRules (this);
        break;
    }
    rules->printRules();

    // Connect to code that paints grid cells and start-positions of sprites.
    connect (this, SIGNAL (animation()), view, SLOT (animate()));
    connect (this, SIGNAL (paintCell (int, int, char, int)),
             view, SLOT   (paintCell (int, int, char, int)));
    connect (this, SIGNAL (setSpriteType (int, char, int, int)),
             view, SLOT   (setSpriteType (int, char, int, int)));

    // Show the layout of this level in the view (KGrCanvas).
    int wall = ConcreteWall;
    for (int j = wall ; j < levelData->height + wall; j++) {
        for (int i = wall; i < levelData->width + wall; i++) {
            char type = grid->cellType (i, j);

            // Hide false bricks.
            if (type == FBRICK) {
                type = BRICK;
            }

            // Count the gold in this level.
            if (type == NUGGET) {
                nuggets++;
            }

            // Either, create a hero and paint him on the background ...
            if (type == HERO) {
                emit paintCell (i, j, FREE, 0);

                if (hero == 0) {
                    targetI = i;
                    targetJ = j;
                    emit setSpriteType (0, HERO, i, j);
                    hero = new KGrHero (this, grid, i, j, rules);
                }
            }

            // Or, create an enemy and paint him on the background ...
            else if (type == ENEMY) {
                emit paintCell (i, j, FREE, 0);

                KGrEnemy * enemy;
                int id = enemies.count();
                emit setSpriteType (id + 1, ENEMY, i, j);
                enemy = new KGrEnemy (this, grid, i, j, id, rules);
                enemies.append (enemy);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type, 0);
            }
        }
    }

    // Connect the new hero and enemies (if any) to the animation code.
    connect (hero, SIGNAL (startAnimation (int, int, int, int,
                                           Direction, AnimationType)),
             view, SLOT   (startAnimation (int, int, int, int,
                                           Direction, AnimationType)));
    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy, SIGNAL (startAnimation (int, int, int, int,
                                                Direction, AnimationType)),
                 view,  SLOT   (startAnimation (int, int, int, int,
                                                Direction, AnimationType)));
    }
}

void KGrLevelPlayer::setTarget (int pointerI, int pointerJ)
{
    // Mouse or other pointer device controls the hero.
    if ((! started) && (pointerI == targetI) && (pointerJ == targetJ)) {
        return;
    }
    pointer = true;
    targetI = pointerI;
    targetJ = pointerJ;
    if (! started) {
        started = true;
    }
}

void KGrLevelPlayer::setDirection (Direction dirn)
{
    // Keystrokes or other actions control the hero.
    pointer = false;
    started = true;
    direction = dirn;
}

Direction KGrLevelPlayer::getDirection (int heroI, int heroJ)
{
    if (pointer) {
        // If using a pointer device, calculate the hero's next direction,
        // starting from last pointer position and hero's current position.

        int di = targetI - heroI;
        int dj = targetJ - heroJ;

        // kDebug() << "di" << di << "dj" << dj;

        if ((dj > 0) && (grid->heroMoves (heroI, heroJ) & dFlag [DOWN])) {
            // kDebug() << "Go down";
            direction = DOWN;
        }
        else if ((dj < 0) && (grid->heroMoves (heroI, heroJ) & dFlag [UP])) {
            // kDebug() << "Go up";
            direction = UP;
        }
        else if (di > 0) {
            // kDebug() << "Go right";
            direction = RIGHT;
        }
        else if (di < 0) {
            // kDebug() << "Go left";
            direction = LEFT;
        }
        else {		// Note: di is zero, but dj is not necessarily zero.
            // kDebug() << "Stand";
            direction = STAND;
        }
    }

    kDebug() << "Hero at" << heroI << heroJ << "mouse at" << targetI << targetJ << "Direction" << direction;
    return direction;
}

void KGrLevelPlayer::tick()
{
    if (started) {
        hero->run();
        emit animation();
    }
}

#include "kgrlevelplayer.moc"
