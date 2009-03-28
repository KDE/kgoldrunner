/****************************************************************************
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

#include <KLocale>
#include <KDebug>
#include <QTimer>
#include <ctype.h>

#include "kgreditor.h"
#include "kgrselector.h"
#include "kgrdialog.h"
#include "kgrgameio.h"

KGrEditor::KGrEditor (KGrCanvas * theView,
                      const QString & theSystemDir,
                      const QString & theUserDir,
                      QList<KGrGameData *> & pGameList)
    :
    QObject          (theView),		// Destroy Editor if view closes.
    view             (theView),
    systemDataDir    (theSystemDir),
    userDataDir      (theUserDir),
    gameList         (pGameList),
    editObj          (BRICK),
    shouldSave       (false),
    mouseDisabled    (true)
{
    levelData.width  = FIELDWIDTH;	// Default values for a brand new game.
    levelData.height = FIELDHEIGHT;

    // Connect and start the timer.
    timer = new QTimer (this);
    connect (timer, SIGNAL (timeout ()), this, SLOT (tick ()));
    timer->start (TickTime);		// TickTime def in kgrglobals.h.

    // Connect edit-mode slots to signals from "view".
    connect (view, SIGNAL (mouseClick (int)), SLOT (doEdit (int)));
    connect (view, SIGNAL (mouseLetGo (int)), SLOT (endEdit (int)));
    connect (this, SIGNAL (getMousePos (int &, int &)),
             view, SLOT   (getMousePos (int &, int &)));
}

KGrEditor::~KGrEditor()
{
}

void KGrEditor::setEditObj (char newEditObj)
{
    editObj = newEditObj;
}

void KGrEditor::createLevel (int pGameIndex)
{
    if (! saveOK ()) {				// Check unsaved work.
        return;
    }

    if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Create Level"),
                i18n ("You cannot create and save a level "
                "until you have created a game to hold "
                "it. Try menu item \"Create Game\"."));
        return;
    }

    int	i, j;
    gameIndex = pGameIndex;
    editLevel = 0;

    // Ignore player input from the mouse while the screen is set up.
    mouseDisabled = true;

    levelData.level = 0;
    levelData.name  = "";
    levelData.hint  = "";
    initEdit();

    // Clear the playfield.
    levelData.layout.resize (levelData.width * levelData.height);
    for (i = 1; i <= levelData.width; i++) {
	for (j = 1; j <= levelData.height; j++) {
            insertEditObj (i, j, FREE);
        }
    }

    // Add a hero.
    insertEditObj (1, 1, HERO);

    savedLevelData.layout = levelData.layout;		// Copy for "saveOK()".
    savedLevelData.name   = levelData.name;
    savedLevelData.hint   = levelData.hint;

    view->update();					// Show the level name.

    // Re-enable player input.
    mouseDisabled = false;
}

void KGrEditor::updateLevel (int pGameIndex, int level)
{

    if (! saveOK ()) {				// Check unsaved work.
        return;
    }

    if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Edit Level"),
            i18n ("You cannot edit and save a level until you "
            "have created a game and a level. Try menu item \"Create Game\"."));
        return;
    }

    gameIndex = pGameIndex;

    // Ignore player input from the mouse while the screen is set up.
    mouseDisabled = true;

    if (level < 0)
        level = 0;
    int selectedLevel = selectLevel (SL_UPDATE, level, gameIndex);
    kDebug() << "Selected" << gameList.at(gameIndex)->name
             << "level" << selectedLevel;
    if (selectedLevel == 0) {
        mouseDisabled = false;
        return;
    }

    if (gameList.at(gameIndex)->owner == SYSTEM) {
        KGrMessage::information (view, i18n ("Edit Level"),
            i18n ("It is OK to edit a system level, but you MUST save "
            "the level in one of your own games. You are not just "
            "taking a peek at the hidden ladders "
            "and fall-through bricks, are you? :-)"));
    }

    loadEditLevel (selectedLevel);
    mouseDisabled = false;
}

void KGrEditor::loadEditLevel (int lev)
{
    // Read the level data.
    KGrLevelData d;
    if (! readLevelData (lev, d)) {
        return;
    }

    editLevel = lev;
    initEdit();

    int  i, j;
    char obj;

    // Load the level.
    for (i = 1; i <= levelData.width;  i++) {
	for (j = 1; j <= levelData.height; j++) {
            obj = d.layout [(j-1) * d.width + (i-1)];
            insertEditObj (i, j, obj);
	}
    }
    savedLevelData.layout = d.layout;		// Copy for "saveOK()".

    // Retain the original language of the name and hint when editing,
    // but convert non-ASCII, UTF-8 substrings to Unicode (eg. Ã¼ to ü).
    levelName = (d.name.size() > 0) ?
                QString::fromUtf8 ((const char *) d.name) : "";
    levelHint = (d.hint.size() > 0) ?
                QString::fromUtf8 ((const char *) d.hint) : "";

    view->setTitle (getTitle());		// Show the level name.
}

// TODO - Does this code really need to be duplicated in KGrGame and KGrEditor?
bool KGrEditor::readLevelData (int levelNo, KGrLevelData & d)
{
    KGrGameIO io;
    // If system game or ENDE screen, choose system dir, else choose user dir.
    const QString dir = ((gameList.at(gameIndex)->owner == SYSTEM) ||
                         (levelNo == 0)) ? systemDataDir : userDataDir;
    kDebug() << "Owner" << gameList.at(gameIndex)->owner << "dir" << dir
             << "Level" << gameList.at(gameIndex)->prefix << levelNo;
    QString filePath;
    IOStatus stat = io.fetchLevelData (dir, gameList.at(gameIndex)->prefix,
                                       levelNo, d, filePath);

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

void KGrEditor::editNameAndHint()
{
    // Run a dialog box to create/edit the level name and hint.
    KGrNHDialog * nh = new KGrNHDialog (levelName, levelHint, view);

    if (nh->exec() == QDialog::Accepted) {
        levelName = nh->getName();
        levelHint = nh->getHint();
        shouldSave = true;
    }

    delete nh;
}

bool KGrEditor::saveLevelFile()
{
    bool isNew;
    int action;

    int i, j;
    QString filePath;

    // Save the current game index.
    int N = gameIndex;

    if (editLevel == 0) {
        // New level: choose a number.
        action = SL_CREATE;
    }
    else {
        // Existing level: confirm the number or choose a new number.
        action = SL_SAVE;
    }

    // Pop up dialog box, which could change the game or level or both.
    int selectedLevel = selectLevel (action, editLevel, gameIndex);
    if (selectedLevel == 0) {
        return (false);
    }

    // Get the new game (if changed).
    int n = gameIndex;

    // Set the name of the output file.
    filePath = getLevelFilePath (gameList.at(n), selectedLevel);
    QFile levelFile (filePath);

    if ((action == SL_SAVE) && (n == N) && (selectedLevel == editLevel)) {
        // This is a normal edit: the old file is to be re-written.
        isNew = false;
    }
    else {
        isNew = true;
        // Check if the file is to be inserted in or appended to the game.
        if (levelFile.exists()) {
            switch (KGrMessage::warning (view, i18n ("Save Level"),
                        i18n ("Do you want to insert a level and "
                        "move existing levels up by one?"),
                        i18n ("&Insert Level"), i18n ("&Cancel"))) {

            case 0:	if (! reNumberLevels (n, selectedLevel,
                                            gameList.at (n)->nLevels, +1)) {
                            return (false);
                        }
                        break;
            case 1:	return (false);
                        break;
            }
        }
    }

    // Open the output file.
    if (! levelFile.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Save Level"),
                i18n ("Cannot open file '%1' for output.", filePath));
        return (false);
    }

    // Save the level - row by row.
    for (j = 1; j <= levelData.height; j++) {
        for (i = 1; i <= levelData.width; i++) {
            levelFile.putChar (editableCell (i, j));
        }
    }
    savedLevelData.layout = levelData.layout;	// Copy for "saveOK()".
    levelFile.putChar ('\n');

    // Save the level name, changing non-ASCII chars to UTF-8 (eg. ü to Ã¼).
    QByteArray levelNameC = levelName.toUtf8();
    int len1 = levelNameC.length();
    if (len1 > 0) {
        for (i = 0; i < len1; i++)
            levelFile.putChar (levelNameC[i]);
        levelFile.putChar ('\n');			// Add a newline.
    }

    // Save the level hint, changing non-ASCII chars to UTF-8 (eg. ü to Ã¼).
    QByteArray levelHintC = levelHint.toUtf8();
    int len2 = levelHintC.length();
    char ch = '\0';

    if (len2 > 0) {
        if (len1 <= 0)
            levelFile.putChar ('\n');		// Leave blank line for name.
        for (i = 0; i < len2; i++) {
            ch = levelHintC[i];
            levelFile.putChar (ch);		// Copy the character.
        }
        if (ch != '\n')
            levelFile.putChar ('\n');		// Add a newline character.
    }

    levelFile.close();
    shouldSave = false;

    if (isNew) {
        gameList.at (n)->nLevels++;
        saveGameData (USER);
    }

    editLevel = selectedLevel;
    emit showLevel (editLevel);
    view->setTitle (getTitle());		// Display new title.
    return (true);
}

void KGrEditor::moveLevelFile (int pGameIndex, int level)
{
    if (level <= 0) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("You must first load a level to be moved. Use "
                     "the \"%1\" or \"%2\" menu.",
                     i18n ("Game"), i18n ("Editor")));
        return;
    }
    gameIndex = pGameIndex;

    int action = SL_MOVE;

    int fromC = gameIndex;
    int fromL = level;
    int toC   = fromC;
    int toL   = fromL;

    if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("You cannot move a level until you "
                "have created a game and at least two levels. Try "
                "menu item \"Create Game\"."));
        return;
    }

    if (gameList.at (fromC)->owner != USER) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("Sorry, you cannot move a system level."));
        return;
    }

    // Pop up dialog box to get the game and level number to move to.
    while ((toC == fromC) && (toL == fromL)) {
        toL = selectLevel (action, toL, gameIndex);
        if (toL == 0)
            return;

        toC = gameIndex;

        if ((toC == fromC) && (toL == fromL)) {
            KGrMessage::information (view, i18n ("Move Level"),
                    i18n ("You must change the level or the game or both."));
        }
    }

    QString filePath1;
    QString filePath2;

    // Save the "fromN" file under a temporary name.
    filePath1 = getLevelFilePath (gameList.at (fromC), fromL);
    filePath2 = filePath1;
    filePath2 = filePath2.append (".tmp");
    if (! KGrGameIO::safeRename (filePath1, filePath2))
        return;

    if (toC == fromC) {					// Same game.
        if (toL < fromL) {				// Decrease level.
            // Move "toL" to "fromL - 1" up by 1.
            if (! reNumberLevels (toC, toL, fromL-1, +1)) {
                return;
            }
        }
        else {						// Increase level.
            // Move "fromL + 1" to "toL" down by 1.
            if (! reNumberLevels (toC, fromL+1, toL, -1)) {
                return;
            }
        }
    }
    else {						// Different game.
        // In "fromC", move "fromL + 1" to "nLevels" down and update "nLevels".
        if (! reNumberLevels (fromC, fromL + 1,
                                    gameList.at (fromC)->nLevels, -1)) {
            return;
        }
        gameList.at (fromC)->nLevels--;

        // In "toC", move "toL + 1" to "nLevels" up and update "nLevels".
        if (! reNumberLevels (toC, toL, gameList.at (toC)->nLevels, +1)) {
            return;
        }
        gameList.at (toC)->nLevels++;

        saveGameData (USER);
    }

    // Rename the saved "fromL" file to become "toL".
    filePath1 = getLevelFilePath (gameList.at (toC), toL);
    KGrGameIO::safeRename (filePath2, filePath1); // IDW

    editLevel = toL;
    emit showLevel (editLevel);
    view->setTitle (getTitle());	// Re-write title.
}

void KGrEditor::deleteLevelFile (int pGameIndex, int level)
{
    int action = SL_DELETE;
    gameIndex = pGameIndex;

    if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Delete Level"),
                i18n ("You cannot delete a level until you "
                "have created a game and a level. Try "
                "menu item \"Create Game\"."));
        return;
    }

    // Pop up dialog box to get the game and level number.
    int selectedLevel = selectLevel (action, level, gameIndex);
    if (selectedLevel == 0) {
        return;
    }

    QString filePath;

    // Set the name of the file to be deleted.
    int n = gameIndex;
    filePath = getLevelFilePath (gameList.at (n), selectedLevel);
    QFile levelFile (filePath);

    // Delete the file for the selected game and level.
    if (levelFile.exists()) {
        if (selectedLevel < gameList.at (n)->nLevels) {
            switch (KGrMessage::warning (view, i18n ("Delete Level"),
                                i18n ("Do you want to delete a level and "
                                "move higher levels down by one?"),
                                i18n ("&Delete Level"), i18n ("&Cancel"))) {
            case 0:	break;
            case 1:	return; break;
            }
            levelFile.remove();
            if (! reNumberLevels (n, selectedLevel + 1,
                                  gameList.at(n)->nLevels, -1)) {
                return;
            }
        }
        else {
            levelFile.remove();
        }
    }
    else {
        KGrMessage::information (view, i18n ("Delete Level"),
                i18n ("Cannot find file '%1' to be deleted.", filePath));
        return;
    }

    gameList.at (n)->nLevels--;
    saveGameData (USER);
    if (selectedLevel <= gameList.at (n)->nLevels) {
        editLevel = selectedLevel;
    }
    else {
        editLevel = gameList.at (n)->nLevels;
    }

    // Repaint the screen with the level that now has the selected number.
    if (level > 0) {
        loadEditLevel (editLevel);	// Load level in edit mode.
    }
    else {
        createLevel (gameIndex);	// No levels left in game.
    }
    emit showLevel (editLevel);
}

void KGrEditor::editGame (int pGameIndex)
{
    int n = -1;
    int action = (pGameIndex < 0) ? SL_CR_GAME : SL_UPD_GAME;
    gameIndex = pGameIndex;

    // If editing, choose a game.
    if (gameIndex >= 0) {
        int selectedLevel = selectLevel (SL_UPD_GAME, editLevel, gameIndex);
        if (selectedLevel == 0) {
            return;
        }
        editLevel = selectedLevel;
        n = gameIndex;
    }

    KGrECDialog * ec = new KGrECDialog (action, n, gameList, view);

    while (ec->exec() == QDialog::Accepted) {	// Loop until valid.

        // Validate the game details.
        QString ecName = ec->getName();
        int len = ecName.length();
        if (len == 0) {
            KGrMessage::information (view, i18n ("Save Game Info"),
                i18n ("You must enter a name for the game."));
            continue;
        }

        QString ecPrefix = ec->getPrefix();
        if ((action == SL_CR_GAME) || (gameList.at (n)->nLevels <= 0)) {
            // The filename prefix could have been entered, so validate it.
            len = ecPrefix.length();
	    if (len == 0) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("You must enter a filename prefix for the game."));
                continue;
            }
            if (len > 5) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("The filename prefix should not "
                    "be more than 5 characters."));
                continue;
            }

            bool allAlpha = true;
            for (int i = 0; i < len; i++) {
                if (! isalpha (ecPrefix.at (i).toLatin1())) {
                    allAlpha = false;
                    break;
                }
            }
            if (! allAlpha) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("The filename prefix should be "
                    "all alphabetic characters."));
                continue;
            }

            bool duplicatePrefix = false;
            KGrGameData * c;
            int imax = gameList.count();
            for (int i = 0; i < imax; i++) {
                c = gameList.at (i);
                if ((c->prefix == ecPrefix) && (i != n)) {
                    duplicatePrefix = true;
                    break;
                }
            }

            if (duplicatePrefix) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("The filename prefix '%1' is already in use.",
                    ecPrefix));
                continue;
            }
        }

        // Save the game details.
        char rules = 'K';
        if (ec->isTrad()) {
            rules = 'T';
        }

        KGrGameData * gameData = 0;
        if (action == SL_CR_GAME) {
            // Create empty game data and add it to the list.
            gameData = new KGrGameData();
            gameList.append (gameData);

            // Set the initial values for a new game.
            gameData->owner   = USER;
            gameData->nLevels = 0;
            gameData->skill   = 'N';
            gameData->width   = FIELDWIDTH;
            gameData->height  = FIELDHEIGHT;
        }
        else {
            // Point to existing game data.
            gameData = gameList.at (gameIndex);
        }

        // Create or update the editable values.
        gameData->rules       = rules;
        gameData->prefix      = ecPrefix;
        gameData->name        = ecName.toUtf8();
        gameData->about       = ec->getAboutText().toUtf8();

        saveGameData (USER);
        break;				// All done now.
    }

    delete ec;
}

/******************************************************************************/
/************************    LEVEL SELECTION DIALOG    ************************/
/******************************************************************************/

