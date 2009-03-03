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
#include <stdlib.h>		// For random-number generator.

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

    currDirection (STAND),
    currAnimation (FALL_L),

    timeLeft  (TickTime)
{
    getRules();

    gridX     = i * pointsPerCell;
    gridY     = j * pointsPerCell;
    pointCtr  = 0;

    vector[X] = 0;
    vector[Y] = 0;
}

KGrRunner::~KGrRunner()
{
}

void KGrRunner::getRules()
{
    pointsPerCell = rules->pointsPerCell();
    turnAnywhere  = rules->turnAnywhere();
    kDebug() << "pointsPerCell" << pointsPerCell << "turnAnywhere" << turnAnywhere;
}

bool KGrRunner::notTimeYet (const int scaledTime)
{
    timeLeft -= scaledTime;
    if (timeLeft >= scaledTime) {
        // kDebug() << "1: Hero interval is:" << interval << "time left:" << timeLeft;
        return true;
    }

    gridX += vector[X];
    gridY += vector[Y];
    pointCtr++;

    // TODO - Count one extra tick when turning to L or R from another dirn.
    if (pointCtr < pointsPerCell) {
        // kDebug() << "2: Hero interval is:" << interval << "time left:" << timeLeft;
        timeLeft += interval;
        return true;
    }
    return false;
}


KGrHero::KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                  int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pSpriteId, pRules),
    nuggets (1000) // TODO - This won't work in the Count, Zero level.
    // Implement KGrRunner::setNuggets (int n); and nuggets as a runner value.
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
    if (notTimeYet (scaledTime)) {
        return NORMAL;
    }

    // Die if a brick has closed over us.
    if (grid->cellType  (gridI, gridJ) == BRICK) {
        return DEAD;
    }

    // TODO - If on top row and all nuggets gone, plus Scav cond, go up a level.
    if ((gridJ == 1) && (nuggets <= 0)) {
        return WON_LEVEL;
    }

    // TODO - Use nextDirection here, and set currDirection (at end?).

    pointCtr = 0;
    gridI    = gridX / pointsPerCell;
    gridJ    = gridY / pointsPerCell;

    char cell = grid->cellType  (gridI, gridJ);
    if (cell == NUGGET) {
        nuggets = levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
    }

    Direction dirn = levelPlayer->getDirection (gridI, gridJ);
    interval = runTime;

    Flags OK       = grid->heroMoves (gridI, gridJ);
    bool  onEnemy  = levelPlayer->standOnEnemy (spriteId, gridX, gridY);
    bool  canStand = (OK & dFlag [STAND]) || (OK == 0) || onEnemy;
    kDebug() << "Direction" << dirn << "Flags" << OK << "at" << gridI << gridJ
             << "on Enemy" << onEnemy;

    AnimationType anim = aType [dirn];
    if (! canStand) {
        dirn = DOWN;
        anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }
    else if (! (OK & dFlag [dirn])) {
        dirn = STAND;
    }

    // TODO - Remove this if above code is OK.  Re-do KGrEnemy::run() code.
    // if (OK & dFlag [dirn]) {
        // if ((dirn == DOWN) && (! canStand)) {
            // anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
            // interval = fallTime;
        // }
    // }
    // else if ((canStand) || (OK == 0)) {
        // dirn = STAND;
    // }
    // else {
        // dirn = DOWN;
        // anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
        // interval = fallTime;
    // }

    if (dirn == STAND) {
        anim = currAnimation;
    }

    timeLeft += interval;
    // kDebug() << "3: Hero interval is:" << interval << "time left:" << timeLeft;

    // TODO - Check for collision with an enemy somewhere around here.
    if ((! onEnemy) && levelPlayer->heroCaught (gridX, gridY)) {
        return DEAD;
    }

    if (((anim == RUN_R) || (anim == RUN_L)) && (cell == POLE)) {
        anim = (dirn == RIGHT) ? CLIMB_R : CLIMB_L;
    }

    if ((dirn == currDirection) && (anim == currAnimation)) {
        // TODO - If dirn == STAND, no animation, else emit resynchAnimation().
        if (dirn == STAND) {
            return NORMAL;
        }
    }

    vector [X] = movement [dirn][X];
    vector [Y] = movement [dirn][Y];

    // kDebug() << "New direction" << dirn << vector [X] << vector [Y] << gridI << gridJ;
    // kDebug() << "Sprite" << spriteId << "Animate" << gridI << gridJ << "time" << interval * pointsPerCell << "dirn" << dirn << "anim" << anim;

    // Start the running animation (repeating).
    emit startAnimation (spriteId, true, gridI, gridJ,
                         interval * pointsPerCell, dirn, anim);
    currAnimation = anim;
    currDirection = dirn;
    return NORMAL;
}

