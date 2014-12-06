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
#include "kgrscene.h"

#include "kgrtimer.h"
#include "kgrview.h"
#include "kgrlevelplayer.h"
#include "kgrrulebook.h"
#include "kgrlevelgrid.h"
#include "kgrrunner.h"

#include <QDebug>
#include <KMessageBox>	// TODO - Remove.
#include <KRandomSequence>

KGrLevelPlayer::KGrLevelPlayer (QObject * parent, KRandomSequence * pRandomGen)
    :
    QObject          (parent),
    game             (parent),
    randomGen        (pRandomGen),
    hero             (0),
    controlMode      (MOUSE),
    holdKeyOption    (CLICK_KEY),
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
    digKillingTime   (2),	// Cycle at which enemy/hero gets killed.
    dX               (0),	// X motion for KEYBOARD + HOLD_KEY option.
    dY               (0)	// Y motion for KEYBOARD + HOLD_KEY option.
{
    t.start(); // IDW

    dbgLevel = 0;
}

int KGrLevelPlayer::playerCount = 0;

KGrLevelPlayer::~KGrLevelPlayer()
{
    qDeleteAll(dugBricks);
    dugBricks.clear(); //TODO: necessary?
    //qDebug() << "LEVEL PLAYER BEING DELETED.";
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

void KGrLevelPlayer::init (KGrView * view,
                           KGrRecording * pRecording,
                           const bool pPlayback,
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

    // Set mouse/keyboard/laptop control and click/hold option for keyboard.
    controlMode     = recording->controlMode;
    holdKeyOption   = recording->keyOption;

    dX              = 0;		// X motion for KEYBOARD + HOLD_KEY.
    dY              = 0;		// Y motion for KEYBOARD + HOLD_KEY.
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

    view->gameScene()->setGoldEnemiesRule (rules->enemiesShowGold());

    // Determine the access for hero and enemies to and from each grid-cell.
    grid->calculateAccess    (rules->runThruHole());

    // Connect to code that paints grid cells and start-positions of sprites.
    connect (this,              SIGNAL  (paintCell(int,int,char)),
             view->gameScene(), SLOT    (paintCell(int,int,char)));

    connect (this,              SIGNAL  (makeSprite(char,int,int)),
             view->gameScene(), SLOT    (makeSprite(char,int,int)));

    // Connect to the mouse-positioning code in the graphics.
    connect (this, SIGNAL (getMousePos(int&,int&)),
             view->gameScene(), SLOT   (getMousePos(int&,int&)));

    connect (this, SIGNAL (setMousePos(int,int)),
             view->gameScene(), SLOT   (setMousePos(int,int)));

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
                emit paintCell (i, j, FREE);
            }

            // If an enemy is here, count him and leave the tile empty.
            else if (type == ENEMY) {
                enemyCount++;
                emit paintCell (i, j, FREE);
            }

            // Or, just paint this tile.
            else {
                emit paintCell (i, j, type);
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
    connect (this, SIGNAL (gotGold(int,int,int,bool,bool)),
             view->gameScene(), SLOT (gotGold(int,int,int,bool,bool)));

    // Connect mouse-clicks from KGrCanvas to digging slot.
    connect (view, SIGNAL (mouseClick(int)), SLOT (doDig(int)));

    // Connect the hero and enemies (if any) to the animation code.
    connect (hero,              SIGNAL  (startAnimation (int, bool, int, int,
                                         int, Direction, AnimationType)),
             view->gameScene(), SLOT    (startAnimation (int, bool, int, int,
                                         int, Direction, AnimationType)));

    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy,             SIGNAL (startAnimation (int, bool, int, int,
                                            int, Direction, AnimationType)),
                 view->gameScene(), SLOT   (startAnimation (int, bool, int, int,
                                            int, Direction, AnimationType)));
    }

    // Connect the scoring.
    connect (hero, SIGNAL (incScore(int)),
             game, SLOT   (incScore(int)));
    foreach (KGrEnemy * enemy, enemies) {
        connect (enemy, SIGNAL (incScore(int)),
                 game,  SLOT   (incScore(int)));
    }

    // Connect the sounds.
    connect (hero, SIGNAL (soundSignal(int,bool)),
             game, SLOT   (playSound(int,bool)));

    // Connect the level player to the animation code (for use with dug bricks).
    connect (this,              SIGNAL (startAnimation (int, bool, int, int,
                                        int, Direction, AnimationType)),
             view->gameScene(), SLOT   (startAnimation (int, bool, int, int,
                                        int, Direction, AnimationType)));

    connect (this,              SIGNAL (deleteSprite(int)),
             view->gameScene(), SLOT   (deleteSprite(int)));

    // Connect the grid to the view, to show hidden ladders when the time comes.
    connect (grid, SIGNAL (showHiddenLadders(QList<int>,int)),
             view->gameScene(), SLOT (showHiddenLadders(QList<int>,int)));

    // Connect and start the timer.  The tick() slot emits signal animation(),
    // so there is just one time-source for the model and the view.

    timer = new KGrTimer (this, TickTime);	// TickTime def in kgrglobals.h.
    if (gameFrozen) {
        timer->pause();				// Pause is ON as level starts.
    }

    connect (timer, SIGNAL (tick(bool,int)), this, SLOT (tick(bool,int)));
    connect (this,              SIGNAL  (animation(bool)),
             view->gameScene(), SLOT    (animate(bool)));

    if (! playback) {
        // Allow some time to view the level before starting a replay.
        recordInitialWaitTime (1500);		// 1500 msec or 1.5 sec.
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
                emit startAnimation (dugBrick->id, false,
                                     dugBrick->digI, dugBrick->digJ,
                                     (digClosingCycles * digCycleTime),
                                     STAND, CLOSE_BRICK);
            }
            if (dugBrick->countdown == digKillingTime) {
                // Close the hole and maybe capture the hero or an enemy.
                grid->changeCellAt (dugBrick->digI, dugBrick->digJ, BRICK);
            }
            if (dugBrick->countdown <= 0) {
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
        else if (! playback) { // TODO - Remove debugging code (3 lines).
            T = 0;
        }
        playState = Playing;
    case Playing:
        // The human player is playing now.
        if (! playback) {
            record (3, pointerI, pointerJ);
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
        // Record the digging action.
        record (1, recordByte);
    }
}

