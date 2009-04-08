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

#include "kgrlevelgrid.h"
#include "kgrrulebook.h"

KGrRuleBook::KGrRuleBook (QObject * parent)
    :
    QObject              (parent),
    mVariableTiming      (true),
    mAlwaysCollectNugget (true),
    mRunThruHole         (true),
    mReappearAtTop       (true),
    mReappearRow         (2),
    mPointsPerCell       (4),
    mTurnAnywhere        (false)
{
}

KGrRuleBook::~KGrRuleBook()
{
}

void KGrRuleBook::setTiming (const int enemyCount)
{
    int choice;
    Timing varTiming[6] = {
                          {40, 58, 78, 88, 170, 23},      // No enemies.
                          {50, 68, 78, 88, 170, 32},      // 1 enemy.
                          {57, 67, 114, 128, 270, 37},    // 2 enemies.
                          {60, 70, 134, 136, 330, 40},    // 3 enemies.
                          {63, 76, 165, 150, 400, 46},    // 4 enemies.
                          {70, 80, 189, 165, 460, 51}     // >4 enemies.
                          };
    if (mVariableTiming) {
        choice = (enemyCount < 0) ? 0 : enemyCount;
        choice = (enemyCount > 5) ? 5 : enemyCount;
        times  = varTiming [choice];
    }
}


// Initialise the flags for the rules.

KGrTraditionalRules::KGrTraditionalRules (QObject * parent)
    :
    KGrRuleBook (parent)
{
    mRules               = TraditionalRules;

    mVariableTiming      = true;	///< More enemies imply less speed.
    mAlwaysCollectNugget = true;	///< Enemies always collect nuggets.
    mRunThruHole         = true;	///< Enemy can run L/R through dug hole.
    mReappearAtTop       = true;	///< Enemies reborn at top of screen.
    mReappearRow         = 2;		///< Row where enemies reappear.
    mPointsPerCell       = 4;		///< Number of points in each grid-cell.
    mTurnAnywhere        = false;	///< Change direction only at next cell.
    mEnemiesShowGold     = true;	///< Show enemies carrying gold.

    Timing t = {40, 58, 78, 88, 170, 23};
    times = t;
}

KGrTraditionalRules::~KGrTraditionalRules()
{
}

Direction KGrTraditionalRules::findBestWay (const int eI, const int eJ,
                                            const int hI, const int hJ,
                                            KGrLevelGrid * pGrid,
                                            bool /* leftRightSearch unused */)
// TODO - Should be const ...               const KGrLevelGrid * pGrid)
{
    grid = pGrid;
    if (grid->cellType (eI, eJ) == USEDHOLE) {	// Could not get out of hole
        return UP;				// (e.g. brick above is closed):
    }						// but keep trying.

    // TODO - Add !standOnEnemy() && as a condition here.
    // TODO - And maybe !((*playfield)[x][y+1]->whatIam() == HOLE)) not just out of hole,
    bool canStand = (grid->enemyMoves (eI, eJ) & dFlag [STAND]) ||
                    (grid->enemyOccupied (eI, eJ + 1) > 0);
    if (! canStand) {
        dbk2 << "can stand" << (grid->enemyMoves (eI, eJ) & dFlag [STAND])
                 << "occ-below" << grid->enemyOccupied (eI, eJ + 1);
        return DOWN;
    }
    dbk2 << "not DOWN yet";

    // Traditional search strategy.
    Direction dirn = STAND;
    if (eJ == hJ) {
        dirn = getHero (eI, eJ, hI);		// Hero on same row.
        if (dirn != STAND) {
            return dirn;			// Can go towards him.
        }
    }

    if (eJ >= hJ) {				// Hero above or on same row.
        dirn = searchUp (eI, eJ, hJ);		// Find a way that leads up.
    }
    else {					// Hero below enemy.
        dirn = searchDown (eI, eJ, hJ);		// Find way that leads down.
        dbk2 << "searchDown1" << eI << eJ << hJ << "ret" << dirn;
        if (dirn == STAND) {
            dirn = searchUp (eI, eJ, hJ);	// No go: try going up first.
        }
    }

    if (dirn == STAND) {			// When all else fails, look for
        dirn = searchDown (eI, eJ, eJ - 1);	// a way below the hero.
        dbk2 << "searchDown2" << eI << eJ << (eJ - 1) << "ret" << dirn;
    }

    return dirn;
}

