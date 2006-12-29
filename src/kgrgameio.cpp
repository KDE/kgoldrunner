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


#include <QtDebug>
#include "kgrgameio.h"

KGrGameIO::KGrGameIO ()
{
}

IOStatus KGrGameIO::fetchLevelData
	(const QString dir, QString prefix, const int level, LevelData & d)
{
    d.filePath = getFilePath (dir, prefix, level);
    qDebug() << "FILE" << d.filePath;

    openLevel.setFileName (d.filePath);

    // Check that the level-file exists.
    if (! openLevel.exists()) {
	return (NotFound);
    }

    // Open the file for read-only.
    if (! openLevel.open (QIODevice::ReadOnly)) {
	return (NoRead);
    }

    d.level  = level;
    d.layout = "";
    d.name   = "";
    d.hint   = "";

    char c;
    QByteArray textLine;
    IOStatus result = UnexpectedEOF;

    // Determine whether the file is in KGoldrunner v3 or v2 format.
    bool kgr3Format = (d.filePath.endsWith (".txt"));
    qDebug() << "kgr3Format:" << kgr3Format;

    if (kgr3Format) {
	// In KGr 3 format, if a line starts with 'L', check the number.
	while ((c = getALine (kgr3Format, textLine)) != '\0') {
	    if ((c == 'L') && (textLine.left(3).toInt() == level)) {
		break;			// We have found the required level.
	    } 
	}
	if (c == '\0') {
	    openLevel.close();		// We reached end-of-file.
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

    openLevel.close();
    return (result);
}

QString KGrGameIO::getFilePath
	(const QString dir, const QString prefix, const int level)
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

char KGrGameIO::getALine (bool kgr3, QByteArray & line)
{
    char c;
    line = "";
    while (openLevel.getChar(&c)) {
	line = line.append (c);
	if (c == '\n') {
	    break;
	}
    }

    if (line.size() <= 0) {
	// Return a '\0' byte if end-of-file.
	return ('\0');
    }
    if (kgr3) {
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

#include "kgrgameio.moc"