void KGrLevelPlayer::setDirectionByKey (const Direction dirn,
                                        const bool pressed)
                     
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
// IDW What happens here if keyboard option is HOLD_KEY?  What *should* happen?
            newDirection = STAND;	// Stop a keyboard move when digging.
        }
        startDigging (dirn);
        record (1, (uchar) (DIRECTION_CODE + dirn));
    }
    else if (controlMode == KEYBOARD) {
        if (playState == Ready) {
            playState = Playing;
            T = 0;
        }
        // Start recording and acting on the new direction at the next tick.
        if ((holdKeyOption == CLICK_KEY) && pressed && (dirn != direction)) {
            newDirection = dirn;
        }
        else if (holdKeyOption == HOLD_KEY) {
            int sign = pressed ? +1 : -1;
            dX = dX + sign * movement [dirn][X];
            dY = dY + sign * movement [dirn][Y];
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

        direction = setDirectionByDelta (targetI - heroI, targetJ - heroJ,
                                         heroI, heroJ);
    }
    else if ((controlMode == KEYBOARD) && (holdKeyOption == HOLD_KEY)) {

        // If using the hold/release key option, resolve diagonal directions
        // (if there are multi-key holds) into either horizontal or vertical.

        direction = setDirectionByDelta (dX, dY, heroI, heroJ);
        dbe2 "T %04d recIndex %03d delta [%02d, %02d] "
             "hero at [%02d, %02d] direction %d\n",
             T, recIndex - 1, dX, dY, heroI, heroJ, direction);
    }

    return direction;
}

Direction KGrLevelPlayer::setDirectionByDelta (const int di, const int dj,
                                               const int heroI, const int heroJ)
{
    Direction dirn = STAND;

    if ((dj > 0) && (grid->heroMoves (heroI, heroJ) & dFlag [DOWN])) {
        dirn = DOWN;
    }
    else if ((dj < 0) && (grid->heroMoves (heroI, heroJ) & dFlag [UP])) {
        dirn = UP;
    }
    else if (di > 0) {
        dirn = RIGHT;
    }
    else if (di < 0) {
        dirn = LEFT;
    }
    else {		// Note: di is zero, but dj is not necessarily zero.
        dirn = STAND;
    }
    return dirn;
}

