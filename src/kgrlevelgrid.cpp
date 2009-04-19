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

#include <stdio.h> // OBSOLESCENT - 6/1/09 - Used for testing.
#include "kgrlevelgrid.h"

KGrLevelGrid::KGrLevelGrid (QObject * parent, const KGrLevelData & theLevelData)
    :
    QObject     (parent)
{
    // Put a concrete wall all round the layout: left, right, top and bottom.
    // This saves ever having to test for being at the edge of the layout.
    int inWidth      = theLevelData.width;
    width            = inWidth + ConcreteWall * 2;

    int inHeight     = theLevelData.height;
    height           = inHeight + ConcreteWall * 2;

    int size         = width * height;

    layout.fill      (CONCRETE, size);

    // Initialise the flags for each cell.
    heroAccess.fill  (0, size);
    enemyAccess.fill (0, size);
    enemyHere.fill   (-1, size);

    // Copy the cells of the layout, but enclosed within the concrete wall.
    int inRow  = 0;
    int outRow = width + ConcreteWall;

    for (int j = 0; j < inHeight; j++) {
        for (int i = 0; i < inWidth; i++) {
            char type = theLevelData.layout [inRow + i];
            switch (type) {
            case HLADDER:
                // Change hidden ladders to FREE, but keep a list of them.
                hiddenLadders.append (outRow + i);
                type = FREE;
                break;
            case HENEMY:
                // Change hidden enemies to BRICK, but keep a list of them.
                hiddenEnemies.append (outRow + i);
                type = BRICK;
                break;
            case FLASHING:
                // Change flashing nuggets to NUGGET, but keep a list of them.
                flashingGold.append (outRow + i);
                type = NUGGET;
                break;
            }
            layout [outRow + i] = type;
        }
        inRow  = inRow  + inWidth;
        outRow = outRow + width;
    }
}

KGrLevelGrid::~KGrLevelGrid()
{
}

// Inline functions (see kgrlevelgrid.h).
//     char cellType   (int i, int j)
//     char heroMoves  (int i, int j)
//     char enemyMoves (int i, int j)
//     void gotGold    (const int i, const int j, const bool runnerHasGold)
//
//     void setEnemyOccupied (int i, int j, const int spriteId)
//     int enemyOccupied (int i, int j)
//
//     int  index      (int i, int j)

void KGrLevelGrid::calculateAccess (bool pRunThruHole)
{
    runThruHole = pRunThruHole;		// Save a copy of the runThruHole rule.

    char here;
    bool canEnter;

    // Calculate which cells can be entered (N.B. USEDHOLE is a trapped enemy).
    for (int j = 1; j < height - 1; j++) {
        for (int i = 1; i < width - 1; i++) {
            here      = cellType (i, j);
            canEnter  = (here != BRICK)  && (here != CONCRETE) &&
                        (here != FBRICK) && (here != USEDHOLE);
            heroAccess  [index (i, j)] = canEnter ? ENTERABLE : 0;
            enemyAccess [index (i, j)] = canEnter ? ENTERABLE : 0;
        }
    }

    // Calculate the access *from* each cell to its neighbours.
    for (int j = 1; j < height - 1; j++) {
        for (int i = 1; i < width - 1; i++) {
            calculateCellAccess (i, j);
        }
    }
}

void KGrLevelGrid::changeCellAt (const int i, const int j, const char type)
{
    int  position          = index (i, j);
    bool canEnter          = (type != BRICK) && (type != CONCRETE) &&
                             (type != FBRICK) && (type != USEDHOLE);
    layout      [position] = type;
    heroAccess  [position] = canEnter ? ENTERABLE : 0;
    enemyAccess [position] = canEnter ? ENTERABLE : 0;

    calculateCellAccess (i, j);		// Recalculate access *from* this cell
					// and access *to* it
    calculateCellAccess (i, j - 1);	// from above,
    calculateCellAccess (i - 1, j);	// from left,
    calculateCellAccess (i + 1, j);	// from right,
    calculateCellAccess (i, j + 1);	// and from below.
}

