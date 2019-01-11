#include "kgrdebug.h"

/****************************************************************************
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
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

#include "kgrrunner.h"
#include "kgrlevelgrid.h"
#include "kgrrulebook.h"
#include "kgrlevelplayer.h"
#include "kgoldrunner_debug.h"

KGrRunner::KGrRunner (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                      int i, int j, const int pSpriteId,
                      KGrRuleBook * pRules, const int startDelay)
    :
    QObject     (pLevelPlayer),	// Destroy runner when level is destroyed.
    levelPlayer (pLevelPlayer),
    grid        (pGrid),
    rules       (pRules),
    spriteId    (pSpriteId),
    gridI       (i),
    gridJ       (j),
    deltaX      (0),
    deltaY      (0),
    pointCtr    (0),
    falling     (false),

    currDirection (STAND),
    currAnimation (FALL_L),

    // The start delay is zero for the hero and 50 msec for the enemies.  This
    // gives the hero about one grid-point advantage.  Without this lead, some
    // levels become impossible, notably Challenge, 4, "Quick Off The Mark" and
    // Curse of the Mummy, 20, "The Parting of the Red Sea".
    timeLeft  (TickTime + startDelay),

    leftRightSearch (true)
{
    getRules();

    gridX    = i * pointsPerCell;
    gridY    = j * pointsPerCell;

    // As soon as the initial timeLeft has expired (i.e. at the first tick in
    // the hero's case and after a short delay in the enemies' case), the
    // pointCtr will reach its maximum, the EndCell situation will occur and
    // each runner will look for a new direction and head that way if he can.
    pointCtr = pointsPerCell - 1;

    dbgLevel = 0;
}

KGrRunner::~KGrRunner()
{
}

void KGrRunner::getRules()
{
    pointsPerCell = rules->pointsPerCell();
    turnAnywhere  = rules->turnAnywhere();
    //if (spriteId < 1) {
    //    qCDebug(KGOLDRUNNER_LOG) << "pointsPerCell" << pointsPerCell
    //             << "turnAnywhere" << turnAnywhere;
    //}
}

Situation KGrRunner::situation (const int scaledTime)
{
    timeLeft -= scaledTime;
    if (timeLeft >= scaledTime) {
        dbe3 "%d sprite %02d scaled %02d timeLeft %03d - Not Time Yet\n",
             pointCtr, spriteId, scaledTime, timeLeft);
        return NotTimeYet;
    }

    if (grid->cellType  (gridI, gridJ) == BRICK) {
        dbe2 "%d sprite %02d scaled %02d timeLeft %03d - Caught in brick\n",
             pointCtr, spriteId, scaledTime, timeLeft);
        return CaughtInBrick;
    }

    gridX += deltaX;
    gridY += deltaY;
    pointCtr++;

    if (pointCtr < pointsPerCell) {
        timeLeft += interval;
        dbe2 "%d sprite %02d scaled %02d timeLeft %03d - Mid Cell\n",
             pointCtr, spriteId, scaledTime, timeLeft);
        return MidCell;
    }

    dbe2 "%d sprite %02d scaled %02d timeLeft %03d - END Cell\n",
         pointCtr, spriteId, scaledTime, timeLeft);
    return EndCell;
}

char KGrRunner::nextCell()
{
    pointCtr = 0;
    gridI    = gridX / pointsPerCell;
    gridJ    = gridY / pointsPerCell;
    return     grid->cellType  (gridI, gridJ);
}

bool KGrRunner::setNextMovement (const char spriteType, const char cellType,
                                 Direction & dir,
                                 AnimationType & anim, int & interval)
{
    bool fallingState = false;

    Flags OK = 0;
    if (spriteType == HERO) {
        dir = levelPlayer->getDirection (gridI, gridJ);
        OK  = grid->heroMoves (gridI, gridJ);
    }
    else {
        dir = levelPlayer->getEnemyDirection (gridI, gridJ, leftRightSearch);
        OK  = grid->enemyMoves (gridI, gridJ);
    }
    if ((dir >= nDirections) || (dir < 0)) {
        dir = STAND;		// Make sure indices stay within range.
    }

    if (dir == STAND) {
        anim = currAnimation;
    }
    else {
        anim = aType [dir];
    }
    if (((anim == RUN_R) || (anim == RUN_L)) && (cellType == BAR)) {
        anim = (dir == RIGHT) ? CLIMB_R : CLIMB_L;
    }

    interval = runTime;

    // if (spriteType == HERO) {
        // qCDebug(KGOLDRUNNER_LOG) << "Calling standOnEnemy() for" << gridX << gridY;
    // }
    onEnemy  = levelPlayer->standOnEnemy (spriteId, gridX, gridY);
    bool canStand = (OK & dFlag [STAND]) || (OK == 0) || onEnemy;
    if ((dir == DOWN) && (cellType == BAR)) {
        canStand = false;
    }
    bool cannotMoveAsRequired = (! (OK & dFlag [dir]));

    // TODO - Check that the trap time is the same as in KGr 3.0.
    // TODO - We seem to be early getting into/out of the hole, but
    //        the captive time is now OK.  Total t down by ~100 in 4400.
    if ((spriteType == ENEMY) && (cellType == USEDHOLE)) {
        // The enemy is in a hole.
        if (currDirection == DOWN) {
            // The enemy is at the bottom of the hole: start the captive-timer.
            dir = STAND;
            anim = currAnimation;
            interval = trapTime;
            dbe1 "T %05d id %02d Captive at [%02d,%02d]\n",
                 t.elapsed(), spriteId, gridI, gridJ);
        }
        else {
            // The enemy can start climbing out after a cycle of captive-times.
            dir = UP;
            anim = CLIMB_U;
            dbe1 "T %05d id %02d Start climb out at [%02d,%02d]\n",
                 t.elapsed(), spriteId, gridI, gridJ);
        }
    }
    else if ((! canStand) ||
             (onEnemy && (onEnemy->isFalling()) && cannotMoveAsRequired)) {
        // Must fall: cannot even walk left or right off an enemy's head here.
        fallingState = true;
        interval = onEnemy ? enemyFallTime : fallTime;
        dir  = DOWN;
        anim = (falling) ? currAnimation :
                           ((currDirection == RIGHT) ? FALL_R : FALL_L);
    }
    else if (cannotMoveAsRequired) {
        // Sprite cannot move, but the animation shows the desired direction.
        dir = STAND;
    }

    return fallingState;
}


KGrHero::KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                  int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pSpriteId, pRules, 0),

    // KGrLevelPlayer object will call setDigWhileFalling() and setNuggets().
    digWhileFalling (true),
    nuggets (0)
{
    //qCDebug(KGOLDRUNNER_LOG) << "THE HERO IS BORN at" << i << j << "sprite ID" << pSpriteId;
    rules->getHeroTimes (runTime, fallTime, enemyFallTime, trapTime);
    //qCDebug(KGOLDRUNNER_LOG) << "Hero run time" << runTime << "fall time" << fallTime;
    interval = runTime;
}

KGrHero::~KGrHero()
{
}

HeroStatus KGrHero::run (const int scaledTime)
{
    Situation s = situation (scaledTime);
    if (s == NotTimeYet) {
        return NORMAL;
    }

    // Die if a brick has closed over us.
    if (s == CaughtInBrick) {
        return DEAD;
    }

    // If standing on top row and all nuggets gone, go up a level.
    if ((gridJ == 1) && (nuggets <= 0) &&
        (grid->heroMoves (gridI, gridJ) & dFlag [STAND])) {
        return WON_LEVEL;
    }

    // Check if we have fallen onto an enemy.  If so, continue at enemy-speed.
    if (falling && (interval != enemyFallTime)) {
        // qCDebug(KGOLDRUNNER_LOG) << "Calling standOnEnemy() for" << gridX << gridY;
	onEnemy = levelPlayer->standOnEnemy (spriteId, gridX, gridY);
        if (onEnemy != nullptr) {
            interval = enemyFallTime;
            // If MidCell, hero-speed animation overshoots, but looks OK.
        }
    }

    // We need to check collision with enemies on every grid-point.
    if (levelPlayer->heroCaught (gridX, gridY)) {
        return DEAD;
    }

    // Emit StepSound once per cell or ClimbSound twice per cell.
    if (((s == EndCell) || (pointCtr == (pointsPerCell/2))) &&
        (currDirection != STAND) && (! falling)) {
        int step = ((currAnimation == RUN_R) || (currAnimation == RUN_L)) ?
                    StepSound : ClimbSound;
        if ((s == EndCell) || (step == ClimbSound)) {
            emit soundSignal (step);
        }
    }

    if (s == MidCell) {
        return NORMAL;
    }

    // Continue to the next cell.
    char cellType = nextCell();

    if (cellType == NUGGET) {
        nuggets = levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
        emit incScore (250);		// Add to the human player's score.
        if (nuggets > 0) {
            emit soundSignal (GoldSound);
        }
        else {
            emit soundSignal (LadderSound);
        }
    }

    Direction nextDirection;
    AnimationType nextAnimation;
    bool newFallingState = setNextMovement (HERO, cellType, nextDirection,
                                            nextAnimation, interval);
    if (newFallingState != falling) {
        emit soundSignal (FallSound, newFallingState);	// Start/stop falling.
        falling = newFallingState;
    }
    timeLeft += interval;
    dbe2 "%d sprite %02d [%02d,%02d] timeLeft %03d currDir %d nextDir %d "
            "currAnim %d nextAnim %d\n",
      pointCtr, spriteId, gridI, gridJ, timeLeft, currDirection, nextDirection,
      currAnimation, nextAnimation);

    if ((nextDirection == currDirection) && (nextAnimation == currAnimation)) {
        if (nextDirection == STAND) {
            return NORMAL;
        }
    }

    deltaX = movement [nextDirection][X];
    deltaY = movement [nextDirection][Y];

    // Start the running animation (repeating).
    emit startAnimation (spriteId, true, gridI, gridJ,
                         (interval * pointsPerCell * TickTime) / scaledTime,
                         nextDirection, nextAnimation);
    currAnimation = nextAnimation;
    currDirection = nextDirection;
    return NORMAL;
}

bool KGrHero::dig (const Direction diggingDirection, int & i, int & j)
{
    QString text = (diggingDirection == DIG_LEFT) ? QStringLiteral("LEFT") : QStringLiteral("RIGHT");
    // qCDebug(KGOLDRUNNER_LOG) << "Start digging" << text;

    Flags moves = grid->heroMoves (gridI, gridJ);
    bool result = false;

    // If currDirection is UP, DOWN or STAND, dig next cell left or right.
    int relativeI = (diggingDirection == DIG_LEFT) ? -1 : +1;

    if ((currDirection == LEFT) && (moves & dFlag [LEFT])) {
        // Running LEFT, so stop at -1: dig LEFT at -2 or dig RIGHT right here.
        relativeI = (diggingDirection == DIG_LEFT) ? -2 : 0;
    }
    else if ((currDirection == RIGHT) && (moves & dFlag [RIGHT])) {
        // Running RIGHT, so stop at +1: dig LEFT right here or dig RIGHT at -2.
        relativeI = (diggingDirection == DIG_LEFT) ? 0 : +2;
    }

    // The place to dig must be clear and there must be a brick under it.
    char aboveBrick = grid->cellType  (gridI + relativeI, gridJ);
    // qCDebug(KGOLDRUNNER_LOG) << "aboveBrick =" << aboveBrick;
    if ((grid->cellType  (gridI + relativeI, gridJ + 1) == BRICK) &&
        ((aboveBrick == FREE) || (aboveBrick == HOLE))) {

        // You can dig under an enemy, empty space or hidden ladder, but not a
        // trapped enemy, ladder, gold, bar, brick, concrete or false brick.
        i = gridI + relativeI;
        j = gridJ + 1;
        result = true;

        // If dig-while-falling is not allowed, prevent attempts to use it.
        // The boolean defaults to true but can be read from a setting for
        // the game, the specific level or a recording. So it can be false.
        if (! digWhileFalling) {
	    // Work out where the hero WILL be standing when he digs. In the
	    // second case, he will dig the brick that is now right under him.
            int nextGridI = (relativeI != 0) ? (gridI + relativeI/2) :
                        ((currDirection == LEFT) ? (gridI - 1) : (gridI + 1));
            Flags OK = grid->heroMoves (nextGridI, gridJ);
            bool canStand = (OK & dFlag [STAND]) || (OK == 0);
            bool enemyUnder = (onEnemy != nullptr);
            // Must be on solid ground or on an enemy (standing or riding down).
            if ((! canStand) && (nextGridI != gridI)) {
		// If cannot move to next cell and stand, is an enemy under it?
                // qCDebug(KGOLDRUNNER_LOG) << "Calling standOnEnemy() at gridX" << gridX
                         // << "for" << (nextGridI * pointsPerCell) << gridY;
                enemyUnder = (levelPlayer->standOnEnemy (spriteId,
                                        nextGridI * pointsPerCell, gridY) != nullptr);
            }
            if ((! canStand) && (! enemyUnder)) {
                qCDebug(KGOLDRUNNER_LOG) << "INVALID DIG: hero at" << gridI << gridJ
                         << "nextGridI" << nextGridI << "relI" << relativeI
                         << "dirn" << currDirection << "brick at" << i << j
                         << "heroMoves" << ((int) OK) << "canStand" << canStand
                         << "enemyUnder" << enemyUnder;
                emit invalidDig();	// Issue warning re dig while falling.
                result = false;
            }
        }
    }
    if (result) {
        emit soundSignal (DigSound);
    }

    return result;	// Tell the levelPlayer whether & where to open a hole.
}

void KGrHero::showState()
{
    fprintf (stderr, "(%02d,%02d) %02d Hero ", gridI, gridJ, spriteId);
    fprintf (stderr, " gold %02d dir %d ctr %d",
                  nuggets, currDirection, pointCtr);
    fprintf (stderr, " X %3d Y %3d anim %d dt %03d\n",
                 gridX, gridY, currAnimation, interval);
}


KGrEnemy::KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                    int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner  (pLevelPlayer, pGrid, i, j, pSpriteId, pRules, 50),
    nuggets    (0),
    birthI     (i),
    birthJ     (j),
    prevInCell (-1)
{
    rulesType     = rules->getEnemyTimes (runTime, fallTime, trapTime);
    enemyFallTime = fallTime;
    interval      = runTime;
    //qCDebug(KGOLDRUNNER_LOG) << "ENEMY" << pSpriteId << "IS BORN at" << i << j;
    //if (pSpriteId < 2) {
    //    qCDebug(KGOLDRUNNER_LOG) << "Enemy run time " << runTime << "fall time" << fallTime;
    //    qCDebug(KGOLDRUNNER_LOG) << "Enemy trap time" << trapTime << "Rules type" << rulesType;
    //}
    t.start(); // IDW
}

KGrEnemy::~KGrEnemy()
{
}

void KGrEnemy::run (const int scaledTime)
{
    Situation s = situation (scaledTime);
    if (s == NotTimeYet) {
        return;
    }

    // Die if a brick has closed over us.
    if (s == CaughtInBrick) {
        releaseCell (gridI + deltaX, gridJ + deltaY);
        emit incScore (75);		// Killed: add to the player's score.
        dbe1 "T %05d id %02d Died in brick at [%02d,%02d]\n",
             t.elapsed(), spriteId, gridI, gridJ);
        dieAndReappear();		// Move to a new (gridI, gridJ).
        reserveCell (gridI, gridJ);
        // Go to next cell, with s = CaughtInBrick, thus forcing re-animation.
    }

    else if ((pointCtr == 1) && (currDirection == DOWN) &&
        (grid->cellType (gridI, gridJ + 1) == HOLE)) {
        // Enemy is starting to fall into a hole.
        dbe1 "T %05d id %02d Mark hole [%02d,%02d] as used\n",
             t.elapsed(), spriteId, gridI, gridJ+1);
        grid->changeCellAt (gridI, gridJ + 1, USEDHOLE);
        dropGold();
        emit incScore (75);		// Trapped: add to the player's score.
        return;
    }

    // Wait till end of cell.
    else if (s == MidCell) {
        if (grid->cellType (gridI, gridJ) == USEDHOLE) {
            dbe1 "T %05d id %02d Stay captive at [%02d,%02d] count %d\n",
                 t.elapsed(), spriteId, gridI, gridJ, pointCtr);
        }
        return;
    }

    // Continue to the next cell.
    char cellType = nextCell();

    // Try to pick up or drop gold in the new cell.
    if (currDirection != STAND) {
        checkForGold();
    }

    // Find the next move that could lead to the hero.
    Direction nextDirection;
    AnimationType nextAnimation;
    bool fallingState = setNextMovement (ENEMY, cellType, nextDirection,
                                         nextAnimation, interval);
    dbe3 "\n");

    // If the enemy just left a hole, change it to empty.  Must execute this
    // code AFTER finding the next direction and valid moves, otherwise the
    // enemy will just fall back into the hole again.
    if ((currDirection == UP) &&
        (grid->cellType  (gridI, gridJ + 1) == USEDHOLE)) {
        dbk3 << spriteId << "Hole emptied at" << gridI << (gridJ + 1);
        // Empty the hole, provided it had not somehow caught two enemies.
        if (grid->enemyOccupied (gridI, gridJ + 1) < 0) {
            grid->changeCellAt (gridI, gridJ + 1, HOLE);
        }
    }

    dbe2 "%d sprite %02d [%02d,%02d] timeLeft %03d currDir %d nextDir %d "
            "currAnim %d nextAnim %d\n",
      pointCtr, spriteId, gridI, gridJ, timeLeft,
      currDirection, nextDirection, currAnimation, nextAnimation);

    if (fallingState != falling) {
        falling = fallingState;
        if (falling) {
            t.restart();
            dbe1 "T %05d id %02d Start falling\n", t.elapsed(), spriteId);
        }
    }

    // Check for a possible collision with another enemy.
    if (levelPlayer->bumpingFriend (spriteId, nextDirection, gridI, gridJ)) {
        nextDirection = STAND;			// Wait for one timer interval.
        pointCtr = pointsPerCell - 1;		// Try again after the interval.
    }

    if ((rulesType == KGoldrunnerRules) && (nextDirection == STAND) &&
        (cellType != USEDHOLE)) {
        // In KGoldrunner rules, if unable to move, switch the search direction.
        leftRightSearch = (leftRightSearch) ? false : true;
        pointCtr = pointsPerCell - 1;
    }

    timeLeft += interval;
    deltaX = movement [nextDirection][X];
    deltaY = movement [nextDirection][Y];

    // If moving, occupy the next cell in the enemy's path and release this one.
    if (nextDirection != STAND) {
        // If multiple enemies move into a cell, they stack.  Each enemy uses
        // nextInCell to remember the ID of the enemy beneath it (or -1).  The
        // top enemy is remembered by grid->setEnemyOccupied().
        //
        // NOTE: In Scavenger rules, multiple enemies can fall or climb into a
        // a cell, but should keep to one per cell if moving horizontally.  In
        // Traditional or KGoldrunner rules, it should not be possible for
        // multiple enemies to enter a cell, but anomalies can happen, so a
        // stack is used to record them temporarily and split them up again.

        releaseCell (gridI, gridJ);

        int nextI  = gridI + deltaX;
        int nextJ  = gridJ + deltaY;
        reserveCell (nextI, nextJ);
    }

    if ((nextDirection == currDirection) && (nextAnimation == currAnimation)) {
        if ((nextDirection == STAND) && (s != CaughtInBrick)) {
            // In the CaughtInBrick situation, enemy sprites must not be shown
            // standing in the bricks where they died: we must re-animate them.
            return;
        }
    }

    // Start the running animation (repeating).
    emit startAnimation (spriteId, true, gridI, gridJ,
                         (interval * pointsPerCell * TickTime) / scaledTime,
                         nextDirection, nextAnimation);
    currAnimation = nextAnimation;
    currDirection = nextDirection;
}

void KGrEnemy::dropGold()
{
    if (nuggets > 0) {
        // If enemy is trapped when carrying gold, he must give it up.
        nuggets = 0;
        // Can drop in an empty cell, otherwise it is lost (no score for hero).
        bool lost = (grid->cellType  (gridI, gridJ) != FREE);
        levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false, lost);
    }
}

void KGrEnemy::checkForGold()
{
    char cell = grid->cellType (gridI, gridJ);
    uchar random;
    if ((nuggets == 0) && (cell == NUGGET)) {
        bool collect = rules->alwaysCollectNugget();
        if (! collect) {
            random = levelPlayer->randomByte ((uchar) 100);
            dbk3 << "Random" << random << "at NUGGET" << gridI << gridJ;
            collect = (random >= 80);
        }
        if (collect) {
            levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
            dbk1 << "Enemy" << spriteId << "at" << gridI << gridJ
                << "COLLECTS gold";
            nuggets = 1;
        }
    }
    else if ((nuggets > 0) && (cell == FREE)) {
        // Dropping gold is a random choice, but do not drop in thin air.
        char below = grid->cellType (gridI, gridJ + 1);
        if ((below != FREE) && (below != NUGGET) && (below != BAR)) {
            random = levelPlayer->randomByte ((uchar) 100);
            dbk3 << "Random" << random << "for DROP " << gridI << gridJ;
            if (random >= 93) {
                levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false);
                dbk1 << "Enemy" << spriteId << "at" << gridI << gridJ
                    <<"DROPS gold";
                nuggets = 0;
            }
        }
    }
}

void KGrEnemy::dieAndReappear()
{
    if (nuggets > 0) {
        // Enemy died and could not drop nugget.  Gold is LOST - no score.
        nuggets = 0;			// Set lost-flag in runnerGotGold().
        levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false, true);
    }

    if (rules->reappearAtTop()) {
        // Traditional or Scavenger rules.
        dbk3 << spriteId << "REAPPEAR AT TOP";
        levelPlayer->enemyReappear (gridI, gridJ);
    }
    else {
        // KGoldrunner rules.
        dbk3 << spriteId << "REAPPEAR AT BIRTHPLACE";
        gridI = birthI;
        gridJ = birthJ;
    }

    // Set up the enemy's state so as to be ready for moving to the next cell.

    // There is no time-delay and no special animation here, though there was
    // in the Apple II game and there is in Scavenger.  KGoldrunner has never
    // had a time-delay here, which makes KGoldrunner more difficult sometimes.
    gridX         = gridI * pointsPerCell;
    gridY         = gridJ * pointsPerCell;
    deltaX        = 0;
    deltaY        = 0;
    pointCtr      = pointsPerCell;
    falling       = false;
    interval      = runTime;
    timeLeft      = TickTime;
    currDirection = STAND;
    currAnimation = FALL_L;
}

void KGrEnemy::reserveCell (const int i, const int j)
{
    // Push down a previous enemy or -1 if the cell was empty.
    prevInCell = grid->enemyOccupied (i, j);
    grid->setEnemyOccupied (i, j, spriteId);
    dbe3 "%02d Entering [%02d,%02d] pushes %02d\n", spriteId, i, j, prevInCell);
}

void KGrEnemy::releaseCell (const int i, const int j)
{
    // Pop up a previous enemy or -1 if the cell was empty.
    if (spriteId == grid->enemyOccupied (i, j)) {
        grid->setEnemyOccupied (i, j, prevInCell);
    }
    else {
        levelPlayer->unstackEnemy (spriteId, i, j, prevInCell);
    }
    dbe3 "%02d Leaves [%02d,%02d] to %02d\n", spriteId, i, j, prevInCell);
}

void KGrEnemy::showState()
{
    fprintf (stderr, "(%02d,%02d) %02d Enemy", gridI, gridJ, spriteId);
    fprintf (stderr, " gold %02d dir %d ctr %d",
                  nuggets, currDirection, pointCtr);
    fprintf (stderr, " X %3d Y %3d anim %d dt %03d prev %d\n",
                 gridX, gridY, currAnimation, interval, prevInCell);
}