int KGrEditor::selectLevel (int action, int requestedLevel, int & requestedGame)
{
    int selectedLevel = 0;		// 0 = no selection (Cancel) or invalid.
    int selectedGame  = requestedGame;

    // Create and run a modal dialog box to select a game and level.
    KGrSLDialog * sl = new KGrSLDialog (action, requestedLevel, requestedGame,
                                        gameList, systemDataDir, userDataDir,
                                        view);
    bool selected = sl->selectLevel (selectedGame, selectedLevel);
    delete sl;

    if (selected) {
        requestedGame = selectedGame;
        return (selectedLevel);
    }
    else {
        return (0);			// 0 = cancelled or invalid.
    }
}

/******************************************************************************/
/*********************  SUPPORTING GAME EDITOR FUNCTIONS  *********************/
/******************************************************************************/

bool KGrEditor::saveOK ()
{
    bool result = true;

    if ((shouldSave) || (levelData.layout != savedLevelData.layout)) {
        // If shouldSave == true, level name or hint was edited.
        switch (KGrMessage::warning (view, i18n ("Editor"),
                    i18n ("You have not saved your work. Do "
                    "you want to save it now?"),
                    i18n ("&Save"), i18n ("&Do Not Save"),
                    i18n ("&Go on editing")))
        {
        case 0:
            result = saveLevelFile();		// Save and continue.
            break;
        case 1:
            shouldSave = false;			// Continue: don't save.
            break;
        case 2:
            result = false;			// Go back to editing.
            break;
        }
    }

    return (result);
}

