/***************************************************************************
 *    Copyright 2009 Ian Wadham <ianw2@optusnet.com.au>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <KLocale>

#include "kgreditor.h"

KGrEditor::KGrEditor (KGrCanvas * theView, const QString & /* theSystemDir */,
                                           const QString & /* theUserDir */)
    :
    QObject          (theView),		// Destroy Editor if view closes.
    view             (theView)
{
    gameData.width   = FIELDWIDTH;
    gameData.height  = FIELDHEIGHT;
    levelData.width  = FIELDWIDTH;
    levelData.height = FIELDHEIGHT;
}

/******************************************************************************/
/************  GAME EDITOR FUNCTIONS ACTIVATED BY MENU OR TOOLBAR  ************/
/******************************************************************************/

void KGrEditor::setEditObj (char newEditObj)
{
    editObj = newEditObj;
}

void KGrEditor::createLevel()
{
    int	i, j;

    if (! saveOK (false)) {				// Check unsaved work.
        return;
    }

    // Force compile IDW if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Create Level"),
                i18n ("You cannot create and save a level "
                "until you have created a game to hold "
                "it. Try menu item \"Create Game\"."));
        // Force compile IDW return;
    // Force compile IDW }

    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

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

    editObj = BRICK;

    showEditLevel();

    savedLevelData.layout = levelData.layout;		// Copy for "saveOK()".
    savedLevelData.name   = levelData.name;
    savedLevelData.hint   = levelData.hint;

    // Re-enable player input.
    loading = false;

    view->update();					// Show the level name.
}

void KGrEditor::updateLevel()
{
    if (! saveOK (false)) {				// Check unsaved work.
        return;
    }

    // Force compile IDW if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Edit Level"),
            i18n ("You cannot edit and save a level until you "
            "have created a game and a level. Try menu item \"Create Game\"."));
        // Force compile IDW return;
    // Force compile IDW }

    if (level < 0)
        level = 0;
    // Force compile IDW int lev = selectLevel (SL_UPDATE, level);
    // Force compile IDW if (lev == 0)
        // Force compile IDW return;

    // Force compile IDW if (owner == SYSTEM) {
        // Force compile IDW KGrMessage::information (view, i18n ("Edit Level"),
            // Force compile IDW i18n ("It is OK to edit a system level, but you MUST save "
            // Force compile IDW "the level in one of your own games. You are not just "
            // Force compile IDW "taking a peek at the hidden ladders "
            // Force compile IDW "and fall-through bricks, are you? :-)"));
    // Force compile IDW }

    // Force compile IDW loadEditLevel (lev);
}

void KGrEditor::updateNext()
{
    if (! saveOK (false)) {				// Check unsaved work.
        return;
    }
    level++;
    updateLevel();
}

void KGrEditor::loadEditLevel (int lev)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

    // Read the level data.
    KGrLevelData d;
    // Force compile IDW if (! readLevelData (lev, d)) {
        // Force compile IDW loading = false;
        // Force compile IDW return;
    // Force compile IDW }

    level = lev;
    initEdit();

    int i, j;
    // Load the level.
    for (i = 1; i <= levelData.width;  i++) {
	for (j = 1; j <= levelData.height; j++) {
            editObj = d.layout [(j-1) * d.width + (i-1)];
            insertEditObj (i, j, editObj);
	}
    }
    savedLevelData.layout = d.layout;		// Copy for "saveOK()".

    // Retain the original language of the name and hint when editing,
    // but convert non-ASCII, UTF-8 substrings to Unicode (eg. Ã¼ to ü).
    levelName = (d.name.size() > 0) ?
                QString::fromUtf8 ((const char *) d.name) : "";
    levelHint = (d.hint.size() > 0) ?
                QString::fromUtf8 ((const char *) d.hint) : "";

    editObj = BRICK;				// Reset default object.

    // Force compile IDW view->setTitle (getTitle());		// Show the level name.
    showEditLevel();				// Reconnect signals.

    // Re-enable player input.
    loading = false;
}

