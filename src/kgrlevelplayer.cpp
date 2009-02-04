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
    controlMode (MOUSE),
    nuggets     (0),
    playState   (NotReady),
    targetI     (1),
    targetJ     (1),
    direction   (STAND)
{
    gameLogging = false;
    bugFixed = false;
}

KGrLevelPlayer::~KGrLevelPlayer()
{
    // TODO - Do we need this delete?
    // while (! enemies.isEmpty()) {
        // delete enemies.takeFirst();
    // }
}

void KGrLevelPlayer::init (KGrCanvas * view, const Control mode)
{
    controlMode = mode;

    // TODO - Should not really remember the view: needed for setMousePos.
    mView = view;

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

    view->setGoldEnemiesRule (rules->enemiesShowGold());
    grid->calculateAccess    (rules->runThruHole());

    // Connect to code that paints grid cells and start-positions of sprites.
    connect (this, SIGNAL (animation()), view, SLOT (animate()));
    connect (this, SIGNAL (paintCell (int, int, char, int)),
             view, SLOT   (paintCell (int, int, char, int)));
    connect (this, SIGNAL (makeSprite (char, int, int)),
             view, SLOT   (makeSprite (char, int, int)));

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

// TODO - Do hero in pass 1 and enemies in pass 2, to ensure the hero has id 0.
            // Either, create a hero and paint him on the background ...
            if (type == HERO) {
                emit paintCell (i, j, FREE, 0);

                if (hero == 0) {
                    targetI = i;
                    targetJ = j;
                    heroID  = emit makeSprite (HERO, i, j);
                    hero    = new KGrHero (this, grid, i, j, heroID, rules);
                    // TODO - Iff mouse mode, setMousePos();
                    view->setMousePos (targetI, targetJ);	// ??????????
                }
            }

            // Or, create an enemy and paint him on the background ...
            else if (type == ENEMY) {
                emit paintCell (i, j, FREE, 0);

                KGrEnemy * enemy;
                int id = emit makeSprite (ENEMY, i, j);
                enemy = new KGrEnemy (this, grid, i, j, id, rules);
                enemies.append (enemy);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type, 0);
            }
        }
    }

    // Connect the hero's and ememies' efforts to the graphics.
    connect (this, SIGNAL (gotGold (int, int, int, bool)),
             view, SLOT   (gotGold (int, int, int, bool)));

    // Connect mouse-clicks from KGrCanvas to digging slot.
    // TODO - Think about levelPlayer managing all the dug bricks.
    // TODO - We need hero to decide where and when to dig.
    connect (view, SIGNAL (mouseClick (int)), SLOT (doDig (int)));

    // Let the hero create and delete sprites for animating dug bricks.
    connect (hero, SIGNAL (makeSprite (char, int, int)),
             view, SLOT   (makeSprite (char, int, int)));

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

void KGrLevelPlayer::startDigging (Direction diggingDirection) {
    int digI = 1;
    int digJ = 1;
    if (hero->dig (diggingDirection, digI, digJ)) {
        // The hero CAN dig as requested: the chosen brick is at (digI, digJ).
        grid->changeCellAt (digI, digJ, HOLE);
        emit paintCell (digI, digJ, FREE, 0);
    }
}

void KGrLevelPlayer::prepareToPlay()
{
    // TODO - Should this be a signal?
    kDebug() << "Set mouse to:" << targetI << targetJ;
    mView->setMousePos (targetI, targetJ);
    playState = Ready;
}

void KGrLevelPlayer::setTarget (int pointerI, int pointerJ)
{
    // Mouse or other pointer device (eg. laptop touchpad) controls the hero.
    switch (playState) {
    case NotReady:
        // Ignore the pointer until KGrLevelPlayer is ready to start.
        break;
    case Ready:
        // Wait until the human player is ready to start playing.
        if ((pointerI == targetI) && (pointerJ == targetJ)) {
            // The pointer is still over the hero: do not start playing yet.
            break;
        }
        else {
            // The pointer moved: fall into "case Playing:" and start playing.
            playState = Playing;
        }
    case Playing:
        // The human player is playing now.
        targetI = pointerI;
        targetJ = pointerJ;
        break;
    }
}

void KGrLevelPlayer::doDig (int button)
{
    // If not ready or game control is not by mouse, ignore mouse-clicks.
    if ((playState == NotReady) || (controlMode != MOUSE)) {
        return;
    }

    // TODO - Work this in with game-freezes.
    playState = Playing;
    switch (button) {
    case Qt::LeftButton:
        startDigging (DIG_LEFT);
        break;
    case Qt::RightButton:
        startDigging (DIG_RIGHT);
        break;
    default:
        break;
    }
}

