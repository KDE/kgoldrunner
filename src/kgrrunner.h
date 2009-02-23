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

#ifndef KGRRUNNER_H
#define KGRRUNNER_H

#include <QObject>

#include "kgrconsts.h" // OBSOLESCENT - 1/1/09
#include "kgrglobals.h"

class KGrLevelPlayer;
class KGrLevelGrid;
class KGrRuleBook;

class KGrRunner : public QObject
{
    Q_OBJECT
public:
    KGrRunner (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
               int i, int j, int pSpriteId, KGrRuleBook  * pRules);
    virtual ~KGrRunner();

    inline int whereAreYou (int & i, int & j, int & point) {
                            i = gridI; j = gridJ; point = pointCtr;
                            return pointsPerCell; }

signals:
    void startAnimation    (const int spriteId, const bool repeating,
                            const int i, const int j, const int time,
                            const Direction dirn, const AnimationType type);

protected:
    KGrLevelPlayer * levelPlayer;
    KGrLevelGrid *   grid;
    KGrRuleBook *    rules;

    int              gridI;
    int              gridJ;
    int              spriteId;
    Vector2D         vector;
    int              pointCtr;

    int              pointsPerCell;
    bool             turnAnywhere;

    void             getRules();

    Direction        currDirection;
    AnimationType    currAnimation;

    int              interval;		// The runner's current time interval.
    int              timeLeft;		// Time till the runner's next action.
};


class KGrHero : public KGrRunner
{
    Q_OBJECT
public:
    KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                int i, int j, int pSpriteId, KGrRuleBook  * pRules);
    ~KGrHero();

    HeroStatus run (const int scaledTime);

    bool dig (const Direction dirn, int & digI, int & digJ);

    void showState (char option);

private:
    int              nuggets;		// Number of gold pieces remaining.

    static int       runTime;		// The time interval for running.
    static int       fallTime;		// The time interval for falling.
};


class KGrEnemy : public KGrRunner
{
    Q_OBJECT
public:
    KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                 int i, int j, int pSpriteId, KGrRuleBook  * pRules);
    ~KGrEnemy();

    void run (const int scaledTime);

    void showState (char option);

private:
    int              nuggets;		// Number of gold pieces an enemy holds.

    static int       runTime;		// The time interval for running.
    static int       fallTime;		// The time interval for falling.
    static int       trapTime;		// Time interval for which an enemy can
					// stay trapped in a brick.
};

#endif // KGRRUNNER_H