void KGrEditor::editNameAndHint()
{
    if (! editMode)
        return;

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
    int selectedLevel = level;

    int i, j;
    QString filePath;

    if (! editMode) {
        KGrMessage::information (view, i18n ("Save Level"),
                i18n ("Inappropriate action: you are not editing a level."));
        return (false);
    }

    // Save the current collection index.
    // Force compile IDW int N = collnIndex;

    if (selectedLevel == 0) {
        // New level: choose a number.
        action = SL_CREATE;
    }
    else {
        // Existing level: confirm the number or choose a new number.
        action = SL_SAVE;
    }

    // Pop up dialog box, which could change the collection or level or both.
    // Force compile IDW selectedLevel = selectLevel (action, selectedLevel);
    if (selectedLevel == 0)
        return (false);

    // Get the new collection (if changed).
    // Force compile IDW int n = collnIndex;

    // Set the name of the output file.
    // Force compile IDW filePath = getFilePath (owner, collection, selectedLevel);
    QFile levelFile (filePath);

    // Force compile IDW if ((action == SL_SAVE) && (n == N) && (selectedLevel == level)) {
        // This is a normal edit: the old file is to be re-written.
        // Force compile IDW isNew = false;
    // Force compile IDW }
    // Force compile IDW else {
        isNew = true;
        // Check if the file is to be inserted in or appended to the collection.
        if (levelFile.exists()) {
            switch (KGrMessage::warning (view, i18n ("Save Level"),
                        i18n ("Do you want to insert a level and "
                        "move existing levels up by one?"),
                        i18n ("&Insert Level"), i18n ("&Cancel"))) {

            case 0:	// Force compile IDW if (! reNumberLevels (n, selectedLevel,
                                            // Force compile IDW collections.at (n)->nLevels, +1)) {
                            // Force compile IDW return (false);
                        // Force compile IDW }
                        break;
            case 1:	return (false);
                        break;
            }
        }
    // Force compile IDW }

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
        // Force compile IDW collections.at (n)->nLevels++;
        // Force compile IDW saveCollections (owner);
    }

    level = selectedLevel;
    // Force compile IDW emit showLevel (level);
    // Force compile IDW view->setTitle (getTitle());		// Display new title.
    return (true);
}

void KGrEditor::moveLevelFile()
{
    if (level <= 0) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("You must first load a level to be moved. Use "
                     "the \"%1\" or \"%2\" menu.",
                     i18n ("Game"), i18n ("Editor")));
        return;
    }

    int action = SL_MOVE;

    // Force compile IDW int fromC = collnIndex;
    int fromL = level;
    // Force compile IDW int toC   = fromC;
    int toL   = fromL;

    // Force compile IDW if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("You cannot move a level until you "
                "have created a game and at least two levels. Try "
                "menu item \"Create Game\"."));
        // Force compile IDW return;
    // Force compile IDW }

    // Force compile IDW if (collections.at (fromC)->owner != USER) {
        KGrMessage::information (view, i18n ("Move Level"),
                i18n ("Sorry, you cannot move a system level."));
        // Force compile IDW return;
    // Force compile IDW }

    // Pop up dialog box to get the collection and level number to move to.
    // Force compile IDW while ((toC == fromC) && (toL == fromL)) {
        // Force compile IDW toL = selectLevel (action, toL);
        if (toL == 0)
            return;

        // Force compile IDW toC = collnIndex;

        // Force compile IDW if ((toC == fromC) && (toL == fromL)) {
            KGrMessage::information (view, i18n ("Move Level"),
                    i18n ("You must change the level or the game or both."));
        // Force compile IDW }
    // Force compile IDW }

    QString filePath1;
    QString filePath2;

    // Save the "fromN" file under a temporary name.
    // Force compile IDW filePath1 = getFilePath (USER, collections.at (fromC), fromL);
    filePath2 = filePath1;
    filePath2 = filePath2.append (".tmp");
    // Force compile IDW if (! safeRename (filePath1, filePath2))
        // Force compile IDW return;

    // Force compile IDW if (toC == fromC) {					// Same collection.
        if (toL < fromL) {				// Decrease level.
            // Move "toL" to "fromL - 1" up by 1.
            // Force compile IDW if (! reNumberLevels (toC, toL, fromL-1, +1)) {
                // Force compile IDW return;
            // Force compile IDW }
        }
        else {						// Increase level.
            // Move "fromL + 1" to "toL" down by 1.
            // Force compile IDW if (! reNumberLevels (toC, fromL+1, toL, -1)) {
                // Force compile IDW return;
            // Force compile IDW }
        }
    // Force compile IDW }
    // Force compile IDW else {						// Different collection.
        // In "fromC", move "fromL + 1" to "nLevels" down and update "nLevels".
        // Force compile IDW if (! reNumberLevels (fromC, fromL + 1,
                                    // Force compile IDW collections.at (fromC)->nLevels, -1)) {
            // Force compile IDW return;
        // Force compile IDW }
        // Force compile IDW collections.at (fromC)->nLevels--;

        // In "toC", move "toL + 1" to "nLevels" up and update "nLevels".
        // Force compile IDW if (! reNumberLevels (toC, toL, collections.at (toC)->nLevels, +1)) {
            // Force compile IDW return;
        // Force compile IDW }
        // Force compile IDW collections.at (toC)->nLevels++;

        // Force compile IDW saveCollections (USER);
    // Force compile IDW }

    // Rename the saved "fromL" file to become "toL".
    // Force compile IDW filePath1 = getFilePath (USER, collections.at (toC), toL);
    // Force compile IDW safeRename (filePath2, filePath1); // IDW

    level = toL;
    // Force compile IDW collection = collections.at (toC);
    // Force compile IDW view->setTitle (getTitle());	// Re-write title.
    // Force compile IDW emit showLevel (level);
}

