/*
    SPDX-FileCopyrightText: 2002 Marco Krüger <grisuji@gmx.de>
    SPDX-FileCopyrightText: 2002 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGOLDRUNNER_H
#define KGOLDRUNNER_H

#include "kgrglobals.h"

#include <KXmlGuiWindow>

class QAction;
class KToggleAction;

class KGrGame;
class KGrView;
class KGrScene;
class KGrRenderer;

/**
 * This class serves as the main window for KGoldrunner.  It handles the menu,
 * toolbar and keystroke actions and sets up the game, scene and view.
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
    ~KGoldrunner() override;

    /**
     * Used to indicate if the class initialised properly.
     */
    bool startedOK() {return (startupOK);}

    void setToggle      (const QString &actionName, const bool onOff);
    void setAvail       (const QString &actionName, const bool onOff);
    void redrawEditToolbar();

protected:
    void keyPressEvent (QKeyEvent * event) override;
    void keyReleaseEvent (QKeyEvent * event) override;

private:
    void setUpKeyboardControl();
    bool identifyMoveAction (QKeyEvent * event, bool pressed);

protected:
    /**
     * This function is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties (KConfigGroup &) override;

    /**
     * This function is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties.
     */
    void readProperties (const KConfigGroup &) override;

    /// To save edits before closing.
    bool queryClose() override;

private Q_SLOTS:
    // An extension of the constructor.  Gives us two scans of the event queue.
    void KGoldrunner_2();

    // Slot to change the graphics theme.
    void changeTheme ();

    void optionsConfigureKeys();

    void gameFreeze (bool);		// Status feedback on Pause state.

    void adjustHintAction (bool);	// Enable/disable "Hint" action.
    void setEditMenu (bool on_off);	// Enable/disable "Save Edits" action.
    void setEditIcon (const QString & actionName, const char iconType);
    void viewFullScreen (bool activation);

    QSize sizeHint() const override;

private:
    void setupActions();
    void setupEditToolbarActions();

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
                          const QKeySequence & shortcut, const int code,
                          const bool mover = false);

    void keyControlDebug (const QString & name, const QString & text,
                          const QKeySequence & shortcut, const int code,
                          const bool mover = false);

    bool startupOK;

    KGrGame     *   game;		// Overall control of the gameplay.

    KGrView     *   view;		// Central widget.
    KGrScene    *   scene;		// Sets text for game-status messages.
    KGrRenderer *   renderer;		// Changes themes and gets icon pixmaps.

    bool frozen;
    bool getDirectories();		// Get directory paths, as below.

    QString systemDataDir;		// Where the system levels are stored.
    QString userDataDir;		// Where the user levels are stored.

    QAction *		saveGame;	// Save game, level, lives and score.

    // A QAction is needed here, to get access to KShortcut::setAlternate().
    QAction *		myPause;	// Pause or resume the game.

    QAction *		hintAction;	// Display a hint, if available.
    QAction *		killHero;	// Kill hero (disabled during edits).
    QAction *		highScore;	// High scores (disabled during edits).

    QAction *		saveEdits;	// Save a level that has been edited.

    KToolBar *		editToolbar;	// Toolbar for creating/editing levels.
};

#endif // KGOLDRUNNER_H