Direction KGrTraditionalRules::searchUp (int ew, int eh, int hh)
{
    int i, ilen, ipos, j, jlen, jpos, deltah, rungs;

    deltah = eh - hh;			// Get distance up to hero's level.

    // Search for the best ladder right here or on the left.
    i = ew; ilen = 0; ipos = -1;
    while (i >= 1) {
        rungs = distanceUp (i, eh, deltah);
        if (rungs > ilen) {
            ilen = rungs;		// This the best yet.
            ipos = i;
        }
        if (searchOK (-1, i, eh))
            i--;			// Look further to the left.
        else
            i = -1;			// Cannot go any further to the left.
    }

    // Search for the best ladder on the right.
    j = ew; jlen = 0; jpos = -1;
    while (j < FIELDWIDTH) {
        if (searchOK (+1, j, eh)) {
            j++;			// Look further to the right.
            rungs = distanceUp (j, eh, deltah);
            if (rungs > jlen) {
                jlen = rungs;		// This the best yet.
                jpos = j;
            }
        }
        else
            j = FIELDWIDTH+1;		// Cannot go any further to the right.
    }

    if ((ilen == 0) && (jlen == 0))	// No ladder found.
        return STAND;

    // Choose a ladder to go to.
    if (ilen != jlen) {			// If the ladders are not the same
                                        // length, choose the longer one.
        if (ilen > jlen) {
            if (ipos == ew)		// If already on the best ladder, go up.
                return UP;
            else
                return LEFT;
        }
        else
            return RIGHT;
    }
    else {				// Both ladders are the same length.

        if (ipos == ew)			// If already on the best ladder, go up.
            return UP;
        else if (ilen == deltah) {	// If both reach the hero's level,
            if ((ew - ipos) <= (jpos - ew)) // choose the closest.
                return LEFT;
            else
                return RIGHT;
        }
        else return LEFT;		// Else choose the left ladder.
    }
}

Direction KGrTraditionalRules::searchDown (int ew, int eh, int hh)
{
    int i, ilen, ipos, j, jlen, jpos, deltah, rungs, path;

    deltah = hh - eh;			// Get distance down to hero's level.

    // Search for the best way down, right here or on the left.
    ilen = 0; ipos = -1;
    i = (willNotFall (ew, eh)) ? ew : -1;
    rungs = distanceDown (ew, eh, deltah);
    if (rungs > 0) {
        ilen = rungs; ipos = ew;
    }

    while (i >= 1) {
        rungs = distanceDown (i - 1, eh, deltah);
        if (((rungs > 0) && (ilen == 0)) ||
            ((deltah > 0) && (rungs > ilen)) ||
            ((deltah <= 0) && (rungs < ilen) && (rungs != 0))) {
            ilen = rungs;		// This the best way yet.
            ipos = i - 1;
        }
        if (searchOK (-1, i, eh))
            i--;			// Look further to the left.
        else
            i = -1;			// Cannot go any further to the left.
    }

    // Search for the best way down, on the right.
    j = ew; jlen = 0; jpos = -1;
    while (j < FIELDWIDTH) {
        rungs = distanceDown (j + 1, eh, deltah);
        if (((rungs > 0) && (jlen == 0)) ||
            ((deltah > 0) && (rungs > jlen)) ||
            ((deltah <= 0) && (rungs < jlen) && (rungs != 0))) {
            jlen = rungs;		// This the best way yet.
            jpos = j + 1;
        }
        if (searchOK (+1, j, eh)) {
            j++;			// Look further to the right.
        }
        else
            j = FIELDWIDTH+1;		// Cannot go any further to the right.
    }

    if ((ilen == 0) && (jlen == 0))	// Found no way down.
        return STAND;

    // Choose a way down to follow.
    if (ilen == 0)
        path = jpos;
    else if (jlen == 0)
        path = ipos;
    else if (ilen != jlen) {		// If the ways down are not same length,
                                        // choose closest to hero's level.
        if (deltah > 0) {
            if (jlen > ilen)
                path = jpos;
            else
                path = ipos;
        }
        else {
            if (jlen > ilen)
                path = ipos;
            else
                path = jpos;
        }
    }
    else {				// Both ways down are the same length.
        if ((deltah > 0) &&		// If both reach the hero's level,
            (ilen == deltah)) {		// choose the closest.
            if ((ew - ipos) <= (jpos - ew))
                path = ipos;
            else
                path = jpos;
        }
        else
            path = ipos;		// Else, go left or down.
    }

    if (path == ew)
        return DOWN;
    else if (path < ew)
        return LEFT;
    else
        return RIGHT;
}

