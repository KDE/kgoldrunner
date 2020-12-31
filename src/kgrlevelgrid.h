/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRLEVELGRID_H
#define KGRLEVELGRID_H

#include "kgrglobals.h"

#include <QList>
#include <QObject>
#include <QVector>

class KGrLevelGrid : public QObject
{
    Q_OBJECT
public:
    KGrLevelGrid (QObject * parent, const KGrRecording * theLevelData);
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

    inline int enemyOccupied (int i, int j) {
        return enemyHere [i + j * width];
    }

    inline void setEnemyOccupied (int i, int j, const int spriteId) {
        enemyHere [i + j * width] = spriteId;
    }

    void calculateAccess (bool pRunThruHole);

    void changeCellAt (const int i, const int j, const char type);

    void placeHiddenLadders();

Q_SIGNALS:
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
    QVector<int>   enemyHere;

    QList<int>     hiddenLadders;
    QList<int>     hiddenEnemies;
    QList<int>     flashingGold;
};

#endif // KGRLEVELGRID_H