void KGrLevelGrid::calculateCellAccess (const int i, const int j)
{
    Flags access = 0;
    char  here   = cellType (i, j);
    // TODO - Maybe we can just RETURN if this cell is CONCRETE.  Always works?
    if (here == CONCRETE) {
        // If edge-cell or other CONCRETE, no calculation (avoid index errors).
        return;
    }
    char  below  = cellType (i, j + 1);

    access = heroMoves (i, j) & ENTERABLE;
    // fprintf (stderr, "[%02d,%02d] %c access %02x below %c\n",
	     // i, j, here, access, below);

    // Cannot enter brick, concrete or used hole: can drop into a false brick.
    if (! (access & ENTERABLE) && (here != FBRICK)) {
	access = 0;
    }
    // If can stand or hang on anything, allow down, left and right.
    else if ((below == BRICK) || (below == CONCRETE) || (below == USEDHOLE) ||
	(below == LADDER) || (here == LADDER) || (here == BAR)) {
        // fprintf (stderr, "Can stand\n");
	access |= (dFlag [STAND] | dFlag [DOWN] |
		   dFlag [LEFT]  | dFlag [RIGHT]);
    }
    // If cannot stand or hang, can go down (space or false brick) or
    // maybe left or right (when standing on an enemy).
    else {
        // fprintf (stderr, "Cannot stand\n");
	access |= (dFlag [DOWN] | dFlag [LEFT]  | dFlag [RIGHT]);
    }
    // Can only go up if there is a ladder here.
    if (here == LADDER) {
	access |= dFlag [UP];
    }

    int enter = (access & ENTERABLE)         ? 1 : 0;
    int stand = (access & dFlag [STAND])     ? 1 : 0;
    int u     = (access & dFlag [UP])        ? 1 : 0;
    int d     = (access & dFlag [DOWN])      ? 1 : 0;
    int l     = (access & dFlag [LEFT])      ? 1 : 0;
    int r     = (access & dFlag [RIGHT])     ? 1 : 0;
    // if (below == HOLE)
    // fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d ***\n",
	     // i, j, here, access, enter, stand, u, d, l, r);

    // Mask out directions that are blocked above, below, L or R, but not for
    // concrete/brick at edge of grid (or elsewhere) to avoid indexing errors.
    if (access != 0) {
        if (! (heroMoves (i, j - 1) & ENTERABLE)) {
            access = ~dFlag [UP] & access;		// Cannot go up.
        }
        if (! (heroMoves (i - 1, j) & ENTERABLE)) {
            access = ~dFlag [LEFT] & access;		// Cannot go left.
        }
        if (! (heroMoves (i + 1, j) & ENTERABLE)) {
            access = ~dFlag [RIGHT] & access;		// Cannot go right.
        }
        if (! (heroMoves (i, j + 1) & ENTERABLE)) {
            if (below != FBRICK) {
                access = ~dFlag [DOWN] & access;	// Cannot go down.
            }
        }
    }

    enter = (access & ENTERABLE)         ? 1 : 0;
    stand = (access & dFlag [STAND])     ? 1 : 0;
    u     = (access & dFlag [UP])        ? 1 : 0;
    d     = (access & dFlag [DOWN])      ? 1 : 0;
    l     = (access & dFlag [LEFT])      ? 1 : 0;
    r     = (access & dFlag [RIGHT])     ? 1 : 0;
    // if (below == HOLE)
    // fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d ***\n",
	     // i, j, here, access, enter, stand, u, d, l, r);
    heroAccess [index (i, j)]  = access;
    enter = (access & ENTERABLE)         ? 1 : 0;
    stand = (access & dFlag [STAND])     ? 1 : 0;
    u     = (access & dFlag [UP])        ? 1 : 0;
    d     = (access & dFlag [DOWN])      ? 1 : 0;
    l     = (access & dFlag [LEFT])      ? 1 : 0;
    r     = (access & dFlag [RIGHT])     ? 1 : 0;
    // fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d\n",
	     // i, j, here, access, enter, stand, u, d, l, r);
    
    // Enemy access is the same as the hero's when no holes are open.
    enemyAccess [index (i, j)] = heroAccess [index (i, j)];

    if (here == USEDHOLE) {
        enemyAccess [index (i, j)] = UP;	// Can only climb out of hole.
    }
    else if (! runThruHole) {			// Check the rule.
        char mask;
        mask = (cellType (i - 1, j) == HOLE) ? dFlag [LEFT] : 0;
        mask = (cellType (i + 1, j) == HOLE) ? (dFlag [RIGHT] | mask) : mask;
        enemyAccess [index (i, j)] &= ~mask;	// Block access to holes at L/R.
    }
    // TODO - Remove the debugging code below.
    access = enemyAccess [index (i, j)];
    enter = (access & ENTERABLE)         ? 1 : 0;
    stand = (access & dFlag [STAND])     ? 1 : 0;
    u     = (access & dFlag [UP])        ? 1 : 0;
    d     = (access & dFlag [DOWN])      ? 1 : 0;
    l     = (access & dFlag [LEFT])      ? 1 : 0;
    r     = (access & dFlag [RIGHT])     ? 1 : 0;
    // fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d Enem\n",
	     // i, j, here, access, enter, stand, u, d, l, r);
}

void KGrLevelGrid::placeHiddenLadders()
{
    int offset, i, j;
    dbe3 "KGrLevelGrid::placeHiddenLadders() %02d width %02d\n",
                     hiddenLadders.count(), width);

    foreach (offset, hiddenLadders) {
        i = offset % width;
        j = offset / width;
        changeCellAt (i, j, LADDER);
        dbe3 "Show ladder at %04d [%02d,%02d]\n", offset, i, j);
    }
    emit showHiddenLadders (hiddenLadders, width);
    hiddenLadders.clear();
}

#include "kgrlevelgrid.moc"
