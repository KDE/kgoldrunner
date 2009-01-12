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

#ifndef KGRLEVELPLAYER_H
#define KGRLEVELPLAYER_H

#include <QObject>
#include <QList>

#include "kgrconsts.h" // OBSOLESCENT - 1/1/09
#include "kgrglobals.h"

class KGrLevelGrid;
class KGrRuleBook;
class KGrNewHero;
class KGrNewEnemy;

class KGrLevelPlayer : public QObject
{
    Q_OBJECT
public:
    KGrLevelPlayer (QObject * parent, KGrGameData  * theGameData,
                                      KGrLevelData * theLevelData);
    ~KGrLevelPlayer();

    void init ();
    KGrNewHero * getHero() const;
    QList<KGrNewEnemy *> getEnemies() const;

    void setDirection (int i, int j);

signals:
    void paintCell (int i, int j, char tileType, int diggingStage = 0);
    void setSpriteType (int id, char spriteType);
    // void makeHeroSprite (int i, int j, int frame); // OBSOLESCENT - 11/1/09
    // void makeEnemySprite (int i, int j, int frame); // OBSOLESCENT - 11/1/09

private:
    KGrGameData  *       gameData;
    KGrLevelData *       levelData;

    KGrLevelGrid *       grid;
    KGrRuleBook *        rules;
    KGrNewHero *         hero;
    QList<KGrNewEnemy *> enemies;
    KGrNewEnemy *        enemy;

    int                  nuggets;

    bool                 started;
    int                  lastI;
    int                  lastJ;
};

#endif // KGRLEVELPLAYER_H
