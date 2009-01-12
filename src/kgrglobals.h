#ifndef KGRGLOBALS_H
#define KGRGLOBALS_H

/// Codes for the rules of the selected game and level.
const char TraditionalRules = 'T';
const char KGoldrunnerRules = 'K';
const char ScavengerRules   = 'S';

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

enum GameAction {HINT, KILL_HERO};

const int  ConcreteWall = 1;

enum AnimationType {RUN, CLIMB, FALL, OPEN_BRICK, CLOSE_BRICK};

#endif // KGRGLOBALS_H
