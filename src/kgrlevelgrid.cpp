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

#include <stdio.h> // OBSOLESCENT - 6/1/09 - Used for testing.
#include "kgrlevelgrid.h"

KGrLevelGrid::KGrLevelGrid (QObject * parent, KGrLevelData * theLevelData)
    :
    QObject (parent)
{
    // Put a concrete wall all round the layout: left, right, top and bottom.
    // This saves ever having to test for being at the edge of the layout.
    int inWidth      = theLevelData->width;
    width            = inWidth + ConcreteWall * 2;

    int inHeight     = theLevelData->height;
    height           = inHeight + ConcreteWall * 2;

    int size         = width * height;

    layout.fill      (CONCRETE, size);

    // Initialise the flags for each cell.
    heroAccess.fill  (0, size);
    enemyAccess.fill (0, size);
    cellStates.fill  (0, size);

    // Copy the cells of the layout, but enclosed within the concrete wall.
    int inRow  = 0;
    int outRow = width + ConcreteWall;

    for (int j = 0; j < inHeight; j++) {
        for (int i = 0; i < inWidth; i++) {
            char type = theLevelData->layout [inRow + i];
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

    calculateAccess();
}

KGrLevelGrid::~KGrLevelGrid()
{
}

// Inline functions (see kgrlevelgrid.h).
//     char cellType   (int i, int j)
//     char heroMoves  (int i, int j)
//     char enemyMoves (int i, int j)
//     char cellState  (int i, int j)

void KGrLevelGrid::calculateAccess()
{
    char here;
    for (int j = 1; j < height - 1; j++) {
        for (int i = 1; i < width - 1; i++) {
            here  = cellType (i, j);
            if ((here != BRICK) && (here != CONCRETE) && (here != FBRICK)) {
                heroAccess  [index (i, j)] = ENTERABLE;
                enemyAccess [index (i, j)] = ENTERABLE;
            }
        }
    }

    Flags below;
    Flags access = 0;

    for (int j = 1; j < height - 1; j++) {
        for (int i = 1; i < width - 1; i++) {
            here   = cellType (i, j);
            below  = cellType (i, j + 1);

            access = heroMoves (i, j);

            // Cannot enter brick or concrete: can enter false brick from above.
            if (! (access & ENTERABLE) && (here != FBRICK)) {
                access = 0;
            }
            // If can stand or hang on anything, allow down, left and right.
            else if ((below == BRICK) || (below == CONCRETE) ||
                (below == LADDER) || (here == LADDER) || (here == POLE)) {
                access |= (dFlag [STAND] | dFlag [DOWN] |
                           dFlag [LEFT] |  dFlag [RIGHT]);
            }
            // If cannot stand or hang, can only go down (space or false brick).
            else {
                access |= dFlag [DOWN];
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
            if ((i == 14) && (j == 20)) 
            fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d ***\n",
                     i, j, here, access, enter, stand, u, d, l, r);

            // Mask out directions that are blocked above, below, L or R.
            if (! (heroMoves (i, j - 1) & ENTERABLE)) {
                access = ~dFlag [UP] & access;		// Cannot go up.
            }
            if (! (heroMoves (i - 1, j) & ENTERABLE)) {
                access = ~dFlag [LEFT] & access;	// Cannot go left.
            }
            if (! (heroMoves (i + 1, j) & ENTERABLE)) {
                access = ~dFlag [RIGHT] & access;	// Cannot go right.
            }
            if (! (heroMoves (i, j + 1) & ENTERABLE)) {
                if (below != FBRICK) {
                    access = ~dFlag [DOWN] & access;	// Cannot go down.
                }
            }

            enter = (access & ENTERABLE)         ? 1 : 0;
            stand = (access & dFlag [STAND])     ? 1 : 0;
            u     = (access & dFlag [UP])        ? 1 : 0;
            d     = (access & dFlag [DOWN])      ? 1 : 0;
            l     = (access & dFlag [LEFT])      ? 1 : 0;
            r     = (access & dFlag [RIGHT])     ? 1 : 0;
            if ((i == 14) && (j == 20)) 
            fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d ***\n",
                     i, j, here, access, enter, stand, u, d, l, r);
            heroAccess [index (i, j)]  |= access;
            access = heroAccess [index (i, j)];
            enter = (access & ENTERABLE)         ? 1 : 0;
            stand = (access & dFlag [STAND])     ? 1 : 0;
            u     = (access & dFlag [UP])        ? 1 : 0;
            d     = (access & dFlag [DOWN])      ? 1 : 0;
            l     = (access & dFlag [LEFT])      ? 1 : 0;
            r     = (access & dFlag [RIGHT])     ? 1 : 0;
            fprintf (stderr, "[%02d,%02d] %c %02x E %d S %d U %d D %d L %d R %d\n",
                     i, j, here, access, enter, stand, u, d, l, r);
            
            // Enemy access is the same as the hero's when no bricks are open.
            enemyAccess [index (i, j)] = heroAccess [index (i, j)];
        }
    }
}

#include "kgrlevelgrid.moc"
