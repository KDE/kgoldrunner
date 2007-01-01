/***************************************************************************
 *   Copyright (C) 2006 by Ian Wadham                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#ifndef _KGRGAMEIO_H_
#define _KGRGAMEIO_H_

#include <QByteArray>
#include <QFile>

#include "kgrconsts.h"

// Return values from I/O operations.
enum IOStatus {OK, NotFound, NoRead, NoWrite, UnexpectedEOF};

// GameData structure: contains attributes of a KGoldrunner game.
typedef struct {
    Owner	owner;		// Owner of the game: "System" or "User".
    QString	name;		// Name of the game.
    QString     prefix;		// Game's filename prefix.
    char        settings;	// Game's rules: KGoldrunner or Traditional.
    int         nLevels;	// Number of levels in the game.
    QString     about;		// Optional text about the game.
} GameData;

// LevelData structure: contains attributes of a KGoldrunner level.
typedef struct {
    QString	filePath;	// Full file-path (for error messages).
    int		level;		// Level number.
    QByteArray	layout;		// Codes for the level layout (mandatory).
    QByteArray	name;		// Level name (optional).
    QByteArray	hint;		// Level hint (optional).
} LevelData;

/**
 * The KGrGameIO class handles I/O for text-files containing KGoldrunner games
 * and levels.  The games and levels that are released with KGoldrunner are
 * installed in a "System" directory (e.g. .../share/apps/kgoldrunner/system).
 * Those that the user composes or edits are stored in a "User" directory
 * (e.g. $HOME/.kde/share/apps/kgoldrunner/user).
 *
 * The class handles files in either KGoldrunner 2 format or KGoldrunner 3
 * format.  In KGoldrunner 2 format, the data for games is in file "games.dat"
 * and the data for levels is in multiple files "levels/<prefix><nnn>.grl".
 * Each level-file has at least one line containing codes for the level's layout
 * and can have optional extra lines containing a level name and a hint.  In
 * KGoldrunner 3 format, each game is in one file called "game_<prefix>.txt",
 * containing the game-data and the data for all levels.  The data formats are
 * the same as for KGoldrunner 2, except that the first character of each line
 * indicates what kind of data is in the rest of the line.  "G" = game data,
 * "L" = start of level data, with the level-number following the "L", and
 * " " = level data, with line 1 being the layout codes, line 2 (optional) the
 * level name and lines >2 (optional) the hint. 
 * 
 * This class is used by the game and its editor and is also used by a utility
 * program that finds game names, level names and hints and rewrites them in
 * a format suitable for extracting strings that KDE translators can use.
 *
 * @short   KGoldrunner Game-File IO
 */

class KGrGameIO : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor.
     */
    KGrGameIO ();

    /**
     * Find and read data for a level of a game, into a LevelData structure.
     */
    IOStatus fetchLevelData
	    (const QString dir, QString prefix, const int level, LevelData & d);

private:
    QFile		openLevel;
    QString		getFilePath (QString dir, const QString prefix,
							const int level);
    char		getALine (bool kgr3, QByteArray & line);
    QByteArray		removeNewline (const QByteArray & line);
};

#endif // _KGRGAMEIO_H_
