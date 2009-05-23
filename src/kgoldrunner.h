/*
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef _KGOLDRUNNER_H_
#define _KGOLDRUNNER_H_

// Status bar
const int ID_LIVES      = 0;            // Field IDs in KDE status bar.
const int ID_SCORE      = 1;
const int ID_LEVEL      = 2;
const int ID_HINTAVL    = 3;
const int ID_MSG        = 4;

const int L_LIVES       = 15;		// Lengths of fields.
const int L_SCORE       = 17;
const int L_LEVEL       = 15;

#include <kxmlguiwindow.h>
#include <kstandarddirs.h>
#include "kgrglobals.h"

class QAction;
class QSignalMapper;
class KAction;
class KToggleAction;
class KToggleFullScreenAction;

class KGrGame;
class KGrCanvas;
class KGrHero;

/**
 * This class serves as the main window for KGoldrunner.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 */
class KGoldrunner : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    KGoldrunner();

    /**
     * Default Destructor
     */
    virtual ~KGoldrunner();

    /**
     * Used to indicate if the class initialised properly.
     */
    bool startedOK() {return (startupOK);}

public slots:
    void setToggle      (const char * actionName, const bool onOff);
    void setAvail       (const char * actionName, const bool onOff);

protected:
    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties (KConfigGroup &);

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties.
     */
    void readProperties (const KConfigGroup &);

    /// To save edits before closing.
    bool queryClose();

private slots:
    // An extension of the constructor.  Gives us two scans of the event queue.
    void KGoldrunner_2();

    // Slot to change the graphics theme.
    void changeTheme (const QString & themeFilepath);

    void optionsConfigureKeys();

    void showLevel (int);		// Show the current level number.
    void showLives (long);		// Show how many lives are remaining.
    void showScore (long);		// Show the player's score.
    void gameFreeze (bool);		// Status feedback on Pause state.

    void adjustHintAction (bool);	// Enable/disable "Hint" action.
    void setEditMenu (bool on_off);	// Enable/disable "Save Edits" action.
    void setEditIcon (const QString & actionName, const char iconType);
    void viewFullScreen (bool activation);

    QSize sizeHint() const;

private:
    void setupActions();
    void initStatusBar();
    void setupEditToolbarActions();
    void setupThemes();
 
    QSignalMapper * tempMapper;		// Temporary pointer.

    QAction * gameAction (const QString & name, const int code,
                          const QString & text, const QString & toolTip,
                          const QString & whatsThis, const QKeySequence & key);

    QAction * editAction (const QString & name, const int code,
                          const QString & text, const QString & toolTip,
                          const QString & whatsThis);

    KToggleAction * settingAction (const QString & name,
                                   const int       code,
                                   const QString & text,
                                   const QString & toolTip,
                                   const QString & whatsThis);

    KToggleAction * editToolbarAction
                         (const QString & name, const char code,
                          const QString & shortText, const QString & text,
                          const QString & toolTip, const QString & whatsThis);

    void keyControl      (const QString & name, const QString & text,
                          const QKeySequence & shortcut, const int code);

    bool startupOK;

    KGrCanvas *	view;
    KGrGame *	game;
    bool        frozen;

    bool getDirectories();		// Get directory paths, as below.
    QString systemHTMLDir;		// Where the manual is stored.
    QString systemDataDir;		// Where the system levels are stored.
    QString userDataDir;		// Where the user levels are stored.

    QAction *		saveGame;	// Save game, level, lives and score.

    // A KAction is needed here, to get access to KShortcut::setAlternate().
    KAction *		myPause;	// Pause or resume the game.
    QString		pauseKeys;	// Keystroke names to put in status bar.

    QAction *		hintAction;	// Display a hint, if available.
    QAction *		killHero;	// Kill hero (disabled during edits).
    QAction *		highScore;	// High scores (disabled during edits).

    QAction *		saveEdits;	// Save a level that has been edited.

    KToolBar *		editToolbar;	// Toolbar for creating/editing levels.
};

#endif // _KGOLDRUNNER_H_
