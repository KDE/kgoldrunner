/*
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRRULEBOOK_H
#define KGRRULEBOOK_H

#include "kgrglobals.h"

#include <QObject>

#include "kgoldrunner_debug.h"

class KGrLevelGrid;

class KGrRuleBook : public QObject
{
    Q_OBJECT
public:
    explicit KGrRuleBook (QObject * parent);
    ~KGrRuleBook() override;

    bool variableTiming() const      { return mVariableTiming;      }
    bool alwaysCollectNugget() const { return mAlwaysCollectNugget; }
    bool runThruHole() const         { return mRunThruHole;         }
    bool reappearAtTop() const       { return mReappearAtTop;       }
    int  reappearRow() const         { return mReappearRow;         }
    int  pointsPerCell() const       { return mPointsPerCell;       }
    bool turnAnywhere() const        { return mTurnAnywhere;        }
    bool enemiesShowGold() const     { return mEnemiesShowGold;     }

    void        setTiming     (const int enemyCount = 0);

    inline void getHeroTimes  (int & runTime,       int & fallTime,
                               int & enemyFallTime, int & trapTime) {
                runTime       = times.hwalk; fallTime = times.hfall;
                enemyFallTime = times.efall; trapTime = times.ecaptive; }

    inline char getEnemyTimes (int & runTime, int & fallTime, int & trapTime) {
                runTime       = times.ewalk; fallTime = times.efall;
                trapTime      = times.ecaptive;
                return mRules; }

    inline void getDigTimes   (int & digTime, int & digCounter) {
                digTime = 200; digCounter = times.hole; }

    virtual Direction findBestWay (const int eI, const int eJ,
                                   const int hI, const int hJ,
                                   KGrLevelGrid * pGrid,
                                   bool leftRightSearch = true) = 0;

protected:
    typedef struct {
        int hwalk;
        int hfall;
        int ewalk;
        int efall;
        int ecaptive;
        int hole;
    } Timing;

    char mRules;		///< The type of rules and enemy search method.

    bool mVariableTiming;	///< More enemies imply less speed.
    bool mAlwaysCollectNugget;	///< Enemies always collect nuggets.
    bool mRunThruHole;		///< Enemy can run L/R through dug hole.
    bool mReappearAtTop;	///< Enemies reborn at top of screen.
    int  mReappearRow;		///< Row where enemies reappear.
    int  mPointsPerCell;	///< Number of points in each grid-cell.
    bool mTurnAnywhere;		///< Can change direction anywhere in grid-cell.
    bool mEnemiesShowGold;	///< Enemies show when they are carrying gold.

    Timing times;
    KGrLevelGrid * grid;
};


class KGrTraditionalRules : public KGrRuleBook
{
    Q_OBJECT
public:
    explicit KGrTraditionalRules (QObject * parent);
    ~KGrTraditionalRules() override;

    Direction findBestWay  (const int eI, const int eJ,
                            const int hI, const int hJ,
                            KGrLevelGrid * pGrid,
                            bool leftRightSearch = true) override;

private:
    Direction searchUp     (int eI, int eJ, int hJ);
    Direction searchDown   (int eI, int eJ, int hJ);
    Direction getHero      (int eI, int eJ, int hI);

    int       distanceUp   (int x,  int y,  int deltah);
    int       distanceDown (int x,  int y,  int deltah);
    bool      searchOK     (int direction,  int x, int y);
    int       canWalkLR    (int direction,  int x, int y);
    bool      willNotFall  (int x,  int y);
};


class KGrKGoldrunnerRules : public KGrRuleBook
{
    Q_OBJECT
public:
    explicit KGrKGoldrunnerRules (QObject * parent);
    ~KGrKGoldrunnerRules() override;

    Direction findBestWay  (const int eI, const int eJ,
                            const int hI, const int hJ,
                            KGrLevelGrid * pGrid,
                            bool leftRightSearch = true) override;

private:
    Direction findWayUp    (const int eI, const int eJ);
    Direction findWayDown  (const int eI, const int eJ);
    Direction findWayLeft  (const int eI, const int eJ);
    Direction findWayRight (const int eI, const int eJ);
};


class KGrScavengerRules : public KGrRuleBook
{
    Q_OBJECT
public:
    explicit KGrScavengerRules (QObject * parent);
    ~KGrScavengerRules() override;

    Direction findBestWay (const int eI, const int eJ,
                           const int hI, const int hJ,
                           KGrLevelGrid * pGrid,
                           bool leftRightSearch = true) override;
};

#endif // KGRRULEBOOK_H