void KGrEditor::deleteLevelFile()
{
    int action = SL_DELETE;
    int lev = level;

    // Force compile IDW if (! ownerOK (USER)) {
        KGrMessage::information (view, i18n ("Delete Level"),
                i18n ("You cannot delete a level until you "
                "have created a game and a level. Try "
                "menu item \"Create Game\"."));
        // Force compile IDW return;
    // Force compile IDW }

    // Pop up dialog box to get the collection and level number.
    // Force compile IDW lev = selectLevel (action, level);
    if (lev == 0)
        return;

    QString filePath;

    // Set the name of the file to be deleted.
    // Force compile IDW int n = collnIndex;
    // Force compile IDW filePath = getFilePath (USER, collections.at (n), lev);
    QFile levelFile (filePath);

    // Delete the file for the selected collection and level.
    if (levelFile.exists()) {
        // Force compile IDW if (lev < collections.at (n)->nLevels) {
            switch (KGrMessage::warning (view, i18n ("Delete Level"),
                                i18n ("Do you want to delete a level and "
                                "move higher levels down by one?"),
                                i18n ("&Delete Level"), i18n ("&Cancel"))) {
            case 0:	break;
            case 1:	return; break;
            }
            levelFile.remove();
            // Force compile IDW if (! reNumberLevels (n, lev + 1, collections.at(n)->nLevels, -1)) {
                // Force compile IDW return;
            // Force compile IDW }
        // Force compile IDW }
        // Force compile IDW else {
            levelFile.remove();
        // Force compile IDW }
    }
    else {
        KGrMessage::information (view, i18n ("Delete Level"),
                i18n ("Cannot find file '%1' to be deleted.", filePath));
        return;
    }

    // Force compile IDW collections.at (n)->nLevels--;
    // Force compile IDW saveCollections (USER);
    // Force compile IDW if (lev <= collections.at (n)->nLevels) {
        // Force compile IDW level = lev;
    // Force compile IDW }
    // Force compile IDW else {
        // Force compile IDW level = collections.at (n)->nLevels;
    // Force compile IDW }

    // Repaint the screen with the level that now has the selected number.
    if (editMode && (level > 0)) {
        loadEditLevel (level);			// Load level in edit mode.
    }
    else if (level > 0) {
        // Force compile IDW enemyCount = 0;				// Load level in play mode.
        //enemies.clear();
        // Force compile IDW while (!enemies.isEmpty())
            // Force compile IDW delete enemies.takeFirst();

        // Force compile IDW view->deleteEnemySprites();
        // Force compile IDW newLevel = true;;
        // Force compile IDW loadLevel (level);
        // Force compile IDW showTutorialMessages (level);
        // Force compile IDW newLevel = false;
    }
    else {
        createLevel();				// No levels left in collection.
    }
    // Force compile IDW emit showLevel (level);
}

