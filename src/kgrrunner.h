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

class KGrLevelGrid;
class KGrRuleBook;

class KGrRunner : public QObject
{
    Q_OBJECT
public:
    KGrRunner (KGrLevelGrid * pGrid, int i, int j, KGrRuleBook  * pRules);
    virtual ~KGrRunner();

    void getLocation (int & row, int & col);

protected:
    KGrLevelGrid * grid;
    KGrRuleBook * rules;
    int  gridX;
    int  gridY;
    int  pointsPerCell;
    bool turnAnywhere;

    virtual void getRules();

private:
};


class KGrNewHero : public KGrRunner
{
    Q_OBJECT
public:
    KGrNewHero (KGrLevelGrid * pGrid, int i, int j, KGrRuleBook  * pRules);
    ~KGrNewHero();

    void run (Direction dirn);

// signals:
    // void moveHero (int i, int j, int frame); // OBSOLESCENT? - 9/1/09

signals:
    void startAnimation   (const int id, const int row, const int col,
                           const int time,
                           const Direction dirn, const AnimationType type);

private:
};


class KGrNewEnemy : public KGrRunner
{
    Q_OBJECT
public:
    KGrNewEnemy (KGrLevelGrid * pGrid, int i, int j, int id, KGrRuleBook  * pRules);
    ~KGrNewEnemy();

signals:
    void startAnimation   (const int id, const int row, const int col,
                           const int time,
                           const Direction dirn, const AnimationType type);

private:
};

#endif // KGRRUNNER_H
