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

#ifndef _KGREDITOR_H_
#define _KGREDITOR_H_

// TODO - Use fwd decls and header-order KGoldrunner-Qt-KDE in other .h files.
#include "kgrglobals.h"

#include <QObject>

class KGrCanvas;
class KGrGameIO;
class QTimer;

/**
 * This class is the game-editor for KGoldrunner.  It loads KGrGameData and
 * KGrLevelData objects from files, operates directly on the data in those
 * objects and saves them back to files.  In particular, the layout of a level
 * is edited by selecting objects such as bricks, ladders, etc. from a toolbar
 * and using mouse-clicks and drags to show where those objects are required.
 * As this happens, the corresponding character-codes are stored directly in
 * the QByteArray of layout-data and the corresponding visual objects (tiles)
 * are displayed on the screen by the KGrCanvas (view) object.
 *
 * @short Game-editor class
 */
class KGrEditor : public QObject
{
    Q_OBJECT
public:
    /**
     * The constructor of KGrEditor.
     *
     * @param theView      The canvas on which the editor paints the layout.
     *                     Also the object that owns the editor and will destroy
     *                     it if the KGoldrunner application is terminated.
     * @param theSystemDir The directory-path where system (released) game and
     *                     level data are stored.  This data is read-only, but
     *                     can be copied, edited and saved in the user's area.
     * @param theUserDir   The directory-path where the user's composed or
     *                     edited game and level data are stored.
     * @param pGameList    The current list of system and user game-data.  The
     *                     user can add a game to the list and add levels to
     *                     that game or any other game in the user's area.
     */
    KGrEditor (KGrCanvas * theView, const QString &theSystemDir,
                                    const QString &theUserDir,
                                    QList<KGrGameData *> & pGameList);
    ~KGrEditor();

    /**
     * Check if there are any unsaved edits and, if so, ask the user what to
     * do.  It will call saveLevelFile() if the user wants to save.
     *
     * @return             If true, the level was successfully saved or there
     *                     was nothing to save or the user decided not to save,
     *                     so it is OK to do something new or close KGoldrunner.
     *                     If false, the user decided to continue editing or the
     *                     file I/O failed.
     */
    bool saveOK();

    /**
     * Set up a blank level-layout, ready for editing.
     *
     * @param pGameIndex   The list-index of the game which will contain the new
     *                     level: assumed for now, but can change at save time.
     */
    void createLevel (int pGameIndex);

    /**
     * Load and display an existing level, ready for editing.  This can be a
     * system (released) level, but the changes must be saved in the user's area.
     *
     * @param pGameIndex   The list-index of the game that contains the level
     *                     to be edited: verified by a dialog and may change.
     * @param pLevel       The number of the level to be edited: verified by a
     *                     dialog and may change.
     */
    void updateLevel (int pGameIndex, int pLevel);

    /**
     * Save an edited level in a text file (*.grl) in the user's area.  The
     * required game and level number are obtained from a dialog.  These are the
     * same as the original game and level number by default, but can be altered
     * so as to get a Save As effect.  For example, a system level can be loaded
     * and edited, then saved in one of the user's own games.
     *
     * @return             If true, the level was successfully saved.  If false,
     *                     the user cancelled the save or the file I/O failed.
     */
    bool saveLevelFile();	// Save the edited level in a text file (.grl).

    /**
     * Move a level to another game or level number.  Can be used to arrange
     * levels in order of difficulty within a game.
     *
     * @param pGameIndex   The list-index of the game that contains the level
     *                     to be moved: a dialog selects the game to move to.
     * @param pLevel       The number of the level to be moved: a dialog selects
     *                     the number to move to.  Other numbers may be changed,
     *                     to preserve the sequential numbering of levels.
     */
    void moveLevelFile (int pGameIndex, int pLevel);

    /**
     * Delete a level from a game.
     *
     * @param pGameIndex   The list-index of the game that contains the level
     *                     to be deleted: verified by a dialog and may change.
     * @param pLevel       The number of the level to be deleted: verified by a
     *                     dialog and may change.
     */
    void deleteLevelFile (int pGameIndex, int pLevel);

    /**
     * Create a new game (a collection point for levels) or load the details
     * of an existing game, ready for editing.
     *
     * @param pGameIndex   The list-index of the game to be created or edited:
     *                     0 = create, >0 = edit (verified by a dialog and may
     *                     change).
     */
    void editGame (int pGameIndex);

    /**
     * Run a dialog in which the name and hint of a level can be edited.
     */
    void editNameAndHint();

    /**
     * Set the next object for the editor to paint, e.g. brick, enemy, ladder.
     *
     * @param newEditObj   A character-code for the type of object.
     */
    void setEditObj (char newEditObj);

    inline void getGameAndLevel (int & game, int & lev) {
                                 game = gameIndex; lev = editLevel; }

signals:
    /**
     * Get the next grid-position at which to paint an object in the layout.
     *
     * @param i            The row-number of the cell (return by reference).
     * @param j            The column-number of the cell (return by reference).
     */
    void getMousePos    (int & i, int & j);

    /**
     * Pass the number of the level being edited to the GUI.
     *
     * @param level        The level-number.
     */
    void showLevel      (int level);

private:
    KGrCanvas * view;		// The canvas on which the editor paints.
    KGrGameIO * io;		// I/O object for reading level-data.
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
    int  gameIndex;

    // The data, including the layout, for the level being composed or edited.
    KGrLevelData levelData;
    KGrLevelData savedLevelData;
    QString      levelName;	// Level name during editing (optional).
    QString      levelHint;	// Level hint during editing (optional).

    /**
     * Run a dialog to select a game and level to be edited or saved.
     *
     * @param action         A code for the type of editing: affects validation
     *                       and labelling in the dialog.
     * @param requestedLevel The current level, used as a default, but can be
     *                       changed by the user.
     * @param requestedGame  The current game, used as a default, but can be
     *                       changed by the user (return by reference).
     *
     * @return               The level the user chose, or zero if the user
     *                       cancelled the dialog.  The level chosen could be
     *                       different from the requestedLevel parameter.
     */
    int  selectLevel (int action, int requestedLevel, int & requestedGame);

    void loadEditLevel (int);	// Load and display an existing level for edit.
    void initEdit();
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
    /**
     * Start painting or erasing cells on the layout.  Triggered by pressing
     * a mouse-button.
     *
     * @param button The button being pressed: left for paint, right for erase.
     */
    void doEdit  (int button);

    /**
     * If the mouse has moved to a new cell and a button is down, continue
     * painting or erasing cells on the layout.  Triggered by a timer signal.
     */
    void tick    ();

    /**
     * Stop painting or erasing cells on the layout.  Triggered by releasing
     * a mouse-button.
     *
     * @param button The button being released: left for paint, right for erase.
     */
    void endEdit (int button);
};

#endif // _KGREDITOR_H_