bool KGrHero::dig (const Direction diggingDirection, int & i, int & j)
{
    QString text = (diggingDirection == DIG_LEFT) ? "LEFT" : "RIGHT";
    kDebug() << "Start digging" << text;

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
    kDebug() << "aboveBrick =" << aboveBrick;
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
    printf ("(%02d,%02d) - Hero      ", gridI, gridJ);
    switch (option) {
        case 'p': printf ("\n"); break;
        case 's': printf (" STATE\n");
            // TODO - Print the hero's state.
            // printf (" nuggets %02d status %d walk-ctr %d ",
                          // nuggets, status, walkCounter);
            // printf ("dirn %d next dirn %d\n", direction, nextDir);
            // printf ("                     rel (%02d,%02d) abs (%03d,%03d)",
                        // relx, rely, absx, absy);
            // printf (" pix %02d", actualPixmap);
            // printf (" mem %d %d %d %d", mem_x, mem_y, mem_relx, mem_rely);
            // if (walkFrozen) printf (" wBlock");
            // if (fallFrozen) printf (" fBlock");
            // printf ("\n");
            break;
    }
}


KGrEnemy::KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                    int i, int j, int pSpriteId, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pSpriteId, pRules),
    nuggets   (0),
    birthI    (i),
    birthJ    (j)
{
    kDebug() << "ENEMY" << pSpriteId << "IS BORN at" << i << j;
    rules->getEnemyTimes (runTime, fallTime, trapTime);
    kDebug() << "Enemy run time " << runTime << "fall time" << fallTime;
    kDebug() << "Enemy trap time" << trapTime;
    interval = runTime;
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
    if (notTimeYet (scaledTime)) {
        if ((pointCtr == 1) && (currDirection == DOWN) &&
            (grid->cellType (gridI, gridJ + 1) == HOLE)) {
            // Enemy is starting to fall into a hole.
            kDebug() << "Enemy" << spriteId <<
                        "falling into hole at:" << gridI << (gridJ + 1);
            grid->changeCellAt (gridI, gridJ + 1, USEDHOLE);
            dropGold();
        }
        return;
    }

    // TODO - Use nextDirection here, and set currDirection (at end?).

    // Die and reappear if a brick has closed over us.
    if (grid->cellType (gridI, gridJ) == BRICK) {
        // TODO - What time-delay (if any) is involved here?
        // TODO - How should pointCtr, interval and timeLeft be affected?
        dieAndReappear();
    }

    // Otherwise, just go on to the next cell.
    else {
        pointCtr = 0;
        gridI    = gridX / pointsPerCell;
        gridJ    = gridY / pointsPerCell;
    }

    // Try to pick up or drop gold in this cell.
    checkForGold();

    // TODO - Call rules->findBestWay(), but where is the hero?
    // Find the next direction that could lead to the hero.
    Direction nextDirection = levelPlayer->getEnemyDirection (gridI, gridJ);
    Flags     OK            = grid->enemyMoves (gridI, gridJ);

    // If the enemy just left a hole, change it to empty.  Must execute this
    // code AFTER finding the next direction and valid moves, otherwise the
    // enemy will just fall back into the hole again.
    if ((currDirection == UP) &&
        (grid->cellType  (gridI, gridJ + 1) == USEDHOLE)) {
        kDebug() << "Hole emptied at" << gridI << (gridJ + 1);
        grid->changeCellAt (gridI, gridJ + 1, HOLE);
    }

    kDebug() << "Enemy" << spriteId << "Direction" << nextDirection <<
                "Flags" << OK << "at" << gridI << gridJ;

    AnimationType nextAnimation = aType [nextDirection];
    bool          canStand      = OK & dFlag [STAND];
    char          cellType      = grid->cellType (gridI, gridJ);
                  interval      = runTime;

    // If arrived in USEDHOLE (as "reserved" above), start trapTime.
    if (cellType == USEDHOLE) {
        if (currDirection == DOWN) {
            nextDirection = STAND;
            // TODO - Check that the trap time is the same as in KGr 3.0.
            // TODO - Ugly 7.  Ugly arithmetic too.  270 * 7 / 4 = 472.5.
            interval = (trapTime * 7) / pointsPerCell;
        }
        else {
            nextDirection = UP;
            nextAnimation = CLIMB_U;
        }
    }
    else if (OK & dFlag [nextDirection]) {
        if ((nextDirection == DOWN) && (! canStand)) {
            nextAnimation = (currDirection == RIGHT) ? FALL_R : FALL_L;
            interval = fallTime;
        }
    }
    else if ((canStand) || (OK == 0)) {
        nextDirection = STAND;
    }
    else {
        nextDirection = DOWN;
        nextAnimation = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }

    if (nextDirection == STAND) {
        nextAnimation = currAnimation;
    }

    timeLeft += interval;
    // kDebug() << "3: Hero interval is:" << interval << "time left:" << timeLeft;

    // TODO - Check for collision with an enemy somewhere around here.

    if (((nextAnimation == RUN_R) || (nextAnimation == RUN_L)) &&
        (cellType == POLE)) {
        nextAnimation = (nextDirection == RIGHT) ? CLIMB_R : CLIMB_L;
    }

    if ((nextDirection == currDirection) && (nextAnimation == currAnimation)) {
        // TODO - If dirn == STAND, no animation, else emit resynchAnimation().
        if (nextDirection == STAND) {
            return;
        }
    }

    vector [X] = movement [nextDirection][X];
    vector [Y] = movement [nextDirection][Y];

    // kDebug() << "New direction" << nextDirection << vector [X] << vector [Y] << gridI << gridJ;
    // kDebug() << "Sprite" << spriteId << "Animate" << gridI << gridJ << "time" << interval * pointsPerCell << "nextDirection" << nextDirection << "anim" << anim;

    // Start the running animation (repeating).
    emit startAnimation (spriteId, true, gridI, gridJ,
                         interval * pointsPerCell, nextDirection, nextAnimation);
    currAnimation = nextAnimation;
    currDirection = nextDirection;
    return;
}