void KGrEditor::initEdit()
{
    paintEditObj = false;
    paintAltObj  = false;

    // Set the default object and button.
    editObj = BRICK;

    oldI = 0;
    oldJ = 0;
    heroCount = 0;

    emit showLevel (editLevel);
    view->setTitle (getTitle());	// Show title of level.

    shouldSave = false;		// Used to flag editing of name or hint.
}

void KGrEditor::insertEditObj (int i, int j, char obj)
{
    kDebug() << i << j << obj;
    if ((i < 1) || (j < 1) || (i > levelData.width) || (j > levelData.height)) {
        return;		// Do nothing: mouse pointer is out of playfield.
    }

    if (editableCell (i, j) == HERO) {
        // The hero is in this cell: remove him.
        setEditableCell (i, j, FREE);
        heroCount = 0;
    }

    if (obj == HERO) {
        if (heroCount > 0) {
            // Can only have one hero: remove him from his previous position.
            for (int m = 1; m <= levelData.width; m++) {
		for (int n = 1; n <= levelData.height; n++) {
                    if (editableCell (m, n) == HERO) {
                        setEditableCell (m, n, FREE);
                    }
		}
            }
        }
        heroCount = 1;
    }

    setEditableCell (i, j, obj);
}

char KGrEditor::editableCell (int i, int j)
{
    return (levelData.layout [(i - 1) + (j - 1) * levelData.width]);
}

