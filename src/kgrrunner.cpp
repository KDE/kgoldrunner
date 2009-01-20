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

#include "kgrrunner.h"
#include "kgrlevelplayer.h"
#include "kgrlevelgrid.h"
#include "kgrrulebook.h"

KGrRunner::KGrRunner (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                      int i, int j, KGrRuleBook * pRules)
    :
    QObject     (pLevelPlayer),	// Destroy runner when level is destroyed.
    levelPlayer (pLevelPlayer),
    grid        (pGrid),
    rules       (pRules),
    gridI       (i),
    gridJ       (j),

    currDirection (STAND),
    currAnimation (FALL_L)
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

// void KGrRunner::getLocation (int & row, int & col) TODO - Remove this.
// {
    // if (pointCtr < pointsPerCell) {
        // row = gridI;
        // col = gridJ;
    // }
    // else {
        // row = gridI + vector [X];
        // col = gridJ + vector [Y];
    // }
// }


KGrHero::KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                  int i, int j, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pRules)
{
    kDebug() << "THE HERO IS BORN at" << i << j;
}

KGrHero::~KGrHero()
{
}

// void KGrHero::setDirection (Direction dirn) TODO - Remove this.
// {
    // nextDirection = dirn;
// }

void KGrHero::run()
{
    if (pointCtr < pointsPerCell) {
        pointCtr++;
        return;
    }
    // TODO - Use nextDirection here, and set currDirection (at end?).
    // TODO - Get KGrLevelPlayer to call setDirection, based on mouse position.
    pointCtr = 0;
    gridI    = gridI + vector [X];
    gridJ    = gridJ + vector [Y];

    Direction dirn = levelPlayer->getDirection (gridI, gridJ);

    Flags OK  = grid->heroMoves (gridI, gridJ);
    char cell = grid->cellType  (gridI, gridJ);
    bool canStand = OK & dFlag [STAND];
    kDebug() << "Direction" << dirn << "Flags" << OK << "at" << gridI << gridJ;

    AnimationType anim = aType [dirn];
    if (OK & dFlag [dirn]) {
        if ((dirn == DOWN) && (! canStand)) {
            anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
        }
    }
    else if (canStand) {
        dirn = STAND;
    }
    else {
        dirn = DOWN;
        anim = (currDirection == RIGHT) ? FALL_R : FALL_L;
    }

    if (dirn == STAND) {
        anim = currAnimation;
    }

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

    kDebug() << "New direction" << dirn << vector [X] << vector [Y] << gridI << gridJ;
    kDebug() << "Sprite" << 0 << "Animate" << gridI << gridJ << "time" << 0 << "dirn" << dirn << "anim" << anim;

    emit startAnimation (0, gridI, gridJ, 0, dirn, anim);
    currAnimation = anim;
    currDirection = dirn;
}


KGrEnemy::KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                    int i, int j, int id, KGrRuleBook * pRules)
    :
    KGrRunner (pLevelPlayer, pGrid, i, j, pRules)
{
    kDebug() << "ENEMY" << id << "IS BORN at" << i << j;
}

KGrEnemy::~KGrEnemy()
{
}

#include "kgrrunner.moc"
