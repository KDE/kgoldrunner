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

#ifndef KGRLEVELGRID_H
#define KGRLEVELGRID_H

#include <QObject>
#include <QVector>
#include <QList>

#include "kgrconsts.h" // OBSOLESCENT - 1/1/09
#include "kgrglobals.h"

class KGrLevelGrid : public QObject
{
    Q_OBJECT
public:
    KGrLevelGrid (QObject * parent, KGrLevelData * theLevelData);
    ~KGrLevelGrid();

    inline char cellType    (int i, int j) {
        return layout [i + j * width];
    }

    inline Flags heroMoves  (int i, int j) {
        return heroAccess [i + j * width];
    }

    inline Flags enemyMoves (int i, int j) {
        return enemyAccess [i + j * width];
    }

    inline void gotGold (const int i, const int j, const bool runnerHasGold) {
        layout [i + j * width] = (runnerHasGold) ? FREE : NUGGET;
    }

    void calculateAccess (bool pRunThruHole);

    void changeCellAt (const int i, const int j, const char type);

    void placeHiddenLadders();

signals:
    void showHiddenLadders (const QList<int> & ladders, const int width);

private:
    inline int index (int i, int j) {
        return (i + j * width);
    }

    void calculateCellAccess (const int i, const int j);

    int  width;
    int  height;

    bool runThruHole;		// Rule: Whether enemies run L/R through a hole.

    QVector<char>  layout;
    QVector<Flags> heroAccess;
    QVector<Flags> enemyAccess;

    QList<int>     hiddenLadders;
    QList<int>     hiddenEnemies;
    QList<int>     flashingGold;
};

#endif // KGRLEVELGRID_H