Direction KGrTraditionalRules::getHero (int eI, int eJ, int hI)
{
    int i, inc, returnValue;

    inc = (eI > hI) ? -1 : +1;
    i = eI;
    while (i != hI) {
        returnValue = canWalkLR (inc, i, eJ);
        if (returnValue > 0)
            i = i + inc;		// Can run further towards the hero.
        else if (returnValue < 0)
            break;			// Will run into a wall regardless.
        else
            return STAND;		// Won't run over a hole.
    }

    if (i < eI)		return LEFT;
    else if (i > eI)	return RIGHT;
    else		return STAND;
}

int KGrTraditionalRules::distanceUp (int x, int y, int deltah)
{
    int rungs = 0;

    // If there is a ladder at (x,y), return its length, else return zero.
    while (grid->cellType (x, y - rungs) == LADDER) {
        rungs++;
        if (rungs >= deltah) {		// To hero's level is enough.
            break;
        }
    }
    return rungs;
}

int KGrTraditionalRules::distanceDown (int x, int y, int deltah)
{
    // When deltah > 0, we want an exit sideways at the hero's level or above.
    // When deltah <= 0, we can go down any distance (as a last resort).

    int rungs = -1;
    int exitRung = 0;
    bool canGoThru = true;
    char objType;

    // If there is a way down at (x,y), return its length, else return zero.
    // Because rungs == -1, we first check that level y is not blocked here.
    while (canGoThru) {
        objType = grid->cellType (x, y + rungs + 1);
        switch (objType) {
        case BRICK:
        case CONCRETE:
        case HOLE:			// Enemy cannot go DOWN through a hole.
        case USEDHOLE:
            if ((deltah > 0) && (rungs <= deltah))
                exitRung = rungs;
            if ((objType == HOLE) && (rungs < 0))
                rungs = 0;		// Enemy can go SIDEWAYS through a hole.
            else
                canGoThru = false;	// Cannot go through here.
            break;
        case LADDER:
        case BAR:			// Can go through or stop.
            rungs++;			// Add to path length.
            if ((deltah > 0) && (rungs >= 0)) {
                // If at or above hero's level, check for an exit from ladder.
                if ((rungs - 1) <= deltah) {
                    // Maybe can stand on top of ladder and exit L or R.
                    if ((objType == LADDER) && (searchOK (-1, x, y+rungs-1) ||
                                                searchOK (+1, x, y+rungs-1)))
                        exitRung = rungs - 1;
                    // Maybe can exit L or R from ladder body or pole.
                    if ((rungs <= deltah) && (searchOK (-1, x, y+rungs) ||
                                              searchOK (+1, x, y+rungs)))
                        exitRung = rungs;
                }
                else
                    canGoThru = false;	// Should stop at hero's level.
            }
            break;
        default:
            rungs++;			// Can go through.  Add to path length.
            break;
        }
    }
    if (rungs == 1) {
        // TODO - Check for another enemy in this space, maybe in KGrRunner.
        // TODO - Maybe the presence of another enemy below could be a bool
        // TODO - parameter for findBestWay() ...  NO, that won't work, we
        // TODO - need to know if there is an enemy ANYWHERE under a LR path.
        // QListIterator<KGrEnemy *> i (*enemies);
        // while (i.hasNext()) {
            // KGrEnemy * enemy = i.next();
            // if ((x*16==enemy->getx()) && (y*16+16==enemy->gety()))
                // rungs = 0;		// Pit is blocked.  Find another way.
        // }
        if (grid->enemyOccupied (x, y + 1) > 0) {
            dbk2 << "Pit block =" << grid->enemyOccupied (x, y + 1)
                     << "at" << x << (y + 1);
            rungs = 0;		// Pit is blocked.  Find another way.
        }
    }
    if (rungs <= 0)
        return 0;			// There is no way down.
    else if (deltah > 0)
        return exitRung;		// We want to take an exit, if any.
    else
        return rungs;			// We can go down all the way.
}

