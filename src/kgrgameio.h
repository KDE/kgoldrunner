/***************************************************************************
 *  Copyright (C) 2006 by Ian Wadham
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************/


#ifndef _KGRGAMEIO_H_
#define _KGRGAMEIO_H_

#include "kgrglobals.h"

#include <QWidget>
#include <QFile>

/// Return values from I/O operations.
enum IOStatus {OK, NotFound, NoRead, NoWrite, UnexpectedEOF};

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
     *
     * @param pView    The view or widget used as a parent for error messages.
     */
    KGrGameIO (QWidget * pView);

    /**
     * Find and read data for games, into a list of KGrGameData structures.
     */
    IOStatus fetchGameListData (Owner o, const QString & dir,
                                QList<KGrGameData *> & gameList,
                                QString & filePath);

    /**
     * Find and read data for a level of a game.  Can display error messages.
     */
    bool readLevelData (const QString & dir, const QString & prefix,
                        const int levelNo, KGrLevelData & d);

    /**
     * Find and read data for a level of a game, into a KGrLevelData structure.
     * Returns an OK or error status, but does not display error messages.
     */
    IOStatus fetchLevelData    (const QString & dir, const QString & prefix,
                                const int level, KGrLevelData & d,
                                QString & filePath);

    /*
     * Rename a file, first removing any existing file that has the target name.
     */
    static bool safeRename (const QString & oldName, const QString & newName);

private:
    QWidget *           view;

    QFile		openFile;

    QString		getFilePath (const QString & dir,
                                const QString & prefix, const int level);
    char		getALine (const bool kgr3, QByteArray & line);
    QByteArray		removeNewline (const QByteArray & line);
    KGrGameData *	initGameData (Owner o);
};

#endif // _KGRGAMEIO_H_
