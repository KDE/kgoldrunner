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
               int i, int j, KGrRuleBook  * pRules);
    virtual ~KGrRunner();

    // void getLocation (int & row, int & col); TODO - Remove this.

protected:
    KGrLevelPlayer * levelPlayer;
    KGrLevelGrid *   grid;
    KGrRuleBook *    rules;

    int              gridI;
    int              gridJ;
    Vector2D         vector;
    int              pointCtr;

    int              pointsPerCell;
    bool             turnAnywhere;

    virtual void     getRules();
    // virtual void     setDirection (Direction dirn); // TODO = 0; Enemy's setDirection() is defined as?

    Direction        currDirection;
    AnimationType    currAnimation;

private:
};


class KGrHero : public KGrRunner
{
    Q_OBJECT
public:
    KGrHero (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                int i, int j, KGrRuleBook  * pRules);
    ~KGrHero();

    // void setDirection (Direction dirn);

    void run();

// signals:
    // void moveHero (int i, int j, int frame); // OBSOLESCENT? - 9/1/09

signals:
    void startAnimation   (const int id, const int row, const int col,
                           const int time,
                           const Direction dirn, const AnimationType type);

private:
};


class KGrEnemy : public KGrRunner
{
    Q_OBJECT
public:
    KGrEnemy (KGrLevelPlayer * pLevelPlayer, KGrLevelGrid * pGrid,
                 int i, int j, int id, KGrRuleBook  * pRules);
    ~KGrEnemy();

signals:
    void startAnimation   (const int id, const int row, const int col,
                           const int time,
                           const Direction dirn, const AnimationType type);

private:
};

#endif // KGRRUNNER_H