bool KGrTraditionalRules::searchOK (int direction, int x, int y)
{
    // Check whether it is OK to search left or right.
    if (canWalkLR (direction, x, y) > 0) {
        // Can go into that cell, but check for a fall.
        if (willNotFall (x+direction, y))
            return true;
    }
    return false;			// Cannot go into and through that cell.
}

int KGrTraditionalRules::canWalkLR (int direction, int x, int y)
{
    if (willNotFall (x, y)) {
        switch (grid->cellType (x+direction, y)) {
        case CONCRETE:
        case BRICK:
        case USEDHOLE:
            return -1;		// Will be halted in current cell.
            break;
        default:
            // NB. FREE, LADDER, HLADDER, NUGGET and BAR are OK of course,
            //     but enemies can also walk left/right through a HOLE and
            //     THINK they can walk left/right through a FBRICK.

            return +1;		// Can walk into next cell.
            break;
        }
    }
    else
        return 0;			// Will fall before getting there.
}

bool KGrTraditionalRules::willNotFall (int x, int y)
{
    // TODO - Remove ... int c, cmax;
    // TODO - Remove ... KGrEnemy *enemy;

    // Check the ceiling.
    switch (grid->cellType (x, y)) {
    case LADDER:
    case BAR:
        return true; break;		// OK, can hang on ladder or pole.
    default:
        break;
    }

    // Check the floor.
    switch (grid->cellType (x, y + 1)) {

    // Cases where the enemy knows he will fall.
    case FREE:
    case HLADDER:
    case FBRICK:

    // N.B. The enemy THINKS he can run over a NUGGET, a buried BAR or a HOLE.
    // The last of these cases allows the hero to trap the enemy, of course.

    // Note that there are several Traditional levels that require an enemy to
    // be trapped permanently in a pit containing a nugget, as he runs towards
    // you.  It is also possible to use a buried BAR in the same way.

        // TODO - Check for another enemy down below, maybe in KGrRunner.
        // TODO - Maybe the presence of another enemy below could be a bool
        // TODO - parameter for findBestWay() ...  NO, that won't work, we
        // TODO - need to know if there is an enemy ANYWHERE under a LR path.
        if (grid->enemyOccupied (x, y + 1) > 0) {
            dbk2 << "Occupied =" << grid->enemyOccupied (x, y + 1)
                     << "at" << x << (y + 1);
            return true;
        }
        // cmax = enemies->count();
        // for (c = 0; c < cmax; c++) {
            // enemy = enemies->at (c);
            // if ((enemy->getx()==16*x) && (enemy->gety()==16*(y+1)))
                // return true;		// Standing on a friend.
        // }
        return false;			// Will fall: there is no floor.
        break;

    default:
        return true;			// OK, will not fall.
        break;
    }
}