void KGrEditor::setEditableCell (int i, int j, char type)
{
    levelData.layout [(i - 1) + (j - 1) * levelData.width] = type;
    view->paintCell (i, j, type);
}

void KGrEditor::showEditLevel()
{
    // Disconnect play-mode slots from signals from "view".
    disconnect (view, SIGNAL (mouseClick (int)), 0, 0);
    disconnect (view, SIGNAL (mouseLetGo (int)), 0, 0);

    // Connect edit-mode slots to signals from "view".
    connect (view, SIGNAL (mouseClick (int)), SLOT (doEdit (int)));
    connect (view, SIGNAL (mouseLetGo (int)), SLOT (endEdit (int)));
}

bool KGrEditor::reNumberLevels (int cIndex, int first, int last, int inc)
{
    int i, n, step;
    QString file1, file2;

    if (inc > 0) {
        i = last;
        n = first - 1;
        step = -1;
    }
    else {
        i = first;
        n = last + 1;
        step = +1;
    }

    while (i != n) {
        file1 = getLevelFilePath (gameList.at (cIndex), i);
        file2 = getLevelFilePath (gameList.at (cIndex), i - step);
        if (! KGrGameIO::safeRename (file1, file2)) {
            return (false);
        }
        i = i + step;
    }

    return (true);
}