void KGrEditor::editCollection (int action)
{
    int lev = level;
    int n = -1;

    // If editing, choose a collection.
    if (action == SL_UPD_GAME) {
        // Force compile IDW lev = selectLevel (SL_UPD_GAME, level);
        if (lev == 0)
            return;
        level = lev;
        // Force compile IDW n = collnIndex;
    }

    // Force compile IDW KGrECDialog * ec = new KGrECDialog (action, n, collections, view);

    // Force compile IDW while (ec->exec() == QDialog::Accepted) {	// Loop until valid.

        // Validate the collection details.
        // Force compile IDW QString ecName = ec->getName();
        // Force compile IDW int len = ecName.length();
        // Force compile IDW if (len == 0) {
            KGrMessage::information (view, i18n ("Save Game Info"),
                i18n ("You must enter a name for the game."));
            // Force compile IDW continue;
        // Force compile IDW }

        // Force compile IDW QString ecPrefix = ec->getPrefix();
        // Force compile IDW if ((action == SL_CR_GAME) || (collections.at (n)->nLevels <= 0)) {
            // The filename prefix could have been entered, so validate it.
            // Force compile IDW len = ecPrefix.length();
            int len = 0; // Force compile IDW
	    if (len == 0) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("You must enter a filename prefix for the game."));
                // Force compile IDW continue;
            }
            if (len > 5) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("The filename prefix should not "
                    "be more than 5 characters."));
                // Force compile IDW continue;
            }

            bool allAlpha = true;
            for (int i = 0; i < len; i++) {
                // Force compile IDW if (! isalpha (ecPrefix.myChar (i))) {
                    allAlpha = false;
                    break;
                // Force compile IDW }
            }
            if (! allAlpha) {
                KGrMessage::information (view, i18n ("Save Game Info"),
                    i18n ("The filename prefix should be "
                    "all alphabetic characters."));
                // Force compile IDW continue;
            }

            bool duplicatePrefix = false;
            // TODO - Use KGrGameData. // KGrCollection * c;
            // Force compile IDW int imax = collections.count();
            // Force compile IDW for (int i = 0; i < imax; i++) {
                // Force compile IDW c = collections.at (i);
                // Force compile IDW if ((c->prefix == ecPrefix) && (i != n)) {
                    duplicatePrefix = true;
                    // Force compile IDW break;
                // Force compile IDW }
            // Force compile IDW }

            if (duplicatePrefix) {
                // Force compile IDW KGrMessage::information (view, i18n ("Save Game Info"),
                    // Force compile IDW i18n ("The filename prefix '%1' is already in use.",
                     // Force compile IDW ecPrefix));
                // Force compile IDW continue;
            }
        // Force compile IDW }

        // Save the collection details.
        char settings = 'K';
        // Force compile IDW if (ec->isTrad()) {
            // Force compile IDW settings = 'T';
        // Force compile IDW }
        if (action == SL_CR_GAME) {
            // Force compile IDW collections.append (new KGrCollection (USER,
                // Force compile IDW ecName, ecPrefix, settings, 0, ec->getAboutText(), 'N'));
        }
        else {
            // Force compile IDW collection->name		= ecName;
            // Force compile IDW collection->prefix		= ecPrefix;
            // Force compile IDW collection->settings	= settings;
            // Force compile IDW collection->about		= ec->getAboutText();
        }

        // Force compile IDW saveCollections (USER);
        // Force compile IDW break;				// All done now.
    // Force compile IDW }

    // Force compile IDW delete ec;
}

/******************************************************************************/
/*********************  SUPPORTING GAME EDITOR FUNCTIONS  *********************/
/******************************************************************************/
bool KGrEditor::saveOK (bool exiting)
{
    int		i, j;
    bool	result;
    QString	option2 = i18n ("&Go on editing");

    result = true;

    if (editMode) {
        if (exiting) {					// If window is closing,
            option2 = "";				// can't go on editing.
        }
        if ((shouldSave) || (levelData.layout != savedLevelData.layout)) {
            // If shouldSave == true, level name or hint was edited.
            switch (KGrMessage::warning (view, i18n ("Editor"),
                        i18n ("You have not saved your work. Do "
                        "you want to save it now?"),
                        i18n ("&Save"), i18n ("&Do Not Save"), option2))
            {
            case 0:
                result = saveLevelFile();	// Save and continue.
                break;
            case 1:
                shouldSave = false;		// Continue: don't save.
                break;
            case 2:
                result = false;			// Go back to editing.
                break;
            }
            return (result);
        }
    }
    return (result);
}

