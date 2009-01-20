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
class KGrCanvas;
class KGrHero;
class KGrEnemy;

class KGrLevelPlayer : public QObject
{
    Q_OBJECT
public:
    KGrLevelPlayer (QObject * parent, KGrGameData  * theGameData,
                                      KGrLevelData * theLevelData);
    ~KGrLevelPlayer();

    void init (KGrCanvas * view);

    void setTarget         (int pointerI, int pointerJ);
    void setDirection      (Direction dirn);
    Direction getDirection (int heroI, int heroJ);

    void tick              ();

signals:
    void animation();
    void paintCell (int i, int j, char tileType, int diggingStage = 0);
    void setSpriteType (int id, char spriteType, int row, int col);

private:
    KGrGameData  *       gameData;
    KGrLevelData *       levelData;

    KGrLevelGrid *       grid;
    KGrRuleBook *        rules;
    KGrHero *            hero;
    QList<KGrEnemy *>    enemies;

    int                  nuggets;

    bool                 pointer;
    bool                 started;
    int                  targetI;
    int                  targetJ;
    Direction            direction;
};

#endif // KGRLEVELPLAYER_H
