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
#include "kgrconsts.h"

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
 * @author $AUTHOR <$EMAIL>
 * @version $APP_VERSION
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

    // Slot to pause or restart the game.
    void stopStart();

    // Slot to change the graphics theme.
    void changeTheme (const QString & themeFilepath);

    // Local slots to create or edit game information.
    void createGame();
    void editGameInfo();

    // Local slots to set mouse/keyboard or laptop-hybrid control of the hero.
    void setMouseMode();
    void setKeyBoardMode();
    void setLaptopMode();

    // Local slots to set game speed.
    void normalSpeed();
    void beginSpeed();
    void champSpeed();
    void incSpeed();
    void decSpeed();

    // Slots to set Traditional or KGoldrunner rules.
    void setTradRules();
    void setKGrRules();

    // void optionsShowToolbar();
    // void optionsShowStatusbar();
    void optionsConfigureKeys();
    // void optionsConfigureToolbars();
    // void optionsPreferences();
    // void newToolbarConfig();

    void changeStatusbar (const QString& text);
    void changeCaption (const QString& text);

    void showLevel (int);		// Show the current level number.
    void showLives (long);		// Show how many lives are remaining.
    void showScore (long);		// Show the player's score.
    void gameFreeze (bool);		// Status feedback on freeze/unfreeze.

    void adjustHintAction (bool);	// Enable/disable "Hint" action.
    void markRuleType (char ruleType);	// Check game's rule type in the menu.
    void setEditMenu (bool on_off);	// Enable/disable "Save Edits" action.
    void setEditIcon (const QString & actionName, const char iconType);
    void viewFullScreen (bool activation);

    QSize sizeHint() const;

private:
    void setupActions();
    void initStatusBar();
    void setupEditToolbarActions();
    void setupThemes();

    QSignalMapper * kbMapper;		// Keyboard game-control mapper.
    QSignalMapper * dbgMapper;		// Debugging-key mapper.
    QSignalMapper * tempMapper;		// Temporary pointer.

    void keyControl (const QString & name, const QString & text,
                     const QKeySequence & shortcut, const int code);

private:
    bool startupOK;

    KGrCanvas *	view;
    KGrGame *	game;

    bool getDirectories();		// Get directory paths, as below.
    QString systemHTMLDir;		// Where the manual is stored.
    QString systemDataDir;		// Where the system levels are stored.
    QString userDataDir;		// Where the user levels are stored.

    QAction *		saveGame;	// Save game, level, lives and score.

    // A KAction is needed here, to get access to KShortcut::setAlternate().
    // IDW KAction *		myPause;	// Pause or resume the game.
    QAction *		myPause;	// Pause or resume the game.
    QString		pauseKeys;	// Keystroke names to put in status bar.

    QAction *		hintAction;	// Display a hint, if available.
    KAction *		killHero;	// Kill hero (disabled during edits).
    QAction *		highScore;	// High scores (disabled during edits).

    QAction *		saveEdits;	// Save a level that has been edited.

    KToggleFullScreenAction *fullScreen; // Show Full Screen Mode on menu.
    KToggleAction *	setMouse;	// Show mouse/keyboard mode on menu.
    KToggleAction *	setKeyboard;	// Show mouse/keyboard mode on menu.
    KToggleAction *	setLaptop;	// Show mouse/keyboard mode on menu.

    KToggleAction *	tradRules;	// Set Traditional rules.
    KToggleAction *	kgrRules;	// Set KGoldrunner rules.

    KToggleAction *	setSounds;	// enable/disable sound effects.

    // OBSOLESCENT - 18/1/09 KGrHero *	hero;			// Pointer to the hero.

    // KToggleAction *	m_toolbarAction;
    // KToggleAction *	m_statusbarAction;

    KToolBar *		editToolbar;	// Toolbar for creating/editing levels.
    KToggleAction *     m_defaultEditAct;

private slots:
    void freeSlot();			// Set editObj to Free Space.
    void edheroSlot();			// Set editObj to Hero.
    void edenemySlot();			// Set editObj to Enemy.
    void brickSlot();			// Set editObj to Brick.
    void betonSlot();			// Set editObj to Concrete.
    void fbrickSlot();			// Set editObj to Fall-through Brick.
    void ladderSlot();			// Set editObj to Ladder.
    void hladderSlot();			// Set editObj to Hidden Ladder.
    void poleSlot();			// Set editObj to Pole (or Bar).
    void nuggetSlot();			// Set editObj to Gold Nugget.
    void defaultEditObj();		// Set editObj to default (brick).
};

#endif // _KGOLDRUNNER_H_
