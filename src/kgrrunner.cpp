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

#include <KDebug>

#include "kgrrunner.h"
#include "kgrlevelplayer.h"
#include "kgrlevelgrid.h"
#include "kgrrulebook.h"

KGrRunner::KGrRunner (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                      int i, int j, int pSpriteId, KGrRuleBook * pRules)
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

    currDirection (STAND),
    currAnimation (FALL_L),

    timeLeft  (TickTime),
    leftRightSearch (true)
{
    getRules();

    gridX = i * pointsPerCell;
    gridY = j * pointsPerCell;
}

KGrRunner::~KGrRunner()
{
}

void KGrRunner::getRules()
{
    pointsPerCell = rules->pointsPerCell();
    turnAnywhere  = rules->turnAnywhere();
    if (spriteId < 1) {
        kDebug() << "pointsPerCell" << pointsPerCell
                 << "turnAnywhere" << turnAnywhere;
    }
}

Situation KGrRunner::situation (const int scaledTime)
{
    timeLeft -= scaledTime;
    if (timeLeft >= scaledTime) {
        return NotTimeYet;
    }

    if (grid->cellType  (gridI, gridJ) == BRICK) {
        return CaughtInBrick;
    }

    gridX += deltaX;
    gridY += deltaY;
    pointCtr++;

    // TODO - Count one extra tick when turning to L or R from another dirn.
    if (pointCtr < pointsPerCell) {
        timeLeft += interval;
        return MidCell;
    }

    return EndCell;
}


KGrHero::KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                  int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pSpriteId, pRules),
    nuggets (0)		// KGrLevelPlayer object will call hero->setNuggets().
{
    kDebug() << "THE HERO IS BORN at" << i << j << "sprite ID" << pSpriteId;
    rules->getHeroTimes (runTime, fallTime);
    kDebug() << "Hero run time" << runTime << "fall time" << fallTime;
    interval = runTime;
}

KGrHero::~KGrHero()
{
}

// These statics will be set to their proper values in the constructor.
int KGrHero::runTime  = 0;
int KGrHero::fallTime = 0;

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

    // TODO - If on top row and all nuggets gone, plus Scav cond, go up a level.
    if ((gridJ == 1) && (nuggets <= 0)) {
        return WON_LEVEL;
    }

    if (s == MidCell) {
        return NORMAL;
    }

    pointCtr = 0;
    gridI    = gridX / pointsPerCell;
    gridJ    = gridY / pointsPerCell;

    char cell = grid->cellType  (gridI, gridJ);
    if (cell == NUGGET) {
        nuggets = levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
        emit incScore (250);		// Add to the human player's score.
    }

    Direction nextDirection = levelPlayer->getDirection (gridI, gridJ);
    interval = runTime;

    Flags OK       = grid->heroMoves (gridI, gridJ);
    bool  onEnemy  = levelPlayer->standOnEnemy (spriteId, gridX, gridY);
    bool  canStand = (OK & dFlag [STAND]) || (OK == 0) || onEnemy;
    // kDebug() << "Direction" << nextDirection << "Flags" << OK
             // << "at" << gridI << gridJ
             // << "on Enemy" << onEnemy;
    // TODO - Do a better, smoother job of falling while standing on enemy.
    // TODO - Interface fall start and end with sound.

    AnimationType nextAnimation = aType [nextDirection];
    if (! canStand) {
        nextDirection = DOWN;
        nextAnimation = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }
    else if (! (OK & dFlag [nextDirection])) {
        nextDirection = STAND;
    }

    if (nextDirection == STAND) {
        nextAnimation = currAnimation;
    }

    timeLeft += interval;

    if ((! onEnemy) && levelPlayer->heroCaught (gridX, gridY)) {
        return DEAD;
    }

    if (((nextAnimation == RUN_R) || (nextAnimation == RUN_L)) &&
        (cell == BAR)) {
        nextAnimation = (nextDirection == RIGHT) ? CLIMB_R : CLIMB_L;
    }

    if ((nextDirection == currDirection) && (nextAnimation == currAnimation)) {
        // TODO - If dirn == STAND, no animation, else emit resynchAnimation().
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
    QString text = (diggingDirection == DIG_LEFT) ? "LEFT" : "RIGHT";
    // kDebug() << "Start digging" << text;

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
    // kDebug() << "aboveBrick =" << aboveBrick;
    if ((grid->cellType  (gridI + relativeI, gridJ + 1) == BRICK) &&
        ((aboveBrick == FREE) || (aboveBrick == HOLE))) {

        // You can dig under an enemy, empty space or hidden ladder, but not a
        // trapped enemy, ladder, gold, bar, brick, concrete or false brick.
        i = gridI + relativeI;
        j = gridJ + 1;
        result = true;
    }
    return result;	// Tell the levelPlayer whether & where to open a hole.
}

