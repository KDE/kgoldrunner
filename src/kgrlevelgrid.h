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

    inline char cellType   (int i, int j) {
        return layout [i + j * width];
    }

    inline Flags heroMoves  (int i, int j) {
        return heroAccess [i + j * width];
    }

    inline Flags enemyMoves (int i, int j) {
        return enemyAccess [i + j * width];
    }

    inline char cellState  (int i, int j) {
        return cellStates [i + j * width];
    }

private:
    inline int index (int i, int j) {
        return (i + j * width);
    }
    void calculateAccess();

    int width;
    int height;

    QVector<char>  layout;
    QVector<Flags> heroAccess;
    QVector<Flags> enemyAccess;
    QVector<char>  cellStates;

    QList<int>     hiddenLadders;
    QList<int>     hiddenEnemies;
    QList<int>     flashingGold;
};

#endif // KGRLEVELGRID_H