KGrKGoldrunnerRules::KGrKGoldrunnerRules (QObject * parent)
    :
    KGrRuleBook     (parent)
{
    mRules               = KGoldrunnerRules;

    mVariableTiming      = false;	///< Speed same for any no. of enemies.
    mAlwaysCollectNugget = false;	///< Enemies sometimes collect nuggets.
    mRunThruHole         = false;	///< Enemy cannot run through dug hole.
    mReappearAtTop       = false;	///< Enemies reborn at start position.
    mReappearRow         = -1;		///< Row where enemies reappear (N/A).
    mPointsPerCell       = 4;		///< Number of points in each grid-cell.
    mTurnAnywhere        = false;	///< Change direction only at next cell.
    mEnemiesShowGold     = true;	///< Show enemies carrying gold.

    Timing t = {45, 50, 55, 100, 500, 40};
    times = t;
}

KGrKGoldrunnerRules::~KGrKGoldrunnerRules()
{
}

Direction KGrKGoldrunnerRules::findBestWay (const int eI, const int eJ,
                                            const int hI, const int hJ,
                                            KGrLevelGrid * pGrid,
                                            bool leftRightSearch)
// TODO - Should be const ...               const KGrLevelGrid * pGrid)
{
    dbk2 << eI << eJ << hI << hJ;
    grid = pGrid;

    if (grid->cellType (eI, eJ) == USEDHOLE) {	// Could not get out of hole
        return UP;				// (e.g. brick above is closed):
    }						// but keep trying.

    // TODO - Add !standOnEnemy() && as a condition here.
    // TODO - And maybe !((*playfield)[x][y+1]->whatIam() == HOLE)) not just out of hole,
    bool canStand = (grid->enemyMoves (eI, eJ) & dFlag [STAND]) ||
                    (grid->enemyOccupied (eI, eJ + 1) > 0);
    if (! canStand) {
        dbk2 << "can stand" << (grid->enemyMoves (eI, eJ) & dFlag [STAND])
                 << "occ-below" << grid->enemyOccupied (eI, eJ + 1);
        return DOWN;
    }

    // KGoldrunner search strategy.
    if (leftRightSearch) {
        if (eI > hI) {
            return findWayLeft  (eI, eJ);
        }
        if (eI < hI) {
            return findWayRight (eI, eJ);
        }
    }
    else {
        if (eJ > hJ) {
            return findWayUp    (eI, eJ);
        }
        if (eJ < hJ) {
            return findWayDown  (eI, eJ);
        }
    }

    return STAND;
}

Direction KGrKGoldrunnerRules::findWayUp (const int eI, const int eJ)
{
    int i, k;
    i = k = eI;
    if (grid->enemyMoves (eI, eJ) & dFlag [UP]) {
        return UP;			// Go up from current position.
    }
    else {
        while ((i >= 0) || (k <= FIELDWIDTH)) {
            if (i >= 0) {
                if (grid->enemyMoves (i, eJ) & dFlag [UP]) {
                    return LEFT;	// Go left, then up later.
                }
                else if (! (grid->enemyMoves (i--, eJ) & dFlag [LEFT])) {
                    i = -1;
                }
            }
            if (k <= FIELDWIDTH) {
                if (grid->enemyMoves (k, eJ) & dFlag [UP]) {
                    return RIGHT;	// Go right, then up later.
                }
                else if (!(grid->enemyMoves (k++, eJ) & dFlag [RIGHT])) {
                    k = FIELDWIDTH + 1;
                }
            }
        }
    }
    // BUG FIX - Ian W., 30/4/01 - Don't leave an enemy standing in mid air.
    if (false) // TODO - Was supposed to be canStand() - maybe redundant now -
               //        canStand() returned (firmGround || standOnEnemy()).
        return DOWN;
    else
        return STAND;
}

