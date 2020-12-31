/*
    SPDX-FileCopyrightText: 2006 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kgrgameio.h"
#include "kgoldrunner_debug.h"

#include <QDir>
#include <QWidget>

#include <KLocalizedString>

KGrGameIO::KGrGameIO (QWidget * pView)
    :
    view        (pView)
{
}

IOStatus KGrGameIO::fetchGameListData
        (const Owner o, const QString & dir, QList<KGrGameData *> & gameList,
                              QString & filePath)
{
    QDir directory (dir);
    QStringList pattern;
    pattern << QStringLiteral("game_*");
    QStringList files = directory.entryList (pattern, QDir::Files, QDir::Name);

    // KGr 3 has a game's data and all its levels in one file.
    // KGr 2 has all game-data in "games.dat" and each level in a separate file.
    bool kgr3Format = (files.count() > 0);
    if (! kgr3Format) {
        files << QStringLiteral("games.dat");
    }

    // Loop to read each file containing game-data.
    for (const QString &filename : qAsConst(files)) {
        if (filename == QLatin1String("game_ende.txt")) {
            continue;			// Skip the "ENDE" file.
        }

        filePath = dir + filename;
        KGrGameData * g = initGameData (o);
        gameList.append (g);
        // //qCDebug(KGOLDRUNNER_LOG)<< "GAME PATH:" << filePath;

        openFile.setFileName (filePath);

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
        QByteArray gameName;

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
            g->nLevels = fields.at (0).toInt();
            g->rules   = fields.at (1).at (0);
            g->prefix  = QString::fromLatin1(fields.at (2));
            // //qCDebug(KGOLDRUNNER_LOG) << "Levels:" << g->nLevels << "Rules:" << g->rules <<
                // "Prefix:" << g->prefix;

            if (kgr3Format) {
                // KGr 3 Format: get skill, get game-name from a later line.
                g->skill = fields.at (3).at (0);
            }
            else {
                // KGr 2 Format: get game-name from end of line 1.
                int n = 0;
                // Skip the first 3 fields and extract the rest of the line.
                n = textLine.indexOf (' ', n) + 1;
                n = textLine.indexOf (' ', n) + 1;
                n = textLine.indexOf (' ', n) + 1;
                gameName = removeNewline (textLine.right (textLine.size() - n));
                g->name  = i18n (gameName.constData());
            }

            // Check for further settings in this game.
            // bool usedDwfOpt = false;		// For debug.
            while ((c = getALine (kgr3Format, textLine)) == '.') {
                if (textLine.startsWith ("dwf ")) {
                    // Dig while falling is allowed in this game, or not.
                    g->digWhileFalling = textLine.endsWith (" false\n") ?
                                         false : true;
                    // usedDwfOpt = true;		// For debug.
                }
            }

            if (kgr3Format && (c == ' ')) {
                gameName = removeNewline (textLine);
                g->name  = i18n (gameName.constData());
                c = getALine (kgr3Format, textLine);
            }
            //qCDebug(KGOLDRUNNER_LOG) << "Dig while falling" << g->digWhileFalling
                     // << "usedDwfOpt" << usedDwfOpt << "Game" << g->name;
            //qCDebug(KGOLDRUNNER_LOG) << "Skill:" << g->skill << "Name:" << g->name;

            // Loop to accumulate lines of about-data.  If kgr3Format, exit on
            // EOF or 'L' line.  If not kgr3Format, exit on EOF or numeric line.
            while (c != '\0') {
                if ((c == '\0') ||
                    (kgr3Format && (c == 'L')) ||
                    ((! kgr3Format) &&
                    (textLine.at (0) >= '0') && (textLine.at (0) <= '9'))) {
                    break;
                }
                g->about.append (textLine);
                c = getALine (kgr3Format, textLine);
            }
            g->about = removeNewline (g->about);	// Remove final '\n'.
            // //qCDebug(KGOLDRUNNER_LOG) << "Info about: [" + g->about + "]";

            if ((! kgr3Format) && (c != '\0')) {
                filePath = dir + filename;
                g = initGameData (o);
                gameList.append (g);
            }
        } // END: game-data loop

        openFile.close();

    } // END: filename loop

    return (OK);
}

bool KGrGameIO::readLevelData (const QString & dir,
                               const QString & prefix,
                               const int levelNo, KGrLevelData & d)
{
    //qCDebug(KGOLDRUNNER_LOG) << "dir" << dir << "Level" << prefix << levelNo;
    QString filePath;
    IOStatus stat = fetchLevelData
                        (dir, prefix, levelNo, d, filePath);
    switch (stat) {
    case NotFound:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Cannot find file '%1'.", filePath));
        break;
    case NoRead:
    case NoWrite:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Cannot open file '%1' for read-only.", filePath));
        break;
    case UnexpectedEOF:
        KGrMessage::information (view, i18n ("Read Level Data"),
            i18n ("Reached end of file '%1' without finding level data.",
            filePath));
        break;
    case OK:
        break;
    }

    return (stat == OK);
}

IOStatus KGrGameIO::fetchLevelData
        (const QString & dir, const QString & prefix,
                const int level, KGrLevelData & d, QString & filePath)
{
    filePath = getFilePath (dir, prefix, level);
    d.level  = level;		// Level number.
    d.width  = FIELDWIDTH;	// Default width of layout grid (28 cells).
    d.height = FIELDHEIGHT;	// Default height of layout grid (20 cells).
    d.layout = "";		// Codes for the level layout (mandatory).
    d.name   = "";		// Level name (optional).
    d.hint   = "";		// Level hint (optional).

    // //qCDebug(KGOLDRUNNER_LOG)<< "LEVEL PATH:" << filePath;
    openFile.setFileName (filePath);

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
    bool kgr3Format = (filePath.endsWith (QLatin1String(".txt")));

    if (kgr3Format) {
        // In KGr 3 format, if a line starts with 'L', check the number.
        while ((c = getALine (kgr3Format, textLine)) != '\0') {
            if ((c == 'L') && (textLine.left (3).toInt() == level)) {
                break;			// We have found the required level.
            }
        }
        if (c == '\0') {
            openFile.close();		// We reached end-of-file.
            return (UnexpectedEOF);
        }
    }  

    // Check for further settings in this level.
    while ((c = getALine (kgr3Format, textLine)) == '.') {
        if (textLine.startsWith ("dwf ")) {
            // Dig while falling is allowed in this level, or not.
            d.digWhileFalling = textLine.endsWith (" false\n") ? false : true;
        }
    }

    // Get the character-codes for the level layout.
    if (c  == ' ') {
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

    // //qCDebug(KGOLDRUNNER_LOG) << "Level:" << level << "Layout length:" << d.layout.size();
    // //qCDebug(KGOLDRUNNER_LOG) << "Name:" << "[" + d.name + "]";
    // //qCDebug(KGOLDRUNNER_LOG) << "Hint:" << "[" + d.hint + "]";

    openFile.close();
    return (result);
}

QString KGrGameIO::getFilePath
        (const QString & dir, const QString & prefix, const int level)
{
    QString filePath = ((level == 0) ? QStringLiteral("ende") : prefix);
    filePath = dir + QLatin1String("game_") + filePath + QLatin1String(".txt");
    QFile test (filePath);

    // See if there is a game-file or "ENDE" screen in KGoldrunner 3 format.
    if (test.exists()) {
        return (filePath);
    }

    // If not, we are looking for a file in KGoldrunner 2 format.
    if (level == 0) {
        // End of game: show the "ENDE" screen.
        filePath = dir + QStringLiteral("levels/level000.grl");
    }
    else {
        QString num = QString::number (level).rightJustified (3, QLatin1Char('0'));
        filePath = dir + QLatin1String("levels/") + prefix + num + QLatin1String(".grl");
    }

    return (filePath);
}

char KGrGameIO::getALine (const bool kgr3, QByteArray & line)
{
    char c;
    line = "";
    while (openFile.getChar (&c)) {
        line.append (c);
        if (c == '\n') {
            break;
        }
    }

    // //qCDebug(KGOLDRUNNER_LOG) << "Raw line:" << line;
    if (line.size() <= 0) {
        // Return a '\0' byte if end-of-file.
        return ('\0');
    }
    if (kgr3) {
        // In KGr 3 format, strip off leading and trailing syntax.
        if (line.startsWith ("// ")) {
            line = line.right (line.size() - 3);
            // //qCDebug(KGOLDRUNNER_LOG) << "Stripped comment is:" << line;
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
            // //qCDebug(KGOLDRUNNER_LOG) << "Stripped syntax is:" << line;
        }
        // In Kgr 3 format, return the first byte if not end-of-file.
        c = line.at (0);
        line = line.right (line.size() - 1);
    }
    else {
        // In KGr 2 format, return a space if not end-of-file.
        c = ' ';
        if (line.startsWith (".")) {	// Line to set an option.
            c = line.at (0);
            line = line.right (line.size() - 1);
        }
    }
    return (c);
}

QByteArray KGrGameIO::removeNewline (const QByteArray & line)
{
    int len = line.size();
    if ((len > 0) && (line.endsWith ('\n'))) {
        return (line.left (len -1));
    }
    else {
        return (line);
    }
}

KGrGameData * KGrGameIO::initGameData (Owner o)
{
    KGrGameData * g = new KGrGameData;
    g->owner    = o;	// Owner of the game: "System" or "User".
    g->nLevels  = 0;	// Number of levels in the game.
    g->rules    = 'T';	// Game's rules: KGoldrunner or Traditional.
    g->digWhileFalling = true;	// The default: allow "dig while falling".
    g->prefix   = QString();	// Game's filename prefix.
    g->skill    = 'N';	// Game's skill: Tutorial, Normal or Champion.
    g->width    = FIELDWIDTH;	// Default width of layout grid (28 cells).
    g->height   = FIELDHEIGHT;	// Default height of layout grid (20 cells).
    g->name     = QString();	// Name of the game (translated, if System game).
    g->about    = "";	// Optional text about the game (untranslated).
    return (g);
}

bool KGrGameIO::safeRename (QWidget * theView, const QString & oldName,
                            const QString & newName)
{
    QFile newFile (newName);
    if (newFile.exists()) {
        // On some file systems we cannot rename if a file with the new name
        // already exists.  We must delete the existing file, otherwise the
        // upcoming QFile::rename will fail, according to Qt4 docs.  This
        // seems to be true with reiserfs at least.
        if (! newFile.remove()) {
            KGrMessage::information (theView, i18n ("Rename File"),
                i18n ("Cannot delete previous version of file '%1'.", newName));
            return false;
        }
    }
    QFile oldFile (oldName);
    if (! oldFile.rename (newName)) {
        KGrMessage::information (theView, i18n ("Rename File"),
            i18n ("Cannot rename file '%1' to '%2'.", oldName, newName));
        return false;
    }
    return true;
}