bool KGrEditor::ownerOK (Owner o)
{
    // Check that this owner has at least one set of game data.
    bool OK = false;

    foreach (KGrGameData * d, gameList) {
        if (d->owner == o) {
            OK = true;
            break;
        }
    }

    return (OK);
}

bool KGrEditor::saveGameData (Owner o)
{
    QString	filePath;

    if (o != USER) {
        KGrMessage::information (view, i18n ("Save Game Info"),
            i18n ("You can only modify user games."));
        return (false);
    }

    filePath = userDataDir + "games.dat";

    QFile c (filePath);

    // Open the output file.
    if (! c.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Save Game Info"),
                i18n ("Cannot open file '%1' for output.", filePath));
        return (false);
    }

    // Save the game-data objects.
    QString             line;
    QByteArray          lineC;
    int			i, len;
    char		ch;

    foreach (KGrGameData * gData, gameList) {
        if (gData->owner == o) {
            line.sprintf ("%03d %c %s %s\n", gData->nLevels, gData->rules,
                                gData->prefix.toLatin1().constData(),
                                gData->name.constData());
            lineC = line.toUtf8();
            len = lineC.length();
            for (i = 0; i < len; i++) {
                c.putChar (lineC.at (i));
            }

            len = gData->about.length();
            if (len > 0) {
                QByteArray aboutC = gData->about;
                len = aboutC.length();		// Might be longer now.
                for (i = 0; i < len; i++) {
                    ch = aboutC[i];
                    if (ch != '\n') {
                        c.putChar (ch);		// Copy the character.
                    }
                    else {
                        c.putChar ('\\');	// Change newline to \ and n.
                        c.putChar ('n');
                    }
                }
                c.putChar ('\n');		// Add a real newline.
            }
        }
    }

    c.close();
    return (true);
}

