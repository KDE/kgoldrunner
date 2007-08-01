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


#include "kgrgameio.h"
#include <KDebug>

KGrGameIO::KGrGameIO ()
{
}

IOStatus KGrGameIO::fetchGameListData
	(const QString & dir, QList<GameData *> & gameList)
{
    QDir directory (dir);
    QStringList pattern;
    pattern << "game_*";
    QStringList files = directory.entryList (pattern, QDir::Files, QDir::Name);

    // KGr 3 has a game's data and all its levels in one file.
    // KGr 2 has all game-data in "games.dat" and each level in a separate file.
    bool kgr3Format = (files.count() > 0);
    if (! kgr3Format) {
	files << "games.dat";
    }

    // Loop to read each file containing game-data.
    foreach (const QString filename, files) {
	if (filename == "game_ende.txt") {
	    continue;			// Skip the "ENDE" file.
	}

	GameData * g = initGameData (dir + filename);
	gameList.append (g);
	// kDebug() << endl << "GAME PATH:" << g->filePath;

	openFile.setFileName (g->filePath);

	// Check that the game-file exists.
	if (! openFile.exists()) {
	    return (NotFound);
	}

	// Open the file for read-only.
	if (! openFile.open (QIODevice::ReadOnly)) {
	    return (NoRead);
	}

	char c;
	QByteArray textLine;

	// Find the first line of game-data.
	c = getALine (kgr3Format, textLine);
	if (kgr3Format) {
	    while ((c != 'G') && (c != '\0')) {
		c = getALine (kgr3Format, textLine);
	    }
	}
	if (c == '\0') {
	    openFile.close();
	    return (UnexpectedEOF);	// We reached end-of-file unexpectedly.
	}

	// Loop to extract the game-data for each game on the file.
	while (c != '\0') {
	    if (kgr3Format && (c == 'L')) {
		break;			// End of KGr 3 game-file header.
	    }
	    // Decode line 1 of the game-data.
	    QList<QByteArray> fields = textLine.split (' ');
	    g->nLevels = fields.at(0).toInt();
	    g->rules   = fields.at(1).at(0);
	    g->prefix  = fields.at(2);
	    // kDebug() << "Levels:" << g->nLevels << "Rules:" << g->rules <<
		// "Prefix:" << g->prefix;

	    if (kgr3Format) {
		// KGr 3 Format: get skill, get game-name from next line.
		g->skill = fields.at(3).at(0);
		c = getALine (kgr3Format, textLine);
		if (c == ' ') {
		    g->name = removeNewline (textLine);
		}
	    }
	    else {
		// KGr 2 Format: get game-name from end of line 1.
		int n = 0;
		// Skip the first 3 fields and extract the rest of the line.
		n = textLine.indexOf(' ', n) + 1;
		n = textLine.indexOf(' ', n) + 1;
		n = textLine.indexOf(' ', n) + 1;
		g->name = removeNewline (textLine.right (textLine.size() - n));
	    }
	    // kDebug() << "Skill:" << g->skill << "Name:" << g->name;

	    // Loop to accumulate lines of about-data.  If kgr3Format, exit on
	    // EOF or 'L' line.  If not kgr3Format, exit on EOF or numeric line.
	    while (c != '\0') {
		c = getALine (kgr3Format, textLine);
		if ((c == '\0') ||
		    (kgr3Format && (c == 'L')) ||
		    ((! kgr3Format) &&
		    (textLine.at(0) >= '0') && (textLine.at(0) <= '9'))) {
		    break;
		}
		g->about.append (textLine);
	    }
	    g->about = removeNewline (g->about);	// Remove final '\n'.
	    // kDebug() << "Info about: [" + g->about + "]";

	    if ((! kgr3Format) && (c != '\0')) {
		g = initGameData (dir + filename);
		gameList.append (g);
	    }
	} // END: game-data loop

	openFile.close();

    } // END: filename loop

    return (OK);
}

