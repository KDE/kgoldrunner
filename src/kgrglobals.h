/****************************************************************************
 *    Copyright 2003  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003  Ian Wadham <ianw2@optusnet.com.au>                    *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
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

#ifndef KGRGLOBALS_H
#define KGRGLOBALS_H

#include <QByteArray>
#include <QString>

// #define ENABLE_SOUND_SUPPORT // Ian W. moved it here - 31 May 2008.

enum Owner {SYSTEM, USER};

const char FREE      = ' ';
const char ENEMY     = 'E';
const char HENEMY    = 'a'; // NEW - 2/1/09
const char HERO      = 'R';
const char CONCRETE  = 'X';
const char BRICK     = 'M';
const char FBRICK    = 'F';
const char HLADDER   = 'Z';
const char LADDER    = 'H';
const char NUGGET    = 'N';
const char FLASHING  = 'b'; // NEW - 2/1/09
const char BAR       = 'T';
const char HOLE      = 'O';
const char USEDHOLE  = 'U';

const char EDIT_HINT = '1';
const char EDIT_TEST = '2';

const int  FIELDWIDTH   = 28;
const int  FIELDHEIGHT  = 20;

const int DIGDELAY = 200;

const int STEP = 4;
const int gameCycle = 4;		// Animation frames per playfield tile.
const int graphicsCycle = 8;		// Animation frames per running cycle.

const double DROPNUGGETDELAY = 70.0;	// Enemy holds gold for avg. 12.5 cells.

// Keyboard action codes
enum KBAction		{KB_UP, KB_DOWN, KB_LEFT, KB_RIGHT,
                         KB_DIGLEFT, KB_DIGRIGHT, KB_STOP};

// Action codes when selecting a level or game for play, editing or replay.
enum SelectAction	{SL_START, SL_ANY, SL_CREATE, SL_UPDATE, SL_SAVE,
                         SL_MOVE, SL_DELETE, SL_CR_GAME, SL_UPD_GAME,
                         SL_REPLAY, SL_SOLVE, SL_NONE};

/// Codes for the rules of the selected game and level.
const char TraditionalRules = 'T';
const char KGoldrunnerRules = 'K';
const char ScavengerRules   = 'S';

/// Centralised message functions: implementations in kgrdialog.cpp.
class QWidget;
class KGrMessage
{
public:
    static void information (QWidget * parent, const QString & caption,
                             const QString & text,
                             const QString & dontShowAgain = QString());
    static int  warning     (QWidget * parent, const QString & caption,
                             const QString & text, const QString & label0,
                             const QString & label1,
                             const QString & label2 = "");
};

/// KGrGameData structure: contains attributes of a KGoldrunner game.
class KGrGameData
{
public:
    Owner	owner;		///< Owner of the game: "System" or "User".
    int         nLevels;	///< Number of levels in the game.
    char        rules;		///< Game's rules: KGoldrunner or Traditional.
    QString     prefix;		///< Game's filename prefix.
    char        skill;		///< Game's skill: Tutorial, Normal or Champion.
    int         width;		///< Width of grid, in cells.
    int         height;		///< Height of grid, in cells.
    QByteArray  name;		///< Name of the game.
    QByteArray  about;		///< Optional info about the game.
};

/// KGrLevelData structure: contains attributes of a KGoldrunner level.
class KGrLevelData
{
public:
    int		level;		///< Level number.
    int         width;		///< Width of grid, in cells.
    int         height;		///< Height of grid, in cells.
    QByteArray	layout;		///< Codes for the level layout (mandatory).
    QByteArray	name;		///< Level name (optional).
    QByteArray	hint;		///< Level hint (optional).
};

/// KGrRecording structure: contains a record of play in a KGoldrunner level.
class KGrRecording
{
public:
    // TODO - Use same header data (including date/time) as Save Game does.
    Owner          owner;	///< Original owner, at time of recording.
    char           rules;	///< Rules that applied then.
    QString        prefix;	///< Game's filename prefix.
    QByteArray     gameName;	///< Name of the game.
    KGrLevelData   levelData;	///< The level data, at time of recording.
    long           lives;	///< Number of lives at start of level.
    long           score;	///< Score at start of level.
    int            speed;	///< Speed of game during recording (normal=10).
    int            controlMode;	///< Control mode during recording (mouse, etc).
    QByteArray     content;	///< The encoded recording of play.
    QByteArray     draws;	///< The random numbers used during play.
};

// Offsets used to encode keystrokes, control modes and speeds in a recording.
// Allow space for 16 direction and digging codes, 16 control modes, 16 special
// actions and 30 speeds.  We actually have (as at May 2009) 8 direction and
// digging codes, control modes from 2 to 4, one special action (code 7) and
// speeds ranging from 2 to 20.
#define DIRECTION_CODE 0x80
#define MODE_CODE      0x90
#define ACTION_CODE    0xa0
#define SPEED_CODE     0xe0
#define END_CODE       0xff

enum GameAction    {NEW, NEXT_LEVEL, LOAD, SAVE_GAME, PAUSE, HIGH_SCORE,
                    KILL_HERO, HINT,
                    DEMO, SOLVE, INSTANT_REPLAY, REPLAY_LAST, REPLAY_ANY};

enum EditAction    {CREATE_LEVEL, EDIT_ANY, SAVE_EDITS, MOVE_LEVEL,
                    DELETE_LEVEL, CREATE_GAME,  EDIT_GAME};

enum Setting       {PLAY_SOUNDS,			// Sound effects on/off.
                    STARTUP_DEMO,			// Starting demo on/off.
                    MOUSE, KEYBOARD, LAPTOP,		// Game-control modes.
                    NORMAL_SPEED, BEGINNER_SPEED,	// Preset game-speeds.
                    CHAMPION_SPEED,
                    INC_SPEED, DEC_SPEED};		// Adjustments of speed.

const int  ConcreteWall = 1;

typedef char    DirectionFlag;
typedef char    AccessFlag;
typedef char    Flags;

enum  Direction    {STAND, RIGHT, LEFT, UP, DOWN, nDirections,
                    DIG_RIGHT = nDirections, DIG_LEFT, NO_DIRECTION};

const DirectionFlag dFlag [nDirections] = {
                0x10,		// Can stand.
                0x1,		// Can go right.
                0x2,		// Can go left.
                0x4,		// Can go up.
                0x8};		// Can go down.

const AccessFlag ENTERABLE = 0x20;

enum  Axis {X, Y, nAxes};

const int movement [nDirections][nAxes] = {
                { 0,  0},	// Standing still.
                {+1,  0},	// Movement right.
                {-1,  0},	// Movement left.
                { 0, -1},	// Movement up.
                { 0, +1}};	// Movement down.

enum AnimationType {
                RUN_R,      RUN_L,
                CLIMB_R,    CLIMB_L,
                CLIMB_U,    CLIMB_D,
                FALL_R,     FALL_L,
                OPEN_BRICK, CLOSE_BRICK, nAnimationTypes};

const AnimationType aType [nDirections] = {
                FALL_L, RUN_R, RUN_L, CLIMB_U, CLIMB_D};

enum  DebugCodes {
                DO_STEP, BUG_FIX, LOGGING, S_POSNS, S_HERO, S_OBJ,
                ENEMY_0, ENEMY_1, ENEMY_2, ENEMY_3, ENEMY_4, ENEMY_5, ENEMY_6};

const int TickTime = 20;

enum  HeroStatus {
                NORMAL, WON_LEVEL, DEAD};

#endif // KGRGLOBALS_H
