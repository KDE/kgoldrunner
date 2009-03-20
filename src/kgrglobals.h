#ifndef KGRGLOBALS_H
#define KGRGLOBALS_H

/// Codes for the rules of the selected game and level.
const char TraditionalRules = 'T';
const char KGoldrunnerRules = 'K';
const char ScavengerRules   = 'S';

/// Modes for controlling the hero in KGoldrunner.
enum Control {MOUSE, KEYBOARD, LAPTOP};

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

enum  GameAction {HINT, KILL_HERO};
enum  EditAction {CREATE_LEVEL, EDIT_ANY, SAVE_EDITS, MOVE_LEVEL, DELETE_LEVEL,
                  CREATE_GAME,  EDIT_GAME};

const int  ConcreteWall = 1;

typedef char    DirectionFlag;
typedef char    AccessFlag;
typedef char    Flags;

enum  Direction  {STAND, RIGHT, LEFT, UP, DOWN, nDirections,
                 DIG_RIGHT = nDirections, DIG_LEFT};

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
                OPEN_BRICK, CLOSE_BRICK};

const AnimationType aType [nDirections] = {
                FALL_L, RUN_R, RUN_L, CLIMB_U, CLIMB_D};

enum  DebugCodes {
                DO_STEP, BUG_FIX, LOGGING, S_POSNS, S_HERO, S_OBJ,
                ENEMY_0, ENEMY_1, ENEMY_2, ENEMY_3, ENEMY_4, ENEMY_5, ENEMY_6};

const int TickTime = 20;

enum  HeroStatus {
                NORMAL, WON_LEVEL, DEAD};

#endif // KGRGLOBALS_H