IOStatus KGrGameIO::fetchLevelData
	(const QString & dir, const QString & prefix,
		const int level, LevelData & d)
{
    d.filePath = getFilePath (dir, prefix, level);
    d.level  = level;		// Level number.
    d.layout = "";		// Codes for the level layout (mandatory).
    d.name   = "";		// Level name (optional).
    d.hint   = "";		// Level hint (optional).

    // kDebug() << endl << "LEVEL PATH:" << d.filePath;
    openFile.setFileName (d.filePath);

    // Check that the level-file exists.
    if (! openFile.exists()) {
	return (NotFound);
    }

    // Open the file for read-only.
    if (! openFile.open (QIODevice::ReadOnly)) {
	return (NoRead);
    }

    char c;
    QByteArray textLine;
    IOStatus result = UnexpectedEOF;

    // Determine whether the file is in KGoldrunner v3 or v2 format.
    bool kgr3Format = (d.filePath.endsWith (".txt"));

    if (kgr3Format) {
	// In KGr 3 format, if a line starts with 'L', check the number.
	while ((c = getALine (kgr3Format, textLine)) != '\0') {
	    if ((c == 'L') && (textLine.left(3).toInt() == level)) {
		break;			// We have found the required level.
	    } 
	}
	if (c == '\0') {
	    openFile.close();		// We reached end-of-file.
	    return (UnexpectedEOF);
	}
    }  

    // Read in the character-codes for the level layout.
    if ((c = getALine (kgr3Format, textLine)) == ' ') {
	result = OK;
	d.layout = removeNewline (textLine);		// Remove '\n'.

	// Look for a line containing a level name (optional).
	if ((c = getALine (kgr3Format, textLine)) == ' ') {
	    d.name = removeNewline (textLine);		// Remove '\n'.

	    // Look for one or more lines containing a hint (optional).
	    while ((c = getALine (kgr3Format, textLine)) == ' ') {
		d.hint.append (textLine);
	    }
	    d.hint = removeNewline (d.hint);		// Remove final '\n'.
	}
    }

    // kDebug() << "Level:" << level << "Layout length:" << d.layout.size();
    // kDebug() << "Name:" << "[" + d.name + "]";
    // kDebug() << "Hint:" << "[" + d.hint + "]";

    openFile.close();
    return (result);
}

QString KGrGameIO::getFilePath
	(const QString & dir, const QString & prefix, const int level)
{
    QString filePath = ((level == 0) ? "ende" : prefix);
    filePath = dir + "game_" + filePath + ".txt";
    QFile test (filePath);

    // See if there is a game-file or "ENDE" screen in KGoldrunner 3 format.
    if (test.exists()) {
	return (filePath);
    }

    // If not, we are looking for a file in KGoldrunner 2 format.
    if (level == 0) {
	// End of game: show the "ENDE" screen.
	filePath = dir + "levels/level000.grl";
    }
    else {
	QString num;
	num.setNum (level);			// Convert INT -> QString.
	num = num.rightJustified (3,'0');	// Add 0-2 zeros at left.
	filePath = dir + "levels/" + prefix + num + ".grl";
    }

    return (filePath);
}

char KGrGameIO::getALine (const bool kgr3, QByteArray & line)
{
    char c;
    line = "";
    while (openFile.getChar(&c)) {
	line = line.append (c);
	if (c == '\n') {
	    break;
	}
    }

    // kDebug() << "Raw line:" << line;
    if (line.size() <= 0) {
	// Return a '\0' byte if end-of-file.
	return ('\0');
    }
    if (kgr3) {
	// In KGr 3 format, strip off leading and trailing syntax.
	if (line.startsWith ("// ")) {
	    line = line.right (line.size() - 3);
	    // kDebug() << "Stripped comment is:" << line;
	}
	else {
	    if (line.startsWith (" i18n(\"")) {
		line = ' ' + line.right (line.size() - 7);
	    }
	    else if (line.startsWith (" NOTi18n(\"")) {
		line = ' ' + line.right (line.size() - 10);
	    }
	    else if (line.startsWith (" \"")) {
		line = ' ' + line.right (line.size() - 2);
	    }
	    if (line.endsWith ("\");\n")) {
		line = line.left (line.size() - 4) + '\n';
	    }
	    else if (line.endsWith ("\\n\"\n")) {
		line = line.left (line.size() - 4) + '\n';
	    }
	    else if (line.endsWith ("\"\n")) {
		line = line.left (line.size() - 2);
	    }
	    // kDebug() << "Stripped syntax is:" << line;
	}
	// In Kgr 3 format, return the first byte if not end-of-file.
	c = line.at (0);
	line = line.right (line.size() - 1);
	return (c);
    }
    else {
	// In KGr 2 format, return a space if not end-of-file.
	return (' ');
    }
}

QByteArray KGrGameIO::removeNewline (const QByteArray & line)
{
    int len = line.size ();
    if ((len > 0) && (line.endsWith ('\n'))) {
	return (line.left (len -1));
    }
    else {
	return (line);
    }
}

GameData * KGrGameIO::initGameData (const QString & filePath)
{
    GameData * g = new GameData;
    g->filePath = filePath;
    g->owner    = USER;	// Owner of the game: "System" or "User".
    g->nLevels  = 0;	// Number of levels in the game.
    g->rules    = 'T';	// Game's rules: KGoldrunner or Traditional.
    g->prefix   = "";	// Game's filename prefix.
    g->skill    = 'N';	// Game's skill: Tutorial, Normal or Champion.
    g->name     = "";	// Name of the game.
    g->about    = "";	// Optional text about the game.
    return (g);
}

#include "kgrgameio.moc"
