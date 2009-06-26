#include "kgrdebug.h"

/****************************************************************************
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                         *
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

#include <stdio.h>
#include <stdlib.h>

// Include kgrgame.h only to access flags KGrGame::bugFix and KGrGame::logging.
#include "kgrgame.h"

#include "kgrtimer.h"
#include "kgrcanvas.h"
#include "kgrlevelplayer.h"
#include "kgrrulebook.h"
#include "kgrlevelgrid.h"
#include "kgrrunner.h"

#include <KDebug>
#include <KMessageBox>	// TODO - Remove.
#include <KRandomSequence>

KGrLevelPlayer::KGrLevelPlayer (QObject * parent, KRandomSequence * pRandomGen)
    :
    QObject          (parent),
    game             (parent),
    randomGen        (pRandomGen),
    hero             (0),
    controlMode      (MOUSE),
    nuggets          (0),
    playState        (NotReady),
    recording        (0),
    playback         (false),
    targetI          (1),
    targetJ          (1),
    direction        (NO_DIRECTION),
    newDirection     (NO_DIRECTION),
    timer            (0),
    digCycleTime     (200),	// Milliseconds per dig-timing cycle (default).
    digCycleCount    (40),	// Cycles while hole is fully open (default).
    digOpeningCycles (5),	// Cycles for brick-opening animation.
    digClosingCycles (4),	// Cycles for brick-closing animation.
    digKillingTime   (2)	// Cycle at which enemy/hero gets killed.
{
    t.start(); // IDW

    dbgLevel = 0;
}

int KGrLevelPlayer::playerCount = 0;

KGrLevelPlayer::~KGrLevelPlayer()
{
    while (! dugBricks.isEmpty()) {
        delete dugBricks.takeFirst();
    }
    kDebug() << "LEVEL PLAYER BEING DELETED.";
    playerCount--;

// TODO - Remove this debugging code.
if (recording) {
    int ch = 0;
    for (int i = 0; i <= (recIndex + 1); i ++) {
        ch = (uchar)(recording->content.at(i));
        dbe1 "%03d ", ch);
        if (ch == 0)
            break;
    }
    dbe1 "\n%d bytes\n", recIndex + 1);
    int j = 0;
    while (j < recording->draws.size()) {
        ch = (uchar)(recording->draws.at(j));
        dbe1 "%03d ", ch);
        if (ch == 0)
            break;
        j++;
    }
    dbe1 "\n%d bytes\n", j);
}

}

void KGrLevelPlayer::init (KGrCanvas * view, const int mode,
                           KGrRecording * pRecording, const bool pPlayback,
                           const bool gameFrozen)
{
    // TODO - Remove?
    playerCount++;
    if (playerCount > 1) {
        KMessageBox::information (view,
                QString("ERROR: KGrLevelPlayer Count = %1").arg(playerCount),
                "KGrLevelPlayer");
    }

    recording = pRecording;
    playback  = pPlayback;

    // Create the internal model of the level-layout.
    grid            = new KGrLevelGrid (this, recording);

    controlMode     = mode;		// Set mouse/keyboard/laptop control.
    levelWidth      = recording->width;
    levelHeight     = recording->height;

    reappearIndex   = levelWidth;	// Initialise the enemy-rebirth code.
    reappearPos.fill (1, levelWidth);

    // Set the rules of this game.
    switch (recording->rules) {
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

    recIndex  = 0;
    recCount  = 0;
    randIndex = 0;
    T         = 0;

    view->setGoldEnemiesRule (rules->enemiesShowGold());

    // Determine the access for hero and enemies to and from each grid-cell.
    grid->calculateAccess    (rules->runThruHole());

    // Connect to code that paints grid cells and start-positions of sprites.
    connect (this, SIGNAL (paintCell (int, int, char, int)),
             view, SLOT   (paintCell (int, int, char, int)));
    connect (this, SIGNAL (makeSprite (char, int, int)),
             view, SLOT   (makeSprite (char, int, int)));

    // Connect to the mouse-positioning code in the graphics.
    connect (this, SIGNAL (getMousePos (int &, int &)),
             view, SLOT   (getMousePos (int &, int &)));
    connect (this, SIGNAL (setMousePos (const int, const int)),
             view, SLOT   (setMousePos (const int, const int)));

    // Show the layout of this level in the view (KGrCanvas).
    int wall = ConcreteWall;
    int enemyCount = 0;
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);

            // Hide false bricks.
            if (type == FBRICK) {
                type = BRICK;
            }

            // Count the gold in this level.
            if (type == NUGGET) {
                nuggets++;
            }

            // If the hero is here, leave the tile empty.
            if (type == HERO) {
                emit paintCell (i, j, FREE, 0);
            }

            // If an enemy is here, count him and leave the tile empty.
            else if (type == ENEMY) {
                enemyCount++;
                emit paintCell (i, j, FREE, 0);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type, 0);
            }
        }
    }

    // Set the timing rules, maybe based on the number of enemies.
    rules->setTiming (enemyCount);
    rules->getDigTimes (digCycleTime, digCycleCount);

    // Create the hero (always sprite 0), with the proper timing.
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);
            if (type == HERO) {
                if (hero == 0) {
                    targetI = i;
                    targetJ = j;
                    heroId  = emit makeSprite (HERO, i, j);
                    hero    = new KGrHero (this, grid, i, j, heroId, rules);
                    hero->setNuggets (nuggets);
                    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
                        emit setMousePos (targetI, targetJ);
                    }
                    grid->changeCellAt (i, j, FREE);	// Hero now a sprite.
                }
            }
        }
    }

    // Create the enemies (sprites 1-n), with the proper timing.
    for (int j = wall ; j < levelHeight + wall; j++) {
        for (int i = wall; i < levelWidth + wall; i++) {
            char type = grid->cellType (i, j);
            if (type == ENEMY) {
                KGrEnemy * enemy;
                int id = emit makeSprite (ENEMY, i, j);
                enemy = new KGrEnemy (this, grid, i, j, id, rules);
                enemies.append (enemy);
                grid->changeCellAt (i, j, FREE);	// Enemy now a sprite.
                grid->setEnemyOccupied (i, j, id);
            }
        }
    }

    // Connect the hero's and enemies' efforts to the graphics.
    connect (this, SIGNAL (gotGold (int, int, int, bool, bool)),
             view, SLOT   (gotGold (int, int, int, bool, bool)));

    // Connect mouse-clicks from KGrCanvas to digging slot.
    connect (view, SIGNAL (mouseClick (int)), SLOT (doDig (int)));

    // Connect the hero and enemies (if any) to the animation code.
    connect (hero, SIGNAL (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)),
             view, SLOT   (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)));
    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy, SIGNAL (startAnimation (int, bool, int, int, int,
                                                Direction, AnimationType)),
                 view,  SLOT   (startAnimation (int, bool, int, int, int,
                                                Direction, AnimationType)));
    }

    // Connect the scoring.
    connect (hero, SIGNAL (incScore (const int)),
             game, SLOT   (incScore (const int)));
    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy, SIGNAL (incScore (const int)),
                 game,  SLOT   (incScore (const int)));
    }

    // Connect the sounds.
    connect (hero, SIGNAL (soundSignal (const int, const bool)),
             game, SLOT   (playSound   (const int, const bool)));

    // Connect the level player to the animation code (for use with dug bricks).
    connect (this, SIGNAL (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)),
             view, SLOT   (startAnimation (int, bool, int, int, int,
                                           Direction, AnimationType)));
    connect (this, SIGNAL (deleteSprite (int)),
             view, SLOT   (deleteSprite (int)));

    // Connect the grid to the view, to show hidden ladders when the time comes.
    connect (grid, SIGNAL (showHiddenLadders (const QList<int> &, const int)),
             view, SLOT   (showHiddenLadders (const QList<int> &, const int)));

    // Connect and start the timer.  The tick() slot emits signal animation(),
    // so there is just one time-source for the model and the view.

    timer = new KGrTimer (this, TickTime);	// TickTime def in kgrglobals.h.
    if (gameFrozen) {
        timer->pause();				// Pause is ON as level starts.
    }

    connect (timer, SIGNAL (tick (bool, int)), this, SLOT (tick (bool, int)));
    connect (this,  SIGNAL (animation (bool)), view, SLOT (animate (bool)));

    if (! playback) {
        dbk << "Play is being RECORDED.";
        recordInitialWaitTime (NO_DIRECTION, 1500);
    }
    else {
        dbk << "Play is being REPRODUCED.";
    }
}

void KGrLevelPlayer::startDigging (Direction diggingDirection)
{
    int digI = 1;
    int digJ = 1;

    // We need the hero to decide if he CAN dig and if so, where.
    if (hero->dig (diggingDirection, digI, digJ)) {
        // The hero can dig as requested: the chosen brick is at (digI, digJ).
        grid->changeCellAt (digI, digJ, HOLE);

        // Start the brick-opening animation (non-repeating).
        int id = emit makeSprite (BRICK, digI, digJ);
        emit startAnimation (id, false, digI, digJ,
                        (digOpeningCycles * digCycleTime), STAND, OPEN_BRICK);

        DugBrick * thisBrick = new DugBrick;
        DugBrick   brick     = {id, digCycleTime, digI, digJ,
                    (digCycleCount + digOpeningCycles + digClosingCycles - 1),
                    t.elapsed()}; // IDW test
        (* thisBrick)        = brick;
        dugBricks.append (thisBrick);
        // kDebug() << "DIG" << thisBrick->id << thisBrick->countdown
                 // << "time" << (t.elapsed() - thisBrick->startTime);
    }
}

void KGrLevelPlayer::processDugBricks (const int scaledTime)
{
    DugBrick * dugBrick;
    QMutableListIterator<DugBrick *> iterator (dugBricks);

    while (iterator.hasNext()) {
        dugBrick = iterator.next();
        dugBrick->cycleTimeLeft -= scaledTime;
        if (dugBrick->cycleTimeLeft < scaledTime) {
            dugBrick->cycleTimeLeft += digCycleTime;
            if (--dugBrick->countdown == digClosingCycles) {
                // Start the brick-closing animation (non-repeating).
                // kDebug() << "Brick" << dugBrick->digI << dugBrick->digJ <<
                            // "count" << dugBrick->countdown;
                emit startAnimation (dugBrick->id, false,
                                     dugBrick->digI, dugBrick->digJ,
                                     (digClosingCycles * digCycleTime),
                                     STAND, CLOSE_BRICK);
            }
            if (dugBrick->countdown == digKillingTime) {
                // kDebug() << "Brick" << dugBrick->digI << dugBrick->digJ <<
                            // "count" << dugBrick->countdown;
                // Close the hole and maybe capture the hero or an enemy.
                grid->changeCellAt (dugBrick->digI, dugBrick->digJ, BRICK);
            }
            if (dugBrick->countdown <= 0) {
                // kDebug() << "DIG" << dugBrick->id << dugBrick->countdown
                         // << "time" << (t.elapsed() - dugBrick->startTime);
                // Dispose of the dug brick and remove it from the list.
                emit deleteSprite (dugBrick->id);
                delete dugBrick;
                iterator.remove();
            }
        }
    }
}

void KGrLevelPlayer::prepareToPlay()
{
    kDebug() << "Set mouse to:" << targetI << targetJ;
    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        emit setMousePos (targetI, targetJ);
    }
    playState = Ready;
}

void KGrLevelPlayer::pause (bool stop)
{
    if (stop) {
        timer->pause();
    }
    else {
        timer->resume();
    }
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
        // The pointer moved: fall into "case Playing:" and start playing.
        else if (! playback) {
            T = 0;
        }
        playState = Playing;
    case Playing:
        // The human player is playing now.
        if (! playback) {
        if ((pointerI == targetI) && (pointerJ == targetJ) &&
            (recCount < 255) && (recCount > 0)) { // IDW test.
            recCount++;
            recording->content [recIndex] = (uchar) recCount;
            dbe2 "T %04d recIndex %03d REC: codes %d %d %d - recCount++\n",
                 T, recIndex - 2, (uchar)(recording->content.at (recIndex-2)),
                                  (uchar)(recording->content.at (recIndex-1)),
                                  (uchar)(recording->content.at (recIndex)));
        }
        else {
            recIndex++;
            recCount = 1;
            recording->content [recIndex++]   = (uchar) pointerI;
            recording->content [recIndex++]   = (uchar) pointerJ;
            recording->content [recIndex]     = (uchar) recCount;
            recording->content [recIndex + 1] = (uchar) END_CODE;
            dbe2 "T %04d recIndex %03d REC: codes %d %d %d - NEW TARGET\n",
                 T, recIndex - 2, pointerI, pointerJ,
                                  (uchar)(recording->content.at (recIndex)));
        }
        }
        targetI = pointerI;
        targetJ = pointerJ;
        break;
    }
}

void KGrLevelPlayer::doDig (int button)
{
    // Click to end demo/playback mode.
    if (playback) {
        interruptPlayback();
        return;
    }

    // If not ready or game control is not by mouse, ignore mouse-clicks.
    if ((playState == NotReady) || (controlMode != MOUSE)) {
        return;
    }

    uchar recordByte = 0;
    playState = Playing;
    switch (button) {
    case Qt::LeftButton:
        recordByte = DIRECTION_CODE + DIG_LEFT;
        startDigging (DIG_LEFT);
        break;
    case Qt::RightButton:
        recordByte = DIRECTION_CODE + DIG_RIGHT;
        startDigging (DIG_RIGHT);
        break;
    default:
        break;
    }
    if (recordByte != 0) {
        // Record a digging action.
        recordDigAction (recordByte);
    }
}

void KGrLevelPlayer::setDirectionByKey (Direction dirn)
{
    // Keystrokes control the hero.  KGrGame should avoid calling this during
    // playback, but better to be safe ...
    if (playback || (playState == NotReady) || (controlMode == MOUSE)) {
        return;
    }

    if ((dirn == DIG_LEFT) || (dirn == DIG_RIGHT)) {
	// Control mode is KEYBOARD or LAPTOP (hybrid: pointer + dig-keys).
        if (playState == Ready) {
            playState = Playing;
            T = 0;
        }
        if (controlMode == KEYBOARD) {
            newDirection = STAND;	// Stop a keyboard move when digging.
        }
        startDigging (dirn);
        recordDigAction ((uchar) (DIRECTION_CODE + dirn));
    }
    else if (controlMode == KEYBOARD) {
        if (playState == Ready) {
            playState = Playing;
            T = 0;
        }
        if (dirn != direction) {
            // Start recording and acting on the new direction at the next tick.
            newDirection = dirn;
        }
    }
}

Direction KGrLevelPlayer::getDirection (int heroI, int heroJ)
{
    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        int index = (playback) ? recIndex : recIndex - 2;
        dbe2 "T %04d recIndex %03d hero at [%02d, %02d] aiming at [%02d, %02d]\n",
             T, index, heroI, heroJ, targetI, targetJ);
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
    else if (controlMode == KEYBOARD) {
        dbe2 "T %04d recIndex %03d hero at [%02d, %02d] direction %d\n",
             T, recIndex - 1, heroI, heroJ, direction);
    }

    return direction;
}

void KGrLevelPlayer::recordInitialWaitTime (const Direction dirn, const int ms)
{
    // Allow a pause for viewing when playback starts.
    recCount = ms / TickTime;			// Convert milliseconds-->ticks.
    if (controlMode == KEYBOARD) {
        // int code = DIRECTION_CODE + dirn;
        recording->content [recIndex++]   = (uchar) (DIRECTION_CODE + dirn);
        recording->content [recIndex]     = (uchar) recCount;
        recording->content [recIndex + 1] = (uchar) END_CODE;
    }
    else {
        recording->content [recIndex++]   = (uchar) targetI;
        recording->content [recIndex++]   = (uchar) targetJ;
        recording->content [recIndex]     = (uchar) recCount;
        recording->content [recIndex + 1] = (uchar) END_CODE;
    }
}

void KGrLevelPlayer::recordDigAction (const uchar code)
{
    if (recIndex >= 3) {
        // If not the hero's first move, interrupt any previous pointer-move.
        if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
            recording->content [recIndex] =
                recording->content [recIndex] - 1;
            dbe2 "T %04d recIndex %03d REC: codes %d %d %d - adjust for dig\n",
                 T, recIndex - 2,
                (uchar)(recording->content.at (recIndex-2)),
                (uchar)(recording->content.at (recIndex-1)),
                (uchar)(recording->content.at (recIndex)));
        }
    }

    // Record the digging action.
    recIndex++;
    recording->content [recIndex]   = code;
    recording->content [recIndex + 1] = END_CODE;
    dbe2 "T %04d recIndex %03d REC: dig code %d\n", T, recIndex, code);

    if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        // Continue recording the previous pointer-move.
        recIndex++;
        recCount = 1;
        recording->content [recIndex++]   = targetI;
        recording->content [recIndex++]   = targetJ;
        recording->content [recIndex]     = (uchar) recCount;
        recording->content [recIndex + 1] = END_CODE;
        dbe2 "T %04d recIndex %03d REC: codes %d %d %d - after dig\n",
             T, recIndex - 2,
            (uchar)(recording->content.at (recIndex-2)),
            (uchar)(recording->content.at (recIndex-1)),
            (uchar)(recording->content.at (recIndex)));
    }
    else if ((controlMode == KEYBOARD) && (direction == STAND)) {
        // If already standing, continue standing after next tick.
        newDirection = STAND;
        direction    = NO_DIRECTION;
        dbe2 "T %04d recIndex %03d REC: continue standing\n", T, recIndex);
    }
}

void KGrLevelPlayer::recordKeystrokes()
{
    if (newDirection != direction) {
        direction = newDirection;

        // Record a change in direction.
        int code = DIRECTION_CODE + direction;
        recIndex++;
        recCount = 1;
        recording->content [recIndex++]   = (uchar) code;
        recording->content [recIndex]     = (uchar) recCount;
        recording->content [recIndex + 1] = (uchar) END_CODE;
        dbe2 "T %04d recIndex %03d REC: codes %d %d %d - NEW DIRECTION\n",
             T, recIndex - 1, direction, code,
                              (uchar)(recording->content.at (recIndex)));
    }
    else if (recIndex < 1) {
        // No keystroke recorded yet.
        return;
    }
    else if (direction == NO_DIRECTION) {
        // Initial wait-time is fixed: do not extend it.
        return;
    }
    else if ((recCount < 255) && (recCount > 0)) { // IDW test
        recCount++;
        recording->content [recIndex] = (uchar) recCount;
        dbe2 "T %04d recIndex %03d REC: codes   %d %d - recCount++\n",
             T, recIndex - 1, (uchar)(recording->content.at (recIndex-1)),
                              (uchar)(recording->content.at (recIndex)));
    }
    else {
        int code = DIRECTION_CODE + direction;
        recIndex++;
        recCount = 1;
        recording->content [recIndex++]   = (uchar) code;
        recording->content [recIndex]     = (uchar) recCount;
        recording->content [recIndex + 1] = (uchar) END_CODE;
        dbe2 "T %04d recIndex %03d REC: codes   %d %d - continued\n",
             T, recIndex - 1, (uchar)(recording->content.at (recIndex-1)),
                              (uchar)(recording->content.at (recIndex)));
    }
}

Direction KGrLevelPlayer::getEnemyDirection (int  enemyI, int enemyJ,
                                             bool leftRightSearch)
{
    int heroX, heroY, pointsPerCell;

    pointsPerCell = hero->whereAreYou (heroX, heroY);
    return rules->findBestWay (enemyI, enemyJ,
                               heroX / pointsPerCell, heroY / pointsPerCell,
                               grid, leftRightSearch);
}

bool KGrLevelPlayer::heroCaught (const int heroX, const int heroY)
{
    if (enemies.count() == 0) {
        return false;
    }
    int enemyX, enemyY, pointsPerCell_1;
    foreach (KGrEnemy * enemy, enemies) {
        pointsPerCell_1 = enemy->whereAreYou (enemyX, enemyY) - 1;
        if (((heroX < enemyX) ? ((heroX + pointsPerCell_1) >= enemyX) :
                                 (heroX <= (enemyX + pointsPerCell_1))) &&
            ((heroY < enemyY) ? ((heroY + pointsPerCell_1) > enemyY) :
                                 (heroY <= (enemyY + pointsPerCell_1)))) {
            dbk << "Caught by";
            enemy->showState();
            return true;
        }
    }
    return false;
}

KGrEnemy * KGrLevelPlayer::standOnEnemy (const int spriteId,
                                         const int x, const int y)
{
    int minEnemies = (spriteId == heroId) ? 1 : 2;
    if (enemies.count() < minEnemies) {
        return 0;
    }
    int enemyX, enemyY, pointsPerCell;
    foreach (KGrEnemy * enemy, enemies) {
        pointsPerCell = enemy->whereAreYou (enemyX, enemyY);
        if (((enemyY == (y + pointsPerCell)) ||
             (enemyY == (y + pointsPerCell - 1))) &&
            (enemyX > (x - pointsPerCell)) &&
            (enemyX < (x + pointsPerCell))) {
            return enemy;
        }
    }
    return 0;
}

bool KGrLevelPlayer::bumpingFriend (const int spriteId, const Direction dirn,
                                    const int gridI,  const int gridJ)
{
    int dI = 0;
    int dJ = 0;
    switch (dirn) {
    case LEFT:
         dI = -1;
         break;
    case RIGHT:
         dI = +1;
         break;
    case UP:
         dJ = -1;
         break;
    case DOWN:
         dJ = +1;
         break;
    default:
         break;
    }

    int otherEnemy;
    if (dI != 0) {
        otherEnemy = grid->enemyOccupied (gridI + dI, gridJ);
        if (otherEnemy > 0) {
            dbk3 << otherEnemy << "at" << (gridI + dI) << gridJ
                     << "dirn" << ((otherEnemy > 0) ?
                               (enemies.at (otherEnemy - 1)->direction()) : 0)
                     << "me" << spriteId << "dirn" << dirn;
            if (enemies.at (otherEnemy - 1)->direction() != dirn) {
                dbk3 << spriteId << "wants" << dirn << ":" << otherEnemy
                         << "at" << (gridI + dI) << gridJ << "wants"
                         << (enemies.at (otherEnemy - 1)->direction());
                return true;
            }
        }
    }
    if (dJ != 0) {
        otherEnemy = grid->enemyOccupied (gridI, gridJ + dJ);
        if (otherEnemy > 0) {
            dbk3 << otherEnemy << "at" << gridI << (gridJ + dJ)
                     << "dirn" << ((otherEnemy > 0) ?
                               (enemies.at (otherEnemy - 1)->direction()) : 0)
                     << "me" << spriteId << "dirn" << dirn;
            if (enemies.at (otherEnemy - 1)->direction() != dirn) {
                dbk3 << spriteId << "wants" << dirn << ":" << otherEnemy
                         << "at" << gridI << (gridJ + dJ) << "wants"
                         << (enemies.at (otherEnemy - 1)->direction());
                return true;
            }
        }
    }
    return false;
}

void KGrLevelPlayer::unstackEnemy (const int spriteId,
                                   const int gridI, const int gridJ,
                                   const int prevEnemy)
{
    dbe2 "KGrLevelPlayer::unstackEnemy (%02d at [%02d,%02d] prevEnemy %02d)\n",
        spriteId, gridI, gridJ, prevEnemy);
    int nextId = grid->enemyOccupied (gridI, gridJ);
    int prevId;
    while (nextId > 0) {
        prevId = enemies.at (nextId - 1)->getPrevInCell();
        dbe2 "Next %02d prev %02d\n", nextId, prevId);
        if (prevId == spriteId) {
            dbe2 "    SET IDs - id %02d prev %02d\n", nextId, prevEnemy);
            enemies.at (nextId - 1)->setPrevInCell (prevEnemy);
            // break;
        }
        nextId = prevId;
    }
}

void KGrLevelPlayer::tick (bool missed, int scaledTime)
{
    if (playback) {			// Replay a recorded move.
        if (! doRecordedMove()) {
            playback = false;
            // TODO - Should we emit interruptDemo() in UNEXPECTED_END case?
            dbk << "Unexpected END_OF_RECORDING - or KILL_HERO ACTION.";
            return;			// End of recording.
        }
    }
    else if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        int i, j;			// Make and record a live pointer-move.
        emit getMousePos (i, j);
        setTarget (i, j);
    }
    else if (controlMode == KEYBOARD) {
        recordKeystrokes();
    }

    if (playState != Playing) {
        return;
    }
    T++;

    if (dugBricks.count() > 0) {
        processDugBricks (scaledTime);
    }

    HeroStatus status = hero->run (scaledTime);
    if ((status == WON_LEVEL) || (status == DEAD)) {
        // Unsolicited timer-pause halts animation immediately, regardless of
        // user-selected state. It's OK: KGrGame deletes KGrLevelPlayer v. soon.
        timer->pause();

        // Queued connection ensures KGrGame slot runs AFTER return from here.
        emit endLevel (status);
        kDebug() << "END OF LEVEL";
        return;
    }

    foreach (KGrEnemy * enemy, enemies) {
        enemy->run (scaledTime);
    }

    emit animation (missed);
}

int KGrLevelPlayer::runnerGotGold (const int  spriteId,
                                   const int  i, const int j,
                                   const bool hasGold, const bool lost)
{
    if (hasGold) {
        dbk2 << "GOLD COLLECTED BY" << spriteId << "AT" << i << j;
    }
    else if (lost) {
        dbk2 << "GOLD LOST BY" << spriteId << "AT" << i << j;
    }
    else {
        dbk2 << "GOLD DROPPED BY" << spriteId << "AT" << i << j;
    }
    if (! lost) {
        grid->gotGold (i, j, hasGold);		// Record pickup/drop on grid.
    }
    emit gotGold (spriteId, i, j, hasGold, lost); // Erase/show gold on screen.

    // If hero got gold, score, maybe show hidden ladders, maybe end the level.
    if ((spriteId == heroId) || lost) {
        if (--nuggets <= 0) {
            grid->placeHiddenLadders();		// All gold picked up or lost.
        }
    }
    if (lost) {
        hero->setNuggets (nuggets);		// Update hero re lost gold.
    }
    return nuggets;
}

void KGrLevelPlayer::makeReappearanceSequence()
{
    // The idea is to make each possible x co-ord come up once per levelWidth
    // reappearances of enemies.  This is not truly random, but it reduces the
    // tedium in levels where you must keep killing enemies until a particular
    // x or range of x comes up (e.g. if they have to collect gold for you).

    // First put the positions in ascending sequence.
    for (int k = 0; k < levelWidth; k++) {
        reappearPos [k] = k + 1;
    }

    int z;
    int left = levelWidth;
    int temp;

    // Shuffle the co-ordinates of reappearance positions (1 to levelWidth).
    for (int k = 0; k < levelWidth; k++) {
        // Pick a random element from those that are left.
        z = (int) (randomByte ((uchar) left));
        // Exchange its value with the last of the ones left.
        temp = reappearPos [z];
        reappearPos [z] = reappearPos [left - 1];
        reappearPos [left - 1] = temp;
        left--;
    }
    dbk2 << "Randoms" << reappearPos;
    reappearIndex = 0;
}

void KGrLevelPlayer::enemyReappear (int & gridI, int & gridJ)
{
    bool looking = true;
    int  i, j, k;

    // Follow Traditional or Scavenger rules: enemies reappear at top.
    j = rules->reappearRow();

    // Randomly look for a free spot in the row.  Limit the number of tries.
    for (k = 1; ((k <= 3) && looking); k++) {
        if (reappearIndex >= levelWidth) {
            makeReappearanceSequence();	// Get next array of random i.
        }
        i = reappearPos [reappearIndex++];
        switch (grid->cellType (i, j)) {
        case FREE:
        case HLADDER:
            looking = false;
            break;
        default:
            break;
        }
    }

    // If unsuccessful, choose the first free spot in the rows below.
    while ((j < levelHeight) && looking) {
        j++;
        i = 0;
        while ((i < levelWidth) && looking) {
            i++;
            switch (grid->cellType (i, j)) {
            case FREE:
            case HLADDER:
                looking = false;
                break;
            default:
                break;
            }
        }
    }
    dbk2 << "Reappear at" << i << j;
    gridI = i;
    gridJ = j;
}

uchar KGrLevelPlayer::randomByte (const uchar limit)
{
    if (! playback) {
        uchar value = randomGen->getLong ((unsigned long) limit);
        // A zero-byte terminates recording->draws, so add 1 when recording ...
        dbe2 "Draw %03d, index %04d, limit %02d\n", value, randIndex, limit);
        recording->draws [randIndex++] = value + 1;
        return value;
    }
    else {
        dbe2 "Draw %03d, index %04d, limit %02d\n",
             (recording->draws.at (randIndex) - 1), randIndex, limit);
        // and subtract 1 when replaying.
        return ((uchar) recording->draws.at (randIndex++) - 1);
    }
}

bool KGrLevelPlayer::doRecordedMove()
{
    int i, j;
    uchar code = recording->content [recIndex];
    while (true) {
        // Check for end of recording.
        if ((code == END_CODE) || (code == 0)) {
            dbe2 "T %04d recIndex %03d PLAY - END of recording\n",
                 T, recIndex);
            emit endLevel (UNEXPECTED_END);
            return false;
        }

        // Simulate recorded mouse movement.
        if (code < DIRECTION_CODE) {
            // playState = Playing;
            if (recCount <= 0) {
                i = code;
                j = (uchar)(recording->content [recIndex + 1]);
                // targetI = code;
                // targetJ = (uchar)(recording->content [recIndex + 1]);
                recCount = (uchar)(recording->content [recIndex + 2]);
                dbe2 "T %04d recIndex %03d PLAY codes %d %d %d - NEW TARGET\n",
                     T, recIndex, i, j, recCount);
                     // T, recIndex, targetI, targetJ, recCount);

                setTarget (i, j);
            }
            else {
                dbe2 "T %04d recIndex %03d PLAY codes %d %d %d\n",
                     T, recIndex, targetI, targetJ, recCount);
            }
            if (--recCount <= 0) {
                recIndex = recIndex + 3;
                dbe2 "T %04d recIndex %03d PLAY - next index\n",
                     T, recIndex);
            }
            break;
        }

        // Simulate a key press or mouse button click.
        else if (code < MODE_CODE) {
            code = code - DIRECTION_CODE;
            if (code != direction) {
                playState = Playing;
            }
            if ((code == DIG_LEFT) || (code == DIG_RIGHT)) {
                dbe2 "T %04d recIndex %03d PLAY dig code %d\n",
                     T, recIndex, code);
                startDigging ((Direction) (code));
                recIndex++;
                code = recording->content [recIndex];
                recCount = 0;
                continue;
            }
            else {
                if (recCount <= 0) {
                    recCount = (uchar)(recording->content [recIndex + 1]);
                    dbe2 "T %04d recIndex %03d PLAY codes %d %d - KEY PRESS\n",
                         T, recIndex, code, recCount);
                    direction = ((Direction) (code));
                }
                else {
                    dbe2 "T %04d recIndex %03d PLAY codes %d %d mode %d\n",
                         T, recIndex, code, recCount, controlMode);
                }
                if (--recCount <= 0) {
                    recIndex = recIndex + 2;
                    dbe2 "T %04d recIndex %03d PLAY - next index\n",
                         T, recIndex);
                }
            }
            break;
        }

        // Replay a change of control-mode.
        else if (code < ACTION_CODE) {
            dbe2 "T %04d recIndex %03d PLAY control-mode code %d\n",
                 T, recIndex, code);
            setControlMode (code - MODE_CODE);
            recIndex++;
            code = recording->content [recIndex];
            recCount = 0;
            continue;
        }

        // Replay an action, such as KILL_HERO.
        else if (code < SPEED_CODE) {
            if (code == (ACTION_CODE + KILL_HERO)) {
                dbe2 "T %04d recIndex %03d PLAY kill-hero code %d\n",
                     T, recIndex, code);
                emit endLevel (DEAD);
                return false;
            }
        }

        // Replay a change of speed.
        else {
            dbe2 "T %04d recIndex %03d PLAY speed-change code %d\n",
                 T, recIndex, code);
            setTimeScale (code - SPEED_CODE);
            recIndex++;
            code = recording->content [recIndex];
            recCount = 0;
            continue;
        }
    }
    return true;
}

void KGrLevelPlayer::interruptPlayback()
{
    // Check if still replaying the wait-time before the first move.
    if (playState != Playing) {
        return;
    }

    uchar code = recording->content [recIndex];
    // Check for end-of-recording already reached.
    if ((code == END_CODE) || (code == 0)) {
        return;
    }

// Start debug stuff.
    dbk2 << "recIndex" << recIndex << "recCount" << recCount
        << "randIndex" << randIndex;
    int ch = 0;
    int i  = 0;
    while (i < recording->content.size()) {
        ch = (uchar)(recording->content.at(i));
        dbe2 "%03d ", ch);
        if (ch == 0)
            break;
        i++;
    }
    dbe2 "\n%d bytes\n", i - 1);
    i  = 0;
    while (i < recording->draws.size()) {
        ch = (uchar)(recording->draws.at(i));
        dbe2 "%03d ", ch);
        if (ch == 0)
            break;
        i++;
    }
    dbe2 "\n%d bytes\n", i - 1);
// End debug stuff.

    if (recCount > 0) {
        if ((code >= DIRECTION_CODE) && (code < (DIRECTION_CODE + nDirections)))
        {
            // Set keyboard counter to show how many ticks have been replayed.
            recCount = (uchar)(recording->content [recIndex + 1]) - recCount;
            recording->content [recIndex + 1] = (uchar) (recCount);
            recIndex = recIndex + 1;	// Count here if same key after pause.
        }
        else if (code < DIRECTION_CODE) {
            // Set mouse-move counter to show how many ticks have been replayed.
            recCount = (uchar)(recording->content [recIndex + 2]) - recCount;
            recording->content [recIndex + 2] = (uchar) (recCount);
            recIndex = recIndex + 2;	// Count here if mouse in same position.
        }
    }

    recording->content [recIndex + 1] = END_CODE;
    for (int i = (recIndex + 2); i < recording->content.size(); i++) {
        recording->content [i] = 0;
    }
    for (int i = randIndex; i < recording->draws.size(); i++) {
        recording->draws [i] = 0;
    }

// Start debug stuff.
    dbk2 << "recIndex" << recIndex << "recCount" << recCount
        << "randIndex" << randIndex;
    i  = 0;
    while (i < recording->content.size()) {
        ch = (uchar)(recording->content.at(i));
        dbe2 "%03d ", ch);
        if (ch == 0)
            break;
        i++;
    }
    dbe2 "\n%d bytes\n", i - 1);
    i  = 0;
    while (i < recording->draws.size()) {
        ch = (uchar)(recording->draws.at(i));
        dbe2 "%03d ", ch);
        if (ch == 0)
            break;
        i++;
    }
    dbe2 "\n%d bytes\n", i - 1);
// End debug stuff.

    playback = false;
    emit interruptDemo();
    dbk << "INTERRUPT - emit interruptDemo();";
}

void KGrLevelPlayer::killHero()
{
    if (! playback) {
        // Record that KILL_HERO is how the level ended.
        int code = ACTION_CODE + KILL_HERO;
        recIndex++;
        recCount = 0;
        recording->content [recIndex]     = (uchar) code;
        recording->content [recIndex + 1] = (uchar) END_CODE;
        dbe2 "T %04d recIndex %03d REC: code %d - KILL HERO\n",
             T, recIndex, code);

        emit endLevel (DEAD);
        kDebug() << "END OF LEVEL";
    }
}

void KGrLevelPlayer::setControlMode  (const int mode)
{
    controlMode = mode;

    if (! playback) {
        // Record the change of mode.
        int code = MODE_CODE + mode;
        recIndex++;
        recCount = 0;
        recording->content [recIndex]     = (uchar) code;
        recording->content [recIndex + 1] = (uchar) END_CODE;
        dbe2 "T %04d recIndex %03d REC: code %d - MODE CHANGE\n",
             T, recIndex, code);
    }
}

void KGrLevelPlayer::setTimeScale  (const int timeScale)
{
    timer->setScale ((float) (timeScale * 0.1));

    if (! playback) {
        // Record the change of speed.
        int code = SPEED_CODE + timeScale;
        recIndex++;
        recCount = 0;
        recording->content [recIndex]     = (uchar) code;
        recording->content [recIndex + 1] = (uchar) END_CODE;
        dbe2 "T %04d recIndex %03d REC: code %d - SPEED CHANGE\n",
             T, recIndex, code);
    }
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGrLevelPlayer::dbgControl (int code)
{
    switch (code) {
    case DO_STEP:
        timer->step();			// Do one timer step only.
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
        hero->showState();		// Show hero's co-ordinates and state.
        break;
    case S_OBJ:
        showObjectState();		// Show an object's state.
        break;
    default:
        showEnemyState (code - ENEMY_0); // Show enemy co-ords and state.
        break;
    }
}

void KGrLevelPlayer::bugFix()
{
    // Toggle a bug fix on/off dynamically.
    KGrGame::bugFix = (KGrGame::bugFix) ? false : true;
    fprintf (stderr, "%s", (KGrGame::bugFix) ? "\n" : "");
    fprintf (stderr, ">> Bug fix is %s\n", (KGrGame::bugFix) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::startLogging()
{
    // Toggle logging on/off dynamically.
    KGrGame::logging = (KGrGame::logging) ? false : true;
    fprintf (stderr, "%s", (KGrGame::logging) ? "\n" : "");
    fprintf (stderr, ">> Logging is %s\n", (KGrGame::logging) ? "ON" : "OFF\n");
}

void KGrLevelPlayer::showFigurePositions()
{
    hero->showState();
    foreach (KGrEnemy * enemy, enemies) {
        enemy->showState();
    }
}

void KGrLevelPlayer::showObjectState()
{
    int   i       = targetI;
    int   j       = targetJ;
    char  here    = grid->cellType (i, j);
    Flags access  = grid->heroMoves (i, j);
    int   enemyId = grid->enemyOccupied (i, j);

    int enter     = (access & ENTERABLE)         ? 1 : 0;
    int stand     = (access & dFlag [STAND])     ? 1 : 0;
    int u         = (access & dFlag [UP])        ? 1 : 0;
    int d         = (access & dFlag [DOWN])      ? 1 : 0;
    int l         = (access & dFlag [LEFT])      ? 1 : 0;
    int r         = (access & dFlag [RIGHT])     ? 1 : 0;
    fprintf (stderr,
             "[%02d,%02d] [%c] %02x E %d S %d U %d D %d L %d R %d occ %02d\n",
	     i, j, here, access, enter, stand, u, d, l, r, enemyId);

    Flags eAccess = grid->enemyMoves (i, j);
    if (eAccess != access) {
        access    = eAccess;
        enter     = (access & ENTERABLE)         ? 1 : 0;
        stand     = (access & dFlag [STAND])     ? 1 : 0;
        u         = (access & dFlag [UP])        ? 1 : 0;
        d         = (access & dFlag [DOWN])      ? 1 : 0;
        l         = (access & dFlag [LEFT])      ? 1 : 0;
        r         = (access & dFlag [RIGHT])     ? 1 : 0;
        fprintf (stderr,
             "[%02d,%02d] [%c] %02x E %d S %d U %d D %d L %d R %d Enemy\n",
	     i, j, here, access, enter, stand, u, d, l, r);
    }
}

void KGrLevelPlayer::showEnemyState (int enemyId)
{
    if (enemyId < enemies.count()) {
        enemies.at(enemyId)->showState();
    }
}


#include "kgrlevelplayer.moc"