void KGrEnemy::dropGold()
{
    if (nuggets > 0) {
        // If enemy is trapped when carrying gold, he must give it up.
        nuggets = 0;
        if (grid->cellType  (gridI, gridJ) == FREE) {
            // OK, drop the gold here.
            levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false);
        }
        else {
            // Cannot drop it here, so it must be lost (no score for the hero).
            // TODO - emit lostNugget();
        }
    }
}

void KGrEnemy::checkForGold()
{
    char cell = grid->cellType (gridI, gridJ);
    if ((nuggets == 0) && (cell == NUGGET) &&
        (rules->alwaysCollectNugget() || (rand()/(RAND_MAX + 1.0) >= 0.8))) {
        // In KGoldrunner rules picking up gold is a random choice.
        levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
        nuggets = 1;
    }
    else if ((nuggets > 0) && (cell == FREE) &&
             (rand()/(RAND_MAX + 1.0) >= 0.9286)) {
        // Dropping gold is a random choice.
        levelPlayer->runnerGotGold (spriteId, gridI, gridJ, false);
        nuggets = 0;
    }
}

void KGrEnemy::dieAndReappear()
{
    if (nuggets > 0) {
        nuggets = 0;			// Enemy died and could not drop nugget.
        // TODO - emit lostNugget();		// NO SCORE for lost nugget.
    }
    // TODO - emit killed (75);			// Killed an enemy: score 75.

    if (rules->reappearAtTop()) {
        // Traditional or Scavenger rules.
        kDebug() << spriteId << "REAPPEAR AT TOP";
        levelPlayer->enemyReappear (gridI, gridJ);
    }
    else {
        // KGoldrunner rules.
        kDebug() << spriteId << "REAPPEAR AT BIRTHPLACE";
        gridI = birthI;
        gridJ = birthJ;
    }

    pointCtr = 0;
    gridX = gridI * pointsPerCell;
    gridY = gridJ * pointsPerCell;
    // TODO - Need more re-initialisation?  See KGrRunner constructor.
}

void KGrEnemy::showState (char option)
{
    printf ("(%02d,%02d) - Enemy  [%d]", gridI, gridJ, spriteId);
    switch (option) {
        case 'p': printf ("\n"); break;
        case 's': printf (" STATE\n");
            // TODO - Print the enemy's state.
            // printf (" nuggets %02d status %d walk-ctr %d ",
                          // nuggets, status, walkCounter);
            // printf ("dirn %d search %d capt-ctr %d\n",
                        // direction, searchStatus, captiveCounter);
            // printf ("                     rel (%02d,%02d) abs (%03d,%03d)",
                        // relx, rely, absx, absy);
            // printf (" pix %02d", actualPixmap);
            // printf (" mem %d %d %d %d", mem_x, mem_y, mem_relx, mem_rely);
            // if (walkFrozen) printf (" wBlock");
            // if (fallFrozen) printf (" fBlock");
            // if (captiveFrozen) printf (" cBlock");
            // printf ("\n");
            break;
    }
}

#include "kgrrunner.moc"
