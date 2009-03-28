/**************************************************************************
*   Copyright 2009 Ian Wadham <ianw2@optusnet.com.au>                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/

#ifndef _KGREDITOR_H_
#define _KGREDITOR_H_

#include <QObject>
#include "kgrconsts.h"	// OBSOLESCENT - 30/12/08
#include "kgrglobals.h"
#include "kgrcanvas.h"
#include "kgrdialog.h"	// OBSOLESCENT - 30/12/08, need KGrMessage and a subset of dialogs for the editor.

// class KGrCollection;	// OBSOLESCENT - 30/12/08

class QTimer;

/**
 * This class is the game-editor for KGoldrunner.
 *
 * @short Game-editor class
 * @author $AUTHOR <$EMAIL>
 * @version $APP_VERSION
 */
class KGrEditor : public QObject
{
    Q_OBJECT
public:
    KGrEditor (KGrCanvas * theView, const QString &theSystemDir,
                                    const QString &theUserDir,
                                    QList<KGrGameData *> & pGameList);
    ~KGrEditor();

    bool saveOK ();			// Check if edits were saved.

    void createLevel (int pGameIndex);	// Set up a blank level-display for edit.
    void updateLevel (int pGameIndex, int level);	// Update an existing level.
    bool saveLevelFile();	// Save the edited level in a text file (.grl).
    void moveLevelFile (int pGameIndex, int level);	// Move level to another game or level number.
    void deleteLevelFile (int pGameIndex, int level);	// Delete a level file.

    void editGame (int pGameIndex);	// Create a game or edit game inormation.

    void editNameAndHint();	// Run a dialog to edit the level name and hint.
    void setEditObj (char newEditObj);	// Set object for editor to paint.

    // Force compile IDW void freeze();		// Stop the gameplay action.
    // Force compile IDW void unfreeze();		// Restart the gameplay action.
    // Force compile IDW void setMessageFreeze (bool);

signals:
    void getMousePos    (int & i, int & j);
    void showLevel      (int level);

private:
    KGrCanvas * view;		// The canvas on which the editor paints.
    QString     systemDataDir;
    QString     userDataDir;
    QList<KGrGameData *> gameList;

    bool mouseMode;		// Flag to set up keyboard OR mouse control.
    bool editMode;		// Flag to change keyboard and mouse functions.
    char editObj;		// Type of object to be painted by the mouse.
    bool paintEditObj;		// Sets painting on/off (toggled by clicking).
    bool paintAltObj;		// Sets painting for the alternate object on/off
    int  oldI, oldJ;		// Last mouse position painted.
    int  editLevel;		// Level to be edited (= 0 for new level).
    int  heroCount;		// Can enter at most one hero.
    bool shouldSave;		// True if name or hint was edited.

    // The list-index of the game (collection of levels) being composed/edited.
    int          gameIndex;

    // The data, including the layout, for the level being composed or edited.
    KGrLevelData levelData;
    KGrLevelData savedLevelData;
    QString      levelName;	// Level name during editing (optional).
    QString      levelHint;	// Level hint during editing (optional).

private:
    int  selectLevel (int action, int requestedLevel, int & requestedGame);
    void loadEditLevel (int);	// Load and display an existing level for edit.
    void initEdit();
    bool readLevelData (int levelNo, KGrLevelData & d);
    void insertEditObj (int, int, char object);
    char editableCell (int i, int j);
    void setEditableCell (int, int, char);
    void showEditLevel();
    bool reNumberLevels (int, int, int, int);
    bool ownerOK (Owner o);
    bool saveGameData (Owner o);

    QString getTitle();
    QString getLevelFilePath (KGrGameData * gameData, int lev);

    QTimer *     timer;		// The time-signal for the game-editor.

    bool mouseDisabled;

private slots:
    void doEdit  (int);		// For mouse-click when in edit-mode.
    void tick    ();
    void endEdit (int);		// For mouse-release when in edit-mode.
};

#endif // _KGREDITOR_H_
