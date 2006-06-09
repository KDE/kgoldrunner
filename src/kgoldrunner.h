/*
 * Copyright (C) 2003 Ian Wadham and Marco Krüger <ianw@netspace.net.au>

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Status bar
const int ID_LIVES      = 0;            // Field IDs in KDE status bar.
const int ID_SCORE      = 1;
const int ID_LEVEL      = 2;
const int ID_HINTAVL    = 3;
const int ID_MSG        = 4;

const int L_LIVES       = 15;		// Lengths of fields.
const int L_SCORE       = 17;
const int L_LEVEL       = 15;

#include <kapplication.h>
#include <kmainwindow.h>
#include <kstandarddirs.h>

class KAction;
class KToggleAction;
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
class KGoldrunner : public KMainWindow
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

    bool startedOK() {return (startupOK);}

protected:
    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig *);

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties.
     */
    void readProperties(KConfig *);

    bool queryClose ();		// To save edits before closing.

private slots:
    // Slot to pause or restart the game.
    void stopStart();

    // Local slots to create or edit game information.
    void createGame();
    void editGameInfo();

    // Local slots to set the landscape (colour scheme).
    void lsKGoldrunner();
    void lsApple2();
    void lsIceCave();
    void lsMidnight();
    void lsKDEKool();

    // Local slots to set mouse/keyboard control of the hero.
    void setMouseMode();
    void setKeyBoardMode();

    // Local slots to set game speed.
    void normalSpeed();
    void beginSpeed();
    void champSpeed();
    void incSpeed();
    void decSpeed();

    // Slots to set Traditional or KGoldrunner rules.
    void setTradRules();
    void setKGrRules();

    // Local slots to make playing area larger or smaller.
    void makeLarger();
    void makeSmaller();

    // Local slots for hero control keys.
    void goUp();
    void goR();
    void goDown();
    void goL();
    void stop();
    void digR();
    void digL();

    void setKey (KBAction movement);

    // Local slots for authors' debugging aids.
    void showEnemy0();
    void showEnemy1();
    void showEnemy2();
    void showEnemy3();
    void showEnemy4();
    void showEnemy5();
    void showEnemy6();

    void optionsShowToolbar();
    void optionsShowStatusbar();
    void optionsConfigureKeys();
    void optionsConfigureToolbars();
    void optionsPreferences();
    void newToolbarConfig();

    void changeStatusbar(const QString& text);
    void changeCaption(const QString& text);

    void showLevel (int);		// Show the current level number.
    void showLives (long);		// Show how many lives are remaining.
    void showScore (long);		// Show the player's score.
    void gameFreeze (bool);		// Status feedback on freeze/unfreeze.

    void adjustHintAction (bool);	// Enable/disable "Hint" action.
    void markRuleType (char ruleType);	// Check game's rule type in the menu.
    void setEditMenu (bool on_off);	// Enable/disable "Save Edits" action.

private:
    void setupAccel();
    void setupActions();
    void initStatusBar();
    void setupEditToolbarActions();
private:
    bool startupOK;

    KGrCanvas *	view;
    KGrGame *	game;

    bool getDirectories();		// Get directory paths, as below.
    QString systemHTMLDir;		// Where the manual is stored.
    QString systemDataDir;		// Where the system levels are stored.
    QString userDataDir;		// Where the user levels are stored.

    KAction *		saveGame;	// Save game, level, lives and score.

    KAction *		myPause;	// Pause or resume the game.
    QString		pauseKeys;	// Keystroke names to put in status bar.

    KAction *		hintAction;	// Display a hint, if available.
    KAction *		killHero;	// Kill hero (disabled during edits).
    KAction *		highScore;	// High scores (disabled during edits).

    KAction *		saveEdits;	// Save a level that has been edited.

    KToggleAction *	setKGoldrunner;	// Show default "KGoldrunner" landscape.
    KToggleAction *	setAppleII;	// Show "Apple II" landscape.
    KToggleAction *	setIceCave;	// Show "Ice Cave" landscape.
    KToggleAction *	setMidnight;	// Show "Midnight" landscape.
    KToggleAction *	setKDEKool;	// Show "KDE Kool" landscape.

    KToggleAction *	setMouse;	// Show mouse/keyboard mode on menu.
    KToggleAction *	setKeyboard;	// Show mouse/keyboard mode on menu.

    KToggleAction *	tradRules;	// Set Traditional rules.
    KToggleAction *	kgrRules;	// Set KGoldrunner rules.

    KGrHero *	hero;			// Pointer to the hero.

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
