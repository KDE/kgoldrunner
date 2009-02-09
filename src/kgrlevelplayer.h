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

// class QTimer;
class KGrTimer;

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

    void init (KGrCanvas * view, const Control mode);
    void prepareToPlay();

    inline void setControlMode (const Control mode) { controlMode = mode; }

    void setTarget         (int pointerI, int pointerJ);
    void setDirectionByKey (Direction dirn);
    Direction getDirection (int heroI, int heroJ);

    void runnerGotGold     (const int  spriteID, const int i, const int j,
                            const bool hasGold);

    void pause             (bool stop);
    void dbgControl        (int code);	// Authors' debugging aids.

signals:
    void animation      ();
    void paintCell      (int i, int j, char tileType, int diggingStage = 0);
    int  makeSprite     (char spriteType, int i, int j);
    void startAnimation (const int spriteId, const int i, const int j,
                         const int time,
                         const Direction dirn, const AnimationType type);
    void gotGold        (const int  spriteID, const int i, const int j,
                         const bool hasGold);

private slots:
    void tick           (bool missed);
    void doDig          (int button);	// Dig using mouse-buttons.

private:
    // TODO - Eliminate mView ...
    KGrCanvas *          mView;

    KGrGameData  *       gameData;
    KGrLevelData *       levelData;

    KGrLevelGrid *       grid;
    KGrRuleBook *        rules;
    KGrHero *            hero;
    int                  heroID;
    QList<KGrEnemy *>    enemies;

    Control              controlMode;

    int                  nuggets;

    enum                 PlayState {NotReady, Ready, Playing};
    PlayState            playState;

    int                  targetI;	// Where the mouse is pointing.
    int                  targetJ;

    Direction            direction;	// Next direction for the hero to take.
    KGrTimer *           timer;		// The time-standard for the level.

// OBSOLESCENT - 21/1/09 Can do this just by calling tick().
    void restart();		// Kickstart the game action.

    void startDigging (Direction diggingDirection);

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

    bool gameLogging;		// If true, do logging printout (debug).
    bool bugFixed;		// If true, enable bug fix code (debug).

    void bugFix();		// Turn a bug fix on/off dynamically.
    void startLogging();	// Turn logging on/off.
    void showFigurePositions();	// Show everybody's co-ordinates.
    void showObjectState();	// Show an object's state.
    void showEnemyState (int);	// Show enemy's co-ordinates and state.
};

#endif // KGRLEVELPLAYER_H