void KGrLevelPlayer::setDirectionByKey (Direction dirn)
{
    // Keystrokes control the hero.
    if ((playState == NotReady) || (controlMode == MOUSE)) {
        return;
    }

    if ((dirn == DIG_LEFT) || (dirn == DIG_RIGHT)) {
	// Control mode is KEYBOARD or LAPTOP (hybrid: pointer + dig-keys).
        playState = Playing;
        direction = STAND;
        startDigging (dirn);
    }
    else if (controlMode == KEYBOARD) {
        playState = Playing;
        direction = dirn;
    }
}

Direction KGrLevelPlayer::getDirection (int heroI, int heroJ)
{
    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
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
    if (playState == Playing) {
        hero->run();
        emit animation();
    }
}

void KGrLevelPlayer::runnerGotGold (const int  spriteID,
                                    const int  i, const int j,
                                    const bool hasGold)
{
    grid->gotGold (i, j, hasGold);		// Record pickup/drop on grid.
    emit  gotGold (spriteID, i, j, hasGold);	// Erase/show gold on screen.

    // If hero got gold, score, maybe show hidden ladders, maybe end the level.
    if (spriteID == heroID) {
        kDebug() << "Collected gold";
        if (--nuggets <= 0) {
            kDebug() << "ALL GOLD COLLECTED";
        }
    }
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGrLevelPlayer::dbgControl (int code)
{
    switch (code) {
    case DO_STEP:
        tick();				// Do one timer step only.
        break;
    case BUG_FIX:
        bugFix();			// Turn a bug fix on/off dynamically.
        break;
    case LOGGING:
        startLogging();			// Turn logging on/off.
        break;
    case S_POSNS:
        showFigurePositions();		// Show everybody's co-ordinates.
        break;
    case S_HERO:
        hero->showState ('s');		// Show hero's co-ordinates and state.
        break;
    case S_OBJ:
        showObjectState();		// Show an object's state.
        break;
    default:
        showEnemyState (code - ENEMY_0); // Show enemy co-ords and state.
        break;
    }
}

// OBSOLESCENT - 21/1/09 Can do this just by calling tick().
void KGrLevelPlayer::restart()
{
    // bool temp;
    // int i,j;

    // if (editMode)		// Can't move figures when in Edit Mode.
        // return;

    // temp = gameFrozen;

    // gameFrozen = false;	// Temporarily restart the game, by re-running
                                // any timer events that have been blocked.

    // OBSOLESCENT - 7/1/09
    // readMousePos();		// Set hero's direction.
    // hero->doStep();		// Move the hero one step.

    // OBSOLESCENT - 7/1/09
    // j = enemies.count();	// Move each enemy one step.
    // for (i = 0; i < j; i++) {
        // enemy = enemies.at (i);	// Need to use an index because called methods
        // enemy->doStep();	// change the "current()" of the "enemies" list.
    // }

    // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
    // for (i = 1; i <= 28; i++)
        // for (j = 1; j <= 20; j++) {
            // if ((playfield[i][j]->whatIam() == HOLE) ||
                // (playfield[i][j]->whatIam() == USEDHOLE) ||
                // (playfield[i][j]->whatIam() == BRICK))
                // ((KGrBrick *)playfield[i][j])->doStep();
        // }

    // gameFrozen = temp;	// If frozen was true, halt again, which gives a
                                // single-step effect, otherwise go on running.
}

void KGrLevelPlayer::bugFix()
{
    // Toggle a bug fix on/off dynamically.
    bugFixed = (bugFixed) ? false : true;
    printf ("%s", (bugFixed) ? "\n" : "");
    printf (">>> Bug fix is %s\n", (bugFixed) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::startLogging()
{
    // Toggle logging on/off dynamically.
    gameLogging = (gameLogging) ? false : true;
    printf ("%s", (gameLogging) ? "\n" : "");
    printf (">>> Logging is %s\n", (gameLogging) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::showFigurePositions()
{
    hero->showState ('p');
    foreach (KGrEnemy * enemy, enemies) {
        enemy->showState ('p');
    }
}

void KGrLevelPlayer::showObjectState()
{
    int i, j;
    // OBSOLESCENT - 20/1/09 KGrObject * myObject;

    // p = view->getMousePos(); Need to get mouse position somehow. DONE.
    i = targetI;
    j = targetJ;
    // OBSOLESCENT - 20/1/09 Need to compile after kgrobject.cpp removed.
    // myObject = playfield[i][j];
    // switch (myObject->whatIam()) {
        // case BRICK:
        // case HOLE:
        // case USEDHOLE:
            // ((KGrBrick *)myObject)->showState (i, j); break;
        // default: myObject->showState (i, j); break;
    // }
}

void KGrLevelPlayer::showEnemyState (int enemyId)
{
    if (enemyId < enemies.count()) {
        enemies.at(enemyId)->showState ('s');
    }
}


#include "kgrlevelplayer.moc"