void KGrHero::showState (char option)
{
    fprintf (stderr, "(%02d,%02d) - Hero      ", gridI, gridJ);
    switch (option) {
        case 'p': fprintf (stderr, "\n"); break;
        case 's': fprintf (stderr, " STATE\n");
            // TODO - Print the hero's state.
            // fprintf (stderr, " nuggets %02d status %d walk-ctr %d ",
                          // nuggets, status, walkCounter);
            // fprintf (stderr, "dirn %d next dirn %d\n", direction, nextDir);
            // fprintf (stderr, "                     rel (%02d,%02d) abs (%03d,%03d)",
                        // relx, rely, absx, absy);
            // fprintf (stderr, " pix %02d", actualPixmap);
            // fprintf (stderr, " mem %d %d %d %d", mem_x, mem_y, mem_relx, mem_rely);
            // if (walkFrozen) fprintf (stderr, " wBlock");
            // if (fallFrozen) fprintf (stderr, " fBlock");
            // fprintf (stderr, "\n");
            break;
    }
}


KGrEnemy::KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                    int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner  (pLevelPlayer, pGrid, i, j, pSpriteId, pRules),
    nuggets    (0),
    birthI     (i),
    birthJ     (j),
    prevInCell (-1)
{
    rulesType = rules->getEnemyTimes (runTime, fallTime, trapTime);
    interval  = runTime;
    kDebug() << "ENEMY" << pSpriteId << "IS BORN at" << i << j;
    if (pSpriteId < 2) {
        kDebug() << "Enemy run time " << runTime << "fall time" << fallTime;
        kDebug() << "Enemy trap time" << trapTime << "Rules type" << rulesType;
    }
}

KGrEnemy::~KGrEnemy()
{
}

// These statics will be set to their proper values in the constructor.
int KGrEnemy::runTime  = 0;
int KGrEnemy::fallTime = 0;
int KGrEnemy::trapTime = 0;

