/****************************************************************************
 *    Copyright 2003  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003  Ian Wadham <iandw.au@gmail.com>                       *
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

const char BACKDROP  = '0';

const char EDIT_HINT = '1';
const char EDIT_TEST = '2';

const int  FIELDWIDTH   = 28;
const int  FIELDHEIGHT  = 20;

const int STEP = 4;
const int gameCycle = 4;		// Animation frames per playfield tile.
const int graphicsCycle = 8;		// Animation frames per running cycle.

// Keyboard action codes
enum KBAction		{KB_UP, KB_DOWN, KB_LEFT, KB_RIGHT,
                         KB_DIGLEFT, KB_DIGRIGHT, KB_STOP};

// Action codes when selecting a level or game for play, editing or replay.
enum SelectAction	{SL_START, SL_ANY, SL_CREATE, SL_UPDATE, SL_SAVE,
                         SL_MOVE, SL_DELETE, SL_CR_GAME, SL_UPD_GAME,
                         SL_REPLAY, SL_SOLVE, SL_SAVE_SOLUTION, SL_NONE};

/// Codes for the rules of the selected game and level.
const char TraditionalRules = 'T';
const char KGoldrunnerRules = 'K';
const char ScavengerRules   = 'S';

// Codes and array indices for the sounds of the game.
enum {GoldSound, StepSound, ClimbSound, FallSound, DigSound, LadderSound, 
      DeathSound, CompletedSound, VictorySound, GameOverSound, NumSounds};

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
    Owner       owner;		///< Owner of the game: "System" or "User".
    int         nLevels;	///< Number of levels in the game.
    char        rules;		///< Game's rules: KGoldrunner or Traditional.
    QString     prefix;		///< Game's filename prefix.
    char        skill;		///< Game's skill: Tutorial, Normal or Champion.
    int         width;		///< Width of grid, in cells.
    int         height;		///< Height of grid, in cells.
    QString     name;		///< Name of game (translated, if System game).
    QByteArray  about;		///< Optional info about game (untranslated).
};

/// KGrLevelData structure: contains attributes of a KGoldrunner level.
class KGrLevelData
{
public:
    int         level;		///< Level number.
    int         width;		///< Width of grid, in cells.
    int         height;		///< Height of grid, in cells.
    QByteArray  layout;		///< Codes for the level layout (mandatory).
    QByteArray  name;		///< Level name (optional).
    QByteArray  hint;		///< Level hint (optional).
};

/// KGrRecording structure: contains a record of play in a KGoldrunner level.
class KGrRecording
{
public:
    QString        dateTime;    ///< Date+time of recording (UTC in ISO format).
    Owner          owner;	///< Original owner, at time of recording.
    char           rules;	///< Rules that applied at time of recording.
    QString        prefix;	///< Game's filename prefix.
    QString        gameName;	///< Name of the game (translated at rec time).
    int            level;	///< Level number (at time of recording).
    int            width;	///< Width of grid, in cells (at rec time).
    int            height;	///< Height of grid, in cells (at rec time).
    QByteArray     layout;	///< Codes for the level layout (at rec time).
    QString        levelName;	///< Name of the level (translated at rec time).
    QString        hint;	///< Hint (translated at recording time).
    long           lives;	///< Number of lives at start of level.
    long           score;	///< Score at start of level.
    int            speed;	///< Speed of game during recording (normal=10).
    int            controlMode;	///< Control mode during recording (mouse, etc).
    int            keyOption;  	///< Click/hold option for keyboard mode.
    QByteArray     content;	///< The encoded recording of play.
    QByteArray     draws;	///< The random numbers used during play.
};

// Offsets used to encode keystrokes, control modes and speeds in a recording.
// Allow space for 16 direction and digging codes, 12 control modes, 4 keyboard
// click/hold option codes, 16 special actions and 30 speeds.  We actually have
// (as at July 2010) 12 direction and digging codes, control modes from 2 to 4,
// two keyboard options, one special action (code 6) and speeds ranging from
// 2 to 20.

#define DIRECTION_CODE 0x80
#define MODE_CODE      0x90
#define KEY_OPT_CODE   0x9c
#define ACTION_CODE    0xa0
#define SPEED_CODE     0xe0
#define END_CODE       0xff

enum GameAction    {NEW, NEXT_LEVEL, LOAD, SAVE_GAME, PAUSE, HIGH_SCORE,
                    KILL_HERO, HINT,
                    DEMO, SOLVE, SAVE_SOLUTION,
                    INSTANT_REPLAY, REPLAY_LAST, REPLAY_ANY};

enum EditAction    {CREATE_LEVEL, EDIT_ANY, SAVE_EDITS, MOVE_LEVEL,
                    DELETE_LEVEL, CREATE_GAME,  EDIT_GAME};

enum Setting       {PLAY_SOUNDS,			// Sound effects on/off.
                    STARTUP_DEMO,			// Starting demo on/off.
                    MOUSE, KEYBOARD, LAPTOP,		// Game-control modes.
                    CLICK_KEY, HOLD_KEY, 		// Key-control method.
                    NORMAL_SPEED, BEGINNER_SPEED,	// Preset game-speeds.
                    CHAMPION_SPEED,
                    INC_SPEED, DEC_SPEED,		// Adjustments of speed.
                    PLAY_STEPS};			// Footsteps on/off.

const int  ConcreteWall = 1;

typedef char    DirectionFlag;
typedef char    AccessFlag;
typedef char    Flags;

enum  Direction    {STAND, RIGHT, LEFT, UP, DOWN, nDirections,
                    DIG_RIGHT = nDirections, DIG_LEFT, NO_DIRECTION,
                    UP_LEFT, DOWN_LEFT, UP_RIGHT, DOWN_RIGHT, EndDirection};

const DirectionFlag dFlag [nDirections] = {
                0x10,		// Can stand.
                0x1,		// Can go right.
                0x2,		// Can go left.
                0x4,		// Can go up.
                0x8};		// Can go down.

const AccessFlag ENTERABLE = 0x20;

enum  Axis {X, Y, nAxes};

const int movement [EndDirection][nAxes] = {
                { 0,  0},	// Standing still.
                {+1,  0},	// Movement right.
                {-1,  0},	// Movement left.
                { 0, -1},	// Movement up.
                { 0, +1},	// Movement down.
                { 0,  0},	// Dig right (placeholder).
                { 0,  0},	// Dig left (placeholder).
                { 0,  0},	// No direction (placeholder).
                {-1, -1},	// Up and left (with hold-key option).
                {-1, +1},	// Down and left (with hold-key option).
                {+1, -1},	// Up and right (with hold-key option).
                {+1, +1}};	// Down and right (with hold-key option).

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
                NORMAL, WON_LEVEL, DEAD, UNEXPECTED_END};

#endif // KGRGLOBALS_H