void KGrLevelPlayer::recordInitialWaitTime (const int ms)
{
    // Allow a pause for viewing when playback starts.
    recCount = ms / TickTime;			// Convert milliseconds-->ticks.
    if (controlMode == KEYBOARD) {
        recording->content [recIndex++]   = (uchar) (DIRECTION_CODE +
                                                     NO_DIRECTION);
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

void KGrLevelPlayer::record (const int bytes, const int n1, const int n2)
{
    if (playback) {
        return;
    }

    // Check for repetition of a previous direction-key or pointer posn. (I, J).
    if (recIndex > 2)
    dbe3 "recCount %d bytes %d n1 %d n2 %d [recIndex-1] %d [recIndex-2] %d\n",
        recCount, bytes, n1, n2, (uchar) recording->content.at (recIndex - 1),
        (uchar) recording->content.at (recIndex - 2));
    if ((recCount > 0) && (bytes > 1) && (recCount < (END_CODE - 1)) &&
        (((bytes == 2) && (n1 == (uchar) recording->content [recIndex - 1])) ||
         ((bytes == 3) && (n1 == (uchar) recording->content [recIndex - 2]) &&
                          (n2 == (uchar) recording->content [recIndex - 1]))
        )) {
        // Count repetitions, up to a maximum of (END_CODE - 1) = 254.
        recording->content [recIndex]       = (uchar) (++recCount);
        if (bytes == 2) {
            dbe2 "T %04d recIndex %03d REC: codes --- %3d %3d - recCount++\n",
                 T, recIndex - 1, (uchar)(recording->content.at (recIndex-1)),
                                  (uchar)(recording->content.at (recIndex)));
        }
        else if (bytes == 3) {
            dbe2 "T %04d recIndex %03d REC: codes %3d %3d %3d - recCount++\n",
                 T, recIndex - 2, (uchar)(recording->content.at (recIndex-2)),
                                  (uchar)(recording->content.at (recIndex-1)),
                                  (uchar)(recording->content.at (recIndex)));
        }
        return;
    }

    // Record a single code or the first byte of a new doublet or triplet.
    recCount = 0;
    recording->content [++recIndex]         = (uchar) n1;

    if (bytes == 3) {
        // Record another byte for a triplet (i.e. the pointer's J position).
        recording->content [++recIndex] = (uchar) n2;
    }

    if (bytes > 1) {
        // Record a repetition-count of 1 for a new doublet or triplet.
        recCount = 1;
        recording->content [++recIndex]     = (uchar) recCount;
    }

    switch (bytes) {
    case 1:
        dbe2 "T %04d recIndex %03d REC: singleton %3d %x\n",
             T, recIndex, n1, n1);
        break;
    case 2:
        dbe2 "T %04d recIndex %03d REC: codes %3d %3d %3d - NEW DIRECTION\n",
             T, recIndex - 1, direction,
                              (uchar)(recording->content.at (recIndex-1)),
                              (uchar)(recording->content.at (recIndex)));
        break;
    case 3:
        dbe2 "T %04d recIndex %03d REC: codes %3d %3d %3d - NEW TARGET\n",
             T, recIndex - 2, (uchar)(recording->content.at (recIndex-2)),
                              (uchar)(recording->content.at (recIndex-1)),
                              (uchar)(recording->content.at (recIndex)));
        break;
    default:
        break;
    }

    // Add the end-of-recording code (= 255).
    recording->content [recIndex + 1] = (uchar) END_CODE;
    return;
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
    int i, j;
    emit getMousePos (i, j);
    if (i == -2) {
        return;         // The KGoldRunner window is inactive.
    }
    if ((i == -1) && (playback || (controlMode != KEYBOARD))) {
        return;		// The pointer is outside the level layout.
    }

    if (playback) {			// Replay a recorded move.
        if (! doRecordedMove()) {
            playback = false;
            // TODO - Should we emit interruptDemo() in UNEXPECTED_END case?
            dbk << "Unexpected END_OF_RECORDING - or KILL_HERO ACTION.";
            return;			// End of recording.
        }
    }
    else if ((controlMode == MOUSE) || (controlMode == LAPTOP)) {
        setTarget (i, j);		// Make and record a live pointer-move.
    }
    else if (controlMode == KEYBOARD) {
        if (holdKeyOption == CLICK_KEY) {
            if (newDirection != direction) {
                direction = newDirection;
            }
        }
        // If keyboard with holdKey option, record one of nine directions.
        else if (holdKeyOption == HOLD_KEY) {
            const Direction d [9] = {UP_LEFT,   UP,    UP_RIGHT,
                                     LEFT,      STAND, RIGHT,
                                     DOWN_LEFT, DOWN,  DOWN_RIGHT};
            direction = d [(3 * (dY + 1)) + (dX + 1)];
        }
        // Record the direction, but do not extend the initial wait-time.
        if ((direction != NO_DIRECTION) && (playState == Playing)) { // IDW
// IDW Need a better condition here. (playState == Playing) was to stop
// IDW the HOLD_KEY option recording a whole lot of STAND directions before
// IDW the first key is pressed. (direction != NO_DIRECTION) is previous code.
            record (2, DIRECTION_CODE + direction);
        }
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
        //qDebug() << "END OF LEVEL";
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
                    dX = movement [direction][X];
                    dY = movement [direction][Y];
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
        else if (code < KEY_OPT_CODE) {
            dbe2 "T %04d recIndex %03d PLAY control-mode code %d\n",
                 T, recIndex, code);
            setControlMode (code - MODE_CODE);
            recIndex++;
            code = recording->content [recIndex];
            recCount = 0;
            continue;
        }

        // Replay a change of keyboard click/hold option.
        else if (code < ACTION_CODE) {
            dbe2 "T %04d recIndex %03d PLAY key-option code %d\n",
                 T, recIndex, code);
            setHoldKeyOption (code - KEY_OPT_CODE + CLICK_KEY);
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
        record (1, ACTION_CODE + KILL_HERO);

        emit endLevel (DEAD);
        //qDebug() << "END OF LEVEL";
    }
}

void KGrLevelPlayer::setControlMode  (const int mode)
{
    controlMode = mode;

    if (! playback) {
        // Record the change of mode.
        record (1, MODE_CODE + mode);
    }
}

void KGrLevelPlayer::setHoldKeyOption  (const int option)
{
    holdKeyOption = option;

    if (! playback) {
        // Record the change of keyboard click/hold option.
        record (1, KEY_OPT_CODE + option - CLICK_KEY);
    }
}

void KGrLevelPlayer::setTimeScale  (const int timeScale)
{
    timer->setScale ((float) (timeScale * 0.1));

    if (! playback) {
        record (1, SPEED_CODE + timeScale);
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