void KGrEnemy::run (const int scaledTime)
{
    Situation s = situation (scaledTime);
    if (s == NotTimeYet) {
        return;
    }

    // Die if a brick has closed over us.
    if (s == CaughtInBrick) {
        // TODO - What time-delay (if any) is involved here?
        // TODO - How should pointCtr, interval and timeLeft be affected?
        // TODO - What if >1 enemy has occupied a cell somehow?
        // TODO - Effect on CPU time of more frequent BRICK checks ...
        releaseCell (gridI + deltaX, gridJ + deltaY);
        emit incScore (75);		// Add to the human player's score.
        dieAndReappear();		// Move to a new (gridI, gridJ).
        reserveCell (gridI, gridJ);
        // No return: treat situation as EndCell.
    }

    else if ((pointCtr == 1) && (currDirection == DOWN) &&
        (grid->cellType (gridI, gridJ + 1) == HOLE)) {
        // Enemy is starting to fall into a hole.
        dbk3 << spriteId
                 << "Falling into hole at:" << gridI << (gridJ + 1);
        grid->changeCellAt (gridI, gridJ + 1, USEDHOLE);
        dropGold();
        emit incScore (75);		// Add to the human player's score.
        return;
    }

    // Wait till end of cell.
    else if (s == MidCell) {
        return;
    }

    // Continue to the next cell.
    pointCtr = 0;
    gridI = gridX / pointsPerCell;
    gridJ = gridY / pointsPerCell;

    // Try to pick up or drop gold in the new cell.
    if (currDirection != STAND) {
        checkForGold();
    }

    // Find the next direction that could lead to the hero.
    dbe3 "\n");
    Direction nextDirection = levelPlayer->getEnemyDirection
                                           (gridI, gridJ, leftRightSearch);
    dbk3 << spriteId << "at" << gridI << gridJ << "===>" << nextDirection;
    Flags     OK            = grid->enemyMoves (gridI, gridJ);

    // If the enemy just left a hole, change it to empty.  Must execute this
    // code AFTER finding the next direction and valid moves, otherwise the
    // enemy will just fall back into the hole again.
    if ((currDirection == UP) &&
        (grid->cellType  (gridI, gridJ + 1) == USEDHOLE)) {
        dbk3 << spriteId << "Hole emptied at" << gridI << (gridJ + 1);
        if (grid->enemyOccupied (gridI, gridJ + 1) < 0) // IDW TODO needed?
        grid->changeCellAt (gridI, gridJ + 1, HOLE);
    }

    // kDebug() << "Enemy" << spriteId << "Direction" << nextDirection
             // << "Flags" << OK << "at" << gridI << gridJ;

    AnimationType nextAnimation = aType [nextDirection];
    bool onEnemy  = levelPlayer->standOnEnemy (spriteId, gridX, gridY);
    if (onEnemy) dbk3 << spriteId << "STANDING ON ENEMY - dirn"
                          << nextDirection;
    // TODO - Do a better, smoother job of falling while standing on enemy.
    // TODO - Especially, do not do a climb-down action.
    bool canStand = (OK & dFlag [STAND]) || (OK == 0) || onEnemy;
    char cellType = grid->cellType (gridI, gridJ);
    interval      = runTime;

    // If arrived in USEDHOLE (as "reserved" above), start trapTime.
    if (cellType == USEDHOLE) {
        if (currDirection == DOWN) {
            nextDirection = STAND;
            // TODO - Check that the trap time is the same as in KGr 3.0.
            // TODO - Ugly 7.  Ugly arithmetic too.  270 * 7 / 4 = 472.5.
            interval = (trapTime * 7) / pointsPerCell;
            dbk3 << spriteId << "Arrived in USEDHOLE at" << gridI << gridJ;
        }
        else {
            nextDirection = UP;
            nextAnimation = CLIMB_U;
            dbk3 << spriteId << "Leaving USEDHOLE at" << gridI << gridJ;
        }
    }
    else if (! canStand) {
        nextDirection = DOWN;
        nextAnimation = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }
    else if (! (OK & dFlag [nextDirection])) {
        nextDirection = STAND;
    }

    if (nextDirection == STAND) {
        nextAnimation = currAnimation;
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
        pointCtr = pointsPerCell - 1; // TODO - Improve & stress-test KGr rules.
    }

    if (((nextAnimation == RUN_R) || (nextAnimation == RUN_L)) &&
        (cellType == BAR)) {
        nextAnimation = (nextDirection == RIGHT) ? CLIMB_R : CLIMB_L;
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
        // TODO - If dirn == STAND, no animation, else emit resynchAnimation().
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
        random = levelPlayer->randomByte ((uchar) 100);
        dbk3 << "Random" << random << "at NUGGET" << gridI << gridJ;
        if (rules->alwaysCollectNugget() || (random >= 80)) {
            levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
            dbk3 << "Enemy" << spriteId << "at" << gridI << gridJ
                << "COLLECTS gold";
            nuggets = 1;
        }
    }
    else if ((nuggets > 0) && (cell == FREE)) {
        // Dropping gold is a random choice, but do not drop in thin air.
        char below = grid->cellType (gridI, gridJ + 1);
        // TODO - Do not drop above a BAR.  Affects recording of Initiation 10.
        if ((below != FREE) && (below != NUGGET)) {
            random = levelPlayer->randomByte ((uchar) 100);
            dbk3 << "Random" << random << "for DROP " << gridI << gridJ;
            if (random >= 93) {
                levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false);
                dbk3 << "Enemy" << spriteId << "at" << gridI << gridJ
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
    // TODO - emit killed (75);		// Killed an enemy: score 75.

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

    pointCtr = 0;
    gridX = gridI * pointsPerCell;
    gridY = gridJ * pointsPerCell;
    currDirection = STAND;
    deltaX = 0;
    deltaY = 0;
    // TODO - Need more re-initialisation?  See KGrRunner constructor.
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

void KGrEnemy::showState (char option)
{
    fprintf (stderr, "(%02d,%02d) - Enemy  [%02d]", gridI, gridJ, spriteId);
    switch (option) {
    // TODO - Remove the 'p'/'s' parameter here and in all calls.
    case 'p':
    case 's':
        fprintf (stderr, " gold %02d dirn %d ctr %d",
                      nuggets, currDirection, pointCtr);
        fprintf (stderr, " gridX %3d gridY %3d anim %d prev %02d\n",
                     gridX, gridY, currAnimation, prevInCell);
        break;
    }
}

#include "kgrrunner.moc"