QString KGrEditor::getTitle()
{
    if (editLevel <= 0) {
        // Generate a special title for a new level.
        return (i18n ("New Level"));
    }

    // Set title string to "Game-name - NNN" or "Game-name - NNN - Level-name".
    KGrGameData * gameData = gameList.at(gameIndex);
    QString levelNumber;
    levelNumber.setNum (editLevel);
    levelNumber = levelNumber.rightJustified (3,'0');

    // TODO - Make sure gameData->name.constData() results in Unicode display
    //        and not some weird UTF8 character stuff.
    QString levelTitle = (levelName.length() <= 0)
                    ?
                    i18nc ("Game name - level number.",
                           "%1 - %2",
                           gameData->name.constData(), levelNumber)
                    :
                    i18nc ("Game name - level number - level name.",
                           "%1 - %2 - %3",
                           gameData->name.constData(), levelNumber, levelName);
    return (levelTitle);
}

QString KGrEditor::getLevelFilePath (KGrGameData * gameData, int lev)
{
    QString filePath;

    filePath.setNum (lev);			// Convert integer -> QString.
    filePath = filePath.rightJustified (3,'0'); // Add 0-2 zeros at left.
    filePath.append (".grl");			// Add KGoldrunner level-suffix.
    filePath.prepend (gameData->prefix);	// Add the game's file-prefix.

    filePath.prepend (userDataDir + "levels/");	// Add user's directory path.

    return (filePath);
}

/******************************************************************************/
/*********************   EDIT ACTION SLOTS   **********************************/
/******************************************************************************/

void KGrEditor::doEdit (int button)
{
    if (mouseDisabled) {
        return;
    }

    // Mouse button down: start making changes.
    int i, j;
    emit getMousePos (i, j);
    kDebug() << "Button" << button << "at" << i << j;

    switch (button) {
    case Qt::LeftButton:
        paintEditObj = true;
        insertEditObj (i, j, editObj);
        oldI = i;
        oldJ = j;
        break;
    case Qt::RightButton:
        paintAltObj = true;
        insertEditObj (i, j, FREE);
        oldI = i;
        oldJ = j;
        break;
    default:
        break;
    }
}

void KGrEditor::tick()
{
    if (mouseDisabled) {
        return;
    }

    // Check if a mouse-button is down: left = paint, right = erase.
    if (paintEditObj || paintAltObj) {

        int i, j;
        emit getMousePos (i, j);

        // Check if the mouse has moved.
        if ((i != oldI) || (j != oldJ)) {
            // If so, paint or erase a cell.
            insertEditObj (i, j, (paintEditObj) ? editObj : FREE);
            oldI = i;
            oldJ = j;
        }
    }
}

void KGrEditor::endEdit (int button)
{
    if (mouseDisabled) {
        return;
    }

    // Mouse button released: finish making changes.
    int i, j;
    emit getMousePos (i, j);

    switch (button) {
    case Qt::LeftButton:
        paintEditObj = false;
        if ((i != oldI) || (j != oldJ)) {
            insertEditObj (i, j, editObj);
        }
        break;
    case Qt::RightButton:
        paintAltObj = false;
        if ((i != oldI) || (j != oldJ)) {
            insertEditObj (i, j, FREE);
        }
        break;
    default:
        break;
    }
}

#include "kgreditor.moc"
