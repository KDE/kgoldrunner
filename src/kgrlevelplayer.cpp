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
    started     (false),
    lastI       (1),
    lastJ       (1)
{
}

KGrLevelPlayer::~KGrLevelPlayer()
{
    // while (! enemies.isEmpty()) {
        // delete enemies.takeFirst();
    // }
}

void KGrLevelPlayer::init ()
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

            // Either, create a hero and paint him on the background.
            if (type == HERO) {
                emit paintCell (i, j, FREE, 0);
                if (hero == 0) {
                    lastI = i;
                    lastJ = j;
                    emit setSpriteType (0, HERO);
                    hero = new KGrNewHero (grid, i, j, rules);
                }
            }

            // Or, create an enemy and paint him on the background.
            else if (type == ENEMY) {
                emit paintCell (i, j, FREE, 0);
                int id = enemies.count();
                emit setSpriteType (id + 1, ENEMY);
                enemy = new KGrNewEnemy (grid, i, j, id, rules);
                enemies.append (enemy);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type, 0);
            }
        }
    }
}

KGrNewHero * KGrLevelPlayer::getHero() const
{
    return hero;
}

QList<KGrNewEnemy *> KGrLevelPlayer::getEnemies() const
{
    return enemies;
}

void KGrLevelPlayer::setDirection (int i, int j)
{
    if ((i == lastI) && (j == lastJ)) {
        return;
    }
    started = true;
    lastI = i;
    lastJ = j;

    int heroI;
    int heroJ;
    hero->getLocation (heroI, heroJ);
    int di = lastI - heroI;
    int dj = lastJ - heroJ;
    Direction nextDir;

    if ((dj > 0) && (grid->heroMoves (heroI, heroJ) & DOWN)) {
        nextDir = DOWN;
    }
    else if ((dj < 0) && (grid->heroMoves (heroI, heroJ) & UP)) {
        nextDir = UP;
    }
    else if (di > 0) {
        nextDir = RIGHT;
    }
    else if (di < 0) {
        nextDir = LEFT;
    }
    else if (di == 0) {
        nextDir = STAND;
    }
    hero->run (nextDir);
}

#include "kgrlevelplayer.moc"