Direction KGrKGoldrunnerRules::findWayDown (const int eI, const int eJ)
{
    int i, k;
    i = k = eI;
    if (grid->enemyMoves (eI, eJ) & dFlag [DOWN]) {
        return DOWN;			// Go down from current position.
    }
    else {
        while ((i >= 0) || (k <= FIELDWIDTH)) {
            if (i >= 0) {
                if (grid->enemyMoves (i, eJ) & dFlag [DOWN]) {
                    return LEFT;	// Go left, then down later.
                }
                else if (! (grid->enemyMoves (i--, eJ) & dFlag [LEFT])) {
                    i = -1;
                }
            }
            if (k <= FIELDWIDTH) {
                if (grid->enemyMoves (k, eJ) & dFlag [DOWN]) {
                    return RIGHT;	// Go right, then down later.
                }
                else if (! (grid->enemyMoves (k++, eJ) & dFlag [RIGHT])) {
                    k = FIELDWIDTH + 1;
                }
            }
        }
    }
    return STAND;			// Cannot go down.
}

Direction KGrKGoldrunnerRules::findWayLeft (const int eI, const int eJ)
{
    int i, k;
    i = k = eJ;
    if (grid->enemyMoves (eI, eJ) & dFlag [LEFT]) {
        return LEFT;			// Go left from current position.
    }
    else {
        while ((i >= 0) || (k <= FIELDHEIGHT)) {
            if (i >= 0) {
                if (grid->enemyMoves (eI, i) & dFlag [LEFT]) {
                    return UP;		// Go up, then left later.
                }
                else if (! (grid->enemyMoves (eI, i--) & dFlag [UP])) {
                    i = -1;
                }
            }
            if (k <= FIELDHEIGHT) {
                if (grid->enemyMoves (eI, k) & dFlag [LEFT]) {
                    return DOWN;	// Go down, then left later.
                }
                else if (! (grid->enemyMoves (eI, k++) & dFlag [DOWN])) {
                    k = FIELDHEIGHT + 1;
                }
            }
        }
    }
    return STAND;			// Cannot go left.
}

Direction KGrKGoldrunnerRules::findWayRight (const int eI, const int eJ)
{
    int i, k;
    i = k = eJ;
    if (grid->enemyMoves (eI, eJ) & dFlag [RIGHT]) {
        return RIGHT;			// Go right from current position.
    }
    else {
        while ((i >= 0) || (k <= FIELDHEIGHT)) {
            if (i >= 0) {
                if (grid->enemyMoves (eI, i) & dFlag [RIGHT]) {
                    return UP;		// Go up, then right later.
                }
                else if (!(grid->enemyMoves (eI, i--) & dFlag [UP])) {
                    i = -1;
                }
            }
            if (k <= FIELDHEIGHT) {
                if (grid->enemyMoves (eI, k) & dFlag [RIGHT]) {
                    return DOWN;	// Go down, then right later.
                }
                else if (! (grid->enemyMoves (eI, k++) & dFlag [DOWN])) {
                    k = FIELDHEIGHT + 1;
                }
            }
        }
    }
    return STAND;			// Cannot go right.
}


KGrScavengerRules::KGrScavengerRules (QObject * parent)
    :
    KGrRuleBook (parent)
{
    mRules               = ScavengerRules;

    mVariableTiming      = false;	///< Speed same for any no. of enemies.
    mAlwaysCollectNugget = true;	///< Enemies always collect nuggets.
    mRunThruHole         = true;	///< Enemy can run L/R through dug hole.
    mReappearAtTop       = true;	///< Enemies reborn at top of screen.
    mReappearRow         = 1;		///< Row where enemies reappear.
    mPointsPerCell       = 12;		///< Number of points in each grid-cell.
    mTurnAnywhere        = true;	///< Change direction anywhere in cell.
    mEnemiesShowGold     = false;	///< Conceal enemies carrying gold.

    Timing t = {45, 50, 55, 100, 500, 40};
    times = t;
}

KGrScavengerRules::~KGrScavengerRules()
{
}

Direction KGrScavengerRules::findBestWay   (const int eI, const int eJ,
                                            const int hI, const int hJ,
                                            KGrLevelGrid * pGrid,
                                            bool /* leftRightSearch unused */)
// TODO - Should be const ...               const KGrLevelGrid * pGrid)
{
    dbk2 << eI << eJ << hI << hJ;
    grid = pGrid;
    return RIGHT;
}

#include "kgrrulebook.moc"
