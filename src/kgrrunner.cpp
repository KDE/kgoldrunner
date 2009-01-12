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
#include "kgrlevelgrid.h"
#include "kgrrulebook.h"

KGrRunner::KGrRunner (KGrLevelGrid * pGrid, int i, int j, KGrRuleBook * pRules)
    :
    QObject (pGrid),	// Destroy hero/enemy when the level-grid is destroyed.
    grid    (pGrid),
    rules   (pRules)
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
    kDebug() << "pointsPerCell" << pointsPerCell << "turnAnywhere" << turnAnywhere;
}

void KGrRunner::getLocation (int & row, int & col)
{
    row = gridX / pointsPerCell;
    col = gridY / pointsPerCell;
}


KGrNewHero::KGrNewHero (KGrLevelGrid * pGrid, int i, int j, KGrRuleBook * pRules)
    :
    KGrRunner (pGrid, i, j, pRules)
{
    kDebug() << "THE HERO IS BORN at" << i << j;
    emit startAnimation (0, gridX / pointsPerCell, gridY / pointsPerCell,
                         0, STAND, FALL);
}

KGrNewHero::~KGrNewHero()
{
}

void KGrNewHero::run (Direction dirn)
{
    char OK = grid->heroMoves (gridX / pointsPerCell, gridY / pointsPerCell);
    if (! (OK & dirn)) {
        dirn = (OK & STAND) ? STAND : DOWN;
    }

    AnimationType anim = FALL;
    switch (dirn) {
    case DOWN:
        gridY++;
        anim = CLIMB;
        break;
    case UP:
        gridY--;
        anim = CLIMB;
        break;
    case RIGHT:
        gridX++;
        anim = RUN;
        break;
    case LEFT:
        gridX--;
        anim = RUN;
        break;
    case STAND:
        break;
    }
    emit startAnimation (0, gridX / pointsPerCell, gridY / pointsPerCell,
                         0, dirn, anim);
}


KGrNewEnemy::KGrNewEnemy (KGrLevelGrid * pGrid, int i, int j, int id, KGrRuleBook * pRules)
    :
    KGrRunner (pGrid, i, j, pRules)
{
    kDebug() << "ENEMY" << id << "IS BORN at" << i << j;
    emit startAnimation (id + 1, gridX / pointsPerCell, gridY / pointsPerCell,
                         0, STAND, FALL);
}

KGrNewEnemy::~KGrNewEnemy()
{
}

#include "kgrrunner.moc"
