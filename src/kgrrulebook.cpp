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

#include "kgrrulebook.h"

KGrRuleBook::KGrRuleBook (QObject * parent)
    :
    QObject (parent),
    mVariableTiming      (true),
    mAlwaysCollectNugget (true),
    mRunThruHole         (true),
    mReappearAtTop       (true),
    mReappearRow         (2),
    mPointsPerCell       (4),
    mTurnAnywhere        (false)
{
}

KGrRuleBook::~KGrRuleBook()
{
}

// Initialise the static flags for the rules.

KGrTraditionalRules::KGrTraditionalRules (QObject * parent)
    :
    KGrRuleBook (parent)
{
    mVariableTiming      = true;	///< More enemies imply less speed.
    mAlwaysCollectNugget = true;	///< Enemies always collect nuggets.
    mRunThruHole         = true;	///< Enemy can run L/R through dug hole.
    mReappearAtTop       = true;	///< Enemies reborn at top of screen.
    mReappearRow         = 2;		///< Row where enemies reappear.
    mPointsPerCell       = 4;		///< Number of points in each grid-cell.
    mTurnAnywhere        = false;	///< Change direction only at next cell.
    mEnemiesShowGold     = true;	///< Show enemies carrying gold.

    Timing t = {40, 58, 78, 88, 170, 23};
    times = t;
}

KGrTraditionalRules::~KGrTraditionalRules()
{
}

Direction KGrTraditionalRules::findBestWay (const QPoint & enemyPosition,
                                            const QPoint & heroPosition,
                                            const KGrLevelGrid & grid)
{
    return RIGHT;
}

void KGrTraditionalRules::setTiming (const int enemyCount)
{
    int choice;
    Timing varTiming[6] = {
                          {40, 58, 78, 88, 170, 23},      // No enemies.
                          {50, 68, 78, 88, 170, 32},      // 1 enemy.
                          {57, 67, 114, 128, 270, 37},    // 2 enemies.
                          {60, 70, 134, 136, 330, 40},    // 3 enemies.
                          {63, 76, 165, 150, 400, 46},    // 4 enemies.
                          {70, 80, 189, 165, 460, 51}     // >4 enemies.
                          };
    choice = (enemyCount < 0) ? 0 : enemyCount;
    choice = (enemyCount > 5) ? 5 : enemyCount;
    times  = varTiming [enemyCount];
}


KGrKGoldrunnerRules::KGrKGoldrunnerRules (QObject * parent)
    :
    KGrRuleBook (parent)
{
    mVariableTiming      = false;	///< Speed same for any no. of enemies.
    mAlwaysCollectNugget = false;	///< Enemies sometimes collect nuggets.
    mRunThruHole         = false;	///< Enemy cannot run through dug hole.
    mReappearAtTop       = false;	///< Enemies reborn at start position.
    mReappearRow         = -1;		///< Row where enemies reappear (N/A).
    mPointsPerCell       = 4;		///< Number of points in each grid-cell.
    mTurnAnywhere        = false;	///< Change direction only at next cell.
    mEnemiesShowGold     = true;	///< Show enemies carrying gold.

    Timing t = {45, 50, 55, 100, 500, 40};
    times = t;
}

KGrKGoldrunnerRules::~KGrKGoldrunnerRules()
{
}

Direction KGrKGoldrunnerRules::findBestWay (const QPoint & enemyPosition,
                                            const QPoint & heroPosition,
                                            const KGrLevelGrid & grid)
{
    return RIGHT;
}


KGrScavengerRules::KGrScavengerRules (QObject * parent)
    :
    KGrRuleBook (parent)
{
    mVariableTiming      = false;	///< Speed same for any no. of enemies.
    mAlwaysCollectNugget = true;	///< Enemies always collect nuggets.
    mRunThruHole         = true;	///< Enemy can run L/R through dug hole.
    mReappearAtTop       = true;	///< Enemies reborn at top of screen.
    mReappearRow         = 1;		///< Row where enemies reappear.
    mPointsPerCell       = 12;		///< Number of points in each grid-cell.
    mTurnAnywhere        = true;	///< Change direction anywhere in cell.
    mEnemiesShowGold     = false;	///< Conceal enemies carrying gold.

    Timing t = {45, 50, 55, 100, 500, 40};
    times = t;
}

KGrScavengerRules::~KGrScavengerRules()
{
}

Direction KGrScavengerRules::findBestWay   (const QPoint & enemyPosition,
                                            const QPoint & heroPosition,
                                            const KGrLevelGrid & grid)
{
    return RIGHT;
}

#include "kgrrulebook.moc"