void KGrEditor::initEdit()
{
    if (! editMode) {

        editMode = true;
        // Force compile IDW emit setEditMenu (true);	// Enable edit menu items and toolbar.

        // We were previously in play mode: stop the hero running or falling.
        // Force compile IDW hero->init (1, 1);
        view->setHeroVisible (false);
    }

    paintEditObj = false;
    paintAltObj = false;

    // Set the default object and button.
    editObj = BRICK;
    // Force compile IDW emit defaultEditObj();	// Set default edit-toolbar button.

    oldI = 0;
    oldJ = 0;
    heroCount = 0;
    // Force compile IDW enemyCount = 0;
    //enemies.clear();
    // Force compile IDW while (!enemies.isEmpty())
        // Force compile IDW delete enemies.takeFirst();

    // Force compile IDW view->deleteEnemySprites();
    // Force compile IDW nuggets = 0;

    // Force compile IDW emit showLevel (level);
    // Force compile IDW emit showLives (0);
    // Force compile IDW emit showScore (0);

    deleteLevel();
    // Force compile IDW setBlankLevel (false);	// Fill playfield with Editable objects.

    // Force compile IDW view->setTitle (getTitle());// Show title of level.

    shouldSave = false;		// Used to flag editing of name or hint.
}

void KGrEditor::deleteLevel()
{
    int i,j;
    // Force compile IDW for (i = 1; i <= FIELDHEIGHT; i++)
    // Force compile IDW for (j = 1; j <= FIELDWIDTH; j++)
        // Force compile IDW delete playfield[j][i];
}

void KGrEditor::insertEditObj (int i, int j, char obj)
{
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
    return (levelData.layout [(i-1) + (j-1) * levelData.width]);
}

void KGrEditor::setEditableCell (int i, int j, char type)
{
    levelData.layout [(i-1) + (j-1) * levelData.width] = type;
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
        // Force compile IDW file1 = getFilePath (USER, collections.at (cIndex), i);
        // Force compile IDW file2 = getFilePath (USER, collections.at (cIndex), i - step);
        // Force compile IDW if (! safeRename (file1, file2)) {
            // Force compile IDW return (false);
        // Force compile IDW }
        i = i + step;
    }

    return (true);
}

void KGrEditor::setLevel (int lev)
{
    level = lev;
    return;
}

// TODO - Used only in KGrEditor.
// bool KGrGame::ownerOK (Owner o)
// {
    // // Check that this owner has at least one set of game data.
    // bool OK = false;
// 
    // foreach (KGrGameData * d, gameList) {
        // if (d->owner == o) {
            // OK = true;
            // break;
        // }
    // }
// 
    // return (OK);
// }

bool KGrEditor::saveGameData (Owner o)
{
    QString	filePath;

    if (o != USER) {
        KGrMessage::information (view, i18n ("Save Game Info"),
            i18n ("You can only modify user games."));
        return false;
    }

    // TODO - Editor needs directory paths. filePath = userDataDir + "games.dat";

    QFile c (filePath);

    // Open the output file.
    if (! c.open (QIODevice::WriteOnly)) {
        KGrMessage::information (view, i18n ("Save Game Info"),
                i18n ("Cannot open file '%1' for output.", filePath));
        return (false);
    }

    // Save the game-data objects.
    QString		line;
    int			i, len;
    char		ch;

    // TODO - Editor needs the game-list.
    // foreach (KGrGameData * gData, gameList) {
        // if (gData->owner == o) {
            // line.sprintf ("%03d %c %s %s\n", gData->nLevels, gData->rules,
                                // gData->prefix.myStr(),
                                // gData->name.constData());
            // len = line.length();
            // for (i = 0; i < len; i++)
                        // c.putChar (line.toUtf8()[i]);

            // len = gData->about.length();
            // if (len > 0) {
                // QByteArray aboutC = gData->about;
                // len = aboutC.length();		// Might be longer now.
                // for (i = 0; i < len; i++) {
                    // ch = aboutC[i];
                    // if (ch != '\n') {
                        // c.putChar (ch);		// Copy the character.
                    // }
                    // else {
                        // c.putChar ('\\');	// Change newline to \ and n.
                        // c.putChar ('n');
                    // }
                // }
                // c.putChar ('\n');		// Add a real newline.
            // }
        // }
    // }

    c.close();
    return (true);
}

/******************************************************************************/
/*********************   EDIT ACTION SLOTS   **********************************/
/******************************************************************************/

void KGrEditor::doEdit (int button)
{
    // Mouse button down: start making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos();
    i = p.x(); j = p.y();

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

void KGrEditor::endEdit (int button)
{
    // Mouse button released: finish making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos();
    i = p.x(); j = p.y();

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
