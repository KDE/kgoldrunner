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
    gridI       (i),
    gridJ       (j),
    spriteId    (pSpriteId),

    currDirection (STAND),
    currAnimation (FALL_L),

    timeLeft  (TickTime)
{
    vector[X] = 0;
    vector[Y] = 0;

    getRules();
    pointCtr = 0;
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
    timeLeft -= scaledTime;
    if (timeLeft >= scaledTime) {
        // kDebug() << "1: Hero interval is:" << interval << "time left:" << timeLeft;
        return NORMAL;
    }

    pointCtr++;
    // TODO - Count one extra tick when turning to L or R from another dirn.
    if (pointCtr < pointsPerCell) {
        // kDebug() << "2: Hero interval is:" << interval << "time left:" << timeLeft;
        timeLeft += interval;
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
    gridI    = gridI + vector [X];
    gridJ    = gridJ + vector [Y];

    char cell = grid->cellType  (gridI, gridJ);
    if (cell == NUGGET) {
        nuggets = levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
    }

    Direction dirn = levelPlayer->getDirection (gridI, gridJ);
    interval = runTime;

    Flags OK  = grid->heroMoves (gridI, gridJ);
    bool canStand = OK & dFlag [STAND];
    // kDebug() << "Direction" << dirn << "Flags" << OK << "at" << gridI << gridJ;

    AnimationType anim = aType [dirn];
    if (OK & dFlag [dirn]) {
        if ((dirn == DOWN) && (! canStand)) {
            anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
            interval = fallTime;
        }
    }
    else if (canStand) {
        dirn = STAND;
    }
    else {
        dirn = DOWN;
        anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }

    if (dirn == STAND) {
        anim = currAnimation;
    }

    timeLeft += interval;
    // kDebug() << "3: Hero interval is:" << interval << "time left:" << timeLeft;

    // TODO - Check for collision with an enemy somewhere around here.

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
    nuggets   (0)
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
    timeLeft -= scaledTime;
    if (timeLeft >= scaledTime) {
        // kDebug() << "1: Hero interval is:" << interval << "time left:" << timeLeft;
        return;
    }

    pointCtr++;
    // TODO - Count one extra tick when turning to L or R from another dirn.
    if (pointCtr < pointsPerCell) {
        // kDebug() << "2: Hero interval is:" << interval << "time left:" << timeLeft;
        timeLeft += interval;
        return;
    }

    // TODO - If falling and the next cell is a hole, drop or lose nugget.
    // TODO - Set the hole to USEDHOLE.
    // TODO - Die and reappear if a brick has closed over us.
    if (grid->cellType  (gridI, gridJ) == BRICK) {
        return;
    }

    // TODO - Use nextDirection here, and set currDirection (at end?).

    pointCtr = 0;
    gridI    = gridI + vector [X];
    gridJ    = gridJ + vector [Y];

    char cell = grid->cellType  (gridI, gridJ);
    // TODO - Apply enemy's rules for picking up or dropping gold in new cell.
    // TODO - if (cell == NUGGET) {
        // TODO - nuggets = levelPlayer->runnerGotGold (spriteId, gridI, gridJ, true);
    // TODO - }

    // TODO - If arrived in USEDHOLE (as "reserved" above), start trapTime.

    // TODO - Call rules->findBestWay(), but where is the hero?
    Direction dirn = levelPlayer->getEnemyDirection (gridI, gridJ);
    interval = runTime;

    Flags OK  = grid->enemyMoves (gridI, gridJ);
    bool canStand = OK & dFlag [STAND];
    kDebug() << "Enemy" << spriteId << "Direction" << dirn <<
                "Flags" << OK << "at" << gridI << gridJ;

    AnimationType anim = aType [dirn];
    if (OK & dFlag [dirn]) {
        if ((dirn == DOWN) && (! canStand)) {
            anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
            interval = fallTime;
        }
    }
    else if (canStand) {
        dirn = STAND;
    }
    else {
        dirn = DOWN;
        anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
        interval = fallTime;
    }

    if (dirn == STAND) {
        anim = currAnimation;
    }

    timeLeft += interval;
    // kDebug() << "3: Hero interval is:" << interval << "time left:" << timeLeft;

    // TODO - Check for collision with an enemy somewhere around here.

    if (((anim == RUN_R) || (anim == RUN_L)) && (cell == POLE)) {
        anim = (dirn == RIGHT) ? CLIMB_R : CLIMB_L;
    }

    if ((dirn == currDirection) && (anim == currAnimation)) {
        // TODO - If dirn == STAND, no animation, else emit resynchAnimation().
        if (dirn == STAND) {
            return;
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
    return;
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
