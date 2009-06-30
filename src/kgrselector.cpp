/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/

#include "kgrglobals.h"

#include "kgrselector.h"

#include "kgrgameio.h"

#include <QTextStream>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QHeaderView>
#include <QScrollBar>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QLabel>
#include <QTextEdit>

#include <QPainter>

#include <KGlobalSettings>
#include <KConfigGroup>
#include <KIntNumInput>

/******************************************************************************/
/*****************    DIALOG BOX TO SELECT A GAME AND LEVEL   *****************/
/******************************************************************************/

KGrSLDialog::KGrSLDialog (int action, int requestedLevel, int gameIndex,
                        QList<KGrGameData *> & gameList,
                        const QString & pSystemDir, const QString & pUserDir,
                        QWidget * parent)
    :
    KDialog       (parent),
    slAction      (action),
    myGameList    (gameList),
    defaultGame   (gameIndex),
    defaultLevel  (requestedLevel),
    systemDir     (pSystemDir),
    userDir       (pUserDir),
    slParent      (parent)
{
    setupWidgets();
}

KGrSLDialog::~KGrSLDialog()
{
}

bool KGrSLDialog::selectLevel (int & selectedGame, int & selectedLevel)
{
    selectedGame  = defaultGame;
    selectedLevel = 0;		// 0 = no selection (Cancel) or invalid.

    // Create and run a modal dialog box to select a game and level.
    while (exec() == QDialog::Accepted) {
        selectedGame = slGameIndex;
        selectedLevel = 0;	// In case the selection is invalid.
        if (myGameList.at (selectedGame)->owner == SYSTEM) {
            switch (slAction) {
            case SL_CREATE:	// Can save only in a USER collection.
            case SL_SAVE:
            case SL_MOVE:
                KGrMessage::information (slParent, i18n ("Select Level"),
                        i18n ("Sorry, you can only save or move "
                        "into one of your own games."));
                continue;			// Re-run the dialog box.
                break;
            case SL_DELETE:	// Can delete only in a USER collection.
                KGrMessage::information (slParent, i18n ("Select Level"),
                        i18n ("Sorry, you can only delete a level "
                        "from one of your own games."));
                continue;			// Re-run the dialog box.
                break;
            case SL_UPD_GAME:	// Can edit info only in a USER collection.
                KGrMessage::information (slParent, i18n ("Edit Game Info"),
                        i18n ("Sorry, you can only edit the game "
                        "information on your own games."));
                continue;			// Re-run the dialog box.
                break;
            default:
                break;
            }
        }

        selectedLevel = number->value();
        if ((selectedLevel > myGameList.at (selectedGame)->nLevels) &&
            (slAction != SL_CREATE) && (slAction != SL_SAVE) &&
            (slAction != SL_MOVE) && (slAction != SL_UPD_GAME)) {
            KGrMessage::information (slParent, i18n ("Select Level"),
                i18n ("There is no level %1 in \"%2\", "
                "so you cannot play or edit it.",
                 selectedLevel,
                 myGameList.at (selectedGame)->name));
            selectedLevel = 0;			// Set an invalid selection.
            continue;				// Re-run the dialog box.
        }
        break;					// Accepted and valid.
    }
    return (selectedLevel > 0);			// 0 = cancelled or invalid.
}

void KGrSLDialog::setupWidgets()
{
    int margin		= marginHint(); 
    int spacing		= spacingHint(); 
    QWidget * dad	= new QWidget (this);
    setMainWidget (dad);
    setCaption (i18n ("Select Game"));
    setButtons (KDialog::Ok | KDialog::Cancel | KDialog::Help);
    setDefaultButton (KDialog::Ok);

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    gameL    = new QLabel
                (i18n ("<html><b>Please select a game:</b></html>"), dad);
    mainLayout->addWidget (gameL, 5);

    games    = new QTreeWidget (dad);
    mainLayout->addWidget (games, 50);
    games->setColumnCount (4);
    games->setHeaderLabels (QStringList() <<
                            i18n ("Name of Game") <<
                            i18n ("Rules") <<
                            i18n ("Levels") <<
                            i18n ("Skill"));
    games->setRootIsDecorated (false);

    QHBoxLayout * hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing (6);
    hboxLayout1->setMargin (0);

    gameN    = new QLabel ("", dad);	// Name of selected game.
    QFont f = gameN->font();
    f.setBold (true);
    gameN->setFont (f);
    hboxLayout1->addWidget (gameN);

    QSpacerItem * spacerItem1 = new QSpacerItem
                        (21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout1->addItem (spacerItem1);

    gameD    = new QLabel ("", dad);		// Description of game.
    hboxLayout1->addWidget (gameD);
    mainLayout->addLayout (hboxLayout1, 5);

    gameAbout = new QTextEdit (dad);
    gameAbout->setReadOnly (true);
    mainLayout->addWidget (gameAbout, 25);

    QFrame * separator = new QFrame (dad);
    separator->setFrameShape (QFrame::HLine);
    mainLayout->addWidget (separator);

    if ((slAction == SL_START) || (slAction == SL_UPD_GAME)) {
        dad->	setWindowTitle (i18n ("Select Game"));
        QLabel * startMsg = new QLabel
            ("<b>" + i18n ("Level 1 of the selected game is:") + "</b>", dad);
        mainLayout->addWidget (startMsg, 5);
    }
    else {
        dad->	setWindowTitle (i18n ("Select Game/Level"));
        QLabel * selectLev = new QLabel
            ("<b>" + i18n ("Please select a level:") + "</b>", dad);
        mainLayout->addWidget (selectLev, 5);
    }

    QGridLayout * grid = new QGridLayout;
    mainLayout->addLayout (grid);

    number    = new QScrollBar (Qt::Vertical, dad);
    number->setRange (1, 150);
    number->setSingleStep (1);
    number->setPageStep (10);
    number->setValue (1);
    grid->addWidget (number, 1, 5, 4, 1);

    QWidget * numberPair = new QWidget (dad);
    QHBoxLayout *hboxLayout2 = new QHBoxLayout (numberPair);
    hboxLayout2->setMargin (0);
    numberPair->setLayout (hboxLayout2);
    grid->addWidget (numberPair, 1, 1, 1, 3);
    numberL   = new QLabel (i18n ("Level number:"), numberPair);
    display   = new KIntSpinBox (numberPair);
    display->setRange (1, 150);
    hboxLayout2->addWidget (numberL);
    hboxLayout2->addWidget (display);

    levelNH   = new QPushButton (i18n ("Edit Level Name && Hint"), dad);
    mainLayout->addWidget (levelNH);

    slName    = new QLabel ("", dad);
    grid->addWidget (slName, 2, 1, 1, 4);
    thumbNail = new KGrThumbNail (dad);
    grid->addWidget (thumbNail, 1, 6, 4, 5);

    // Set thumbnail cell size to about 1/5 of game cell size.
    int cellSize = slParent->width() / (5 * (FIELDWIDTH + 4));
    cellSize =  (cellSize < 4) ? 4 : cellSize;
    thumbNail->	setFixedWidth  ((FIELDWIDTH  * cellSize) + 2);
    thumbNail->	setFixedHeight ((FIELDHEIGHT * cellSize) + 2);

    // Base the geometry of the dialog box on the playing area.
    int cell =  slParent->width() / (FIELDWIDTH + 4);
    dad->	setMinimumSize ((FIELDWIDTH*cell/2), (FIELDHEIGHT-3)*cell);

    // Set the default for the level-number in the scrollbar.
    number->	setTracking (true);
    number->setValue (defaultLevel);

    slSetGames (defaultGame);

    // Vary the dialog according to the action.
    QString OKText = "";
    switch (slAction) {
    case SL_START:	// Must start at level 1, but can choose a game.
                        OKText = i18n ("Start Game");
                        number->setValue (1);
                        number->setEnabled (false);
                        display->setEnabled (false);
                        number->hide();
                        numberL->hide();
                        display->hide();
                        break;
    case SL_ANY:	// Can start playing at any level in any game.
                        OKText = i18n ("Play Level");
                        break;
    case SL_REPLAY:	// Can ask to see a replay of any level in any game.
                        OKText = i18n ("Replay Level");
                        break;
    case SL_SOLVE:	// Can ask to see a solution of any level in any game.
                        OKText = i18n ("Show Solution");
                        break;
    case SL_UPDATE:	// Can use any level in any game as edit input.
                        OKText = i18n ("Edit Level");
                        break;
    case SL_CREATE:	// Can save a new level only in a USER game.
                        OKText = i18n ("Save New");
                        break;
    case SL_SAVE:	// Can save an edited level only in a USER game.
                        OKText = i18n ("Save Change");
                        break;
    case SL_DELETE:	// Can delete a level only in a USER game.
                        OKText = i18n ("Delete Level");
                        break;
    case SL_MOVE:	// Can move a level only into a USER game.
                        OKText = i18n ("Move To...");
                        break;
    case SL_UPD_GAME:	// Can only edit USER game details.
                        OKText = i18n ("Edit Game Info");
                        number->setValue (1);
                        number->setEnabled (false);
                        display->setEnabled (false);
                        number->hide();
                        numberL->hide();
                        display->hide();
                        break;

    default:		break;			// Keep the default settings.
    }
    if (!OKText.isEmpty()) {
        setButtonGuiItem (KDialog::Ok, KGuiItem (OKText));
    }

    // Set value in the line-edit box.
    slShowLevel (number->value());

    if (display->isEnabled()) {
        display->setFocus();			// Set the keyboard input on.
        display->selectAll();
    }

    // Paint a thumbnail sketch of the level.
    thumbNail->setFrameStyle (QFrame::Box | QFrame::Plain);
    thumbNail->setLineWidth (1);
    slPaintLevel();
    thumbNail->show();

    connect (games,   SIGNAL (itemSelectionChanged()), this, SLOT (slGame()));

    connect (display, SIGNAL (valueChanged (const QString &)),
                this, SLOT (slUpdate (const QString &)));

    connect (number, SIGNAL(valueChanged (int)), this, SLOT(slShowLevel (int)));

    // Only enable name and hint dialog here if saving a new or edited level.
    // At other times the name and hint have not been loaded or initialised yet.
    if ((slAction == SL_CREATE) || (slAction == SL_SAVE)) {
        // Signal editNameAndHint() relays the click to a KGrEditor connection.
        connect (levelNH, SIGNAL (clicked()),
                 this,    SIGNAL (editNameAndHint()));
    }
    else {
        levelNH->setEnabled (false);
        levelNH->hide();
    }

    connect (games, SIGNAL(itemSelectionChanged()), this, SLOT(slPaintLevel()));
    connect (number,  SIGNAL (sliderReleased()), this, SLOT (slPaintLevel()));

    connect (this,    SIGNAL (helpClicked()), this, SLOT (slotHelp()));
}

/******************************************************************************/
/*****************    LOAD THE LIST OF GAMES (COLLECTIONS)    *****************/
/******************************************************************************/

void KGrSLDialog::slSetGames (int cIndex)
{
    int i;
    int imax = myGameList.count();

    // Set values in the table that holds details of available games.
    // The table is displayed in order of skill then the kind of rules.
    games->clear();
    slGameIndex = -1;

    QList<char> sortOrder1, sortOrder2;		// Crude, but effective.
    sortOrder1 << 'N' << 'C' << 'T';
    sortOrder2 << 'T' << 'K';

    foreach (char sortItem1, sortOrder1) {
        foreach (char sortItem2, sortOrder2) {
            for (i = 0; i < imax; i++) {
                if ((myGameList.at (i)->skill == sortItem1) &&
                    (myGameList.at (i)->rules == sortItem2)) {
                    QStringList data;
                    data
                        << myGameList.at (i)->name
                        << ((myGameList.at (i)->rules == 'K') ? 
                            i18nc ("Rules", "KGoldrunner") :
                            i18nc ("Rules", "Traditional"))
                        << QString().setNum (myGameList.at (i)->nLevels)
                        << ((myGameList.at (i)->skill == 'T') ? 
                            i18nc ("Skill Level", "Tutorial") :
                            ((myGameList.at (i)->skill == 'N') ? 
                            i18nc ("Skill Level", "Normal") :
                            i18nc ("Skill Level", "Championship")));
                    KGrGameListItem * thisGame = new KGrGameListItem (data, i);
                    games->addTopLevelItem (thisGame);

                    if (slGameIndex < 0) {
                        slGameIndex = i; // There is at least one game.
                    }
                    if (i == cIndex) {
                        // Mark the currently selected game (default 0).
                        games->setCurrentItem (thisGame);
                    }
                }
            } // End "for" loop.
        }
    }

    if (slGameIndex < 0) {
        return;				// The game-list is empty (unlikely).
    }

    // Fetch and display information on the selected game.
    slGame();

    // Make the column for the game's name a bit wider.
    games->header()->setResizeMode (0, QHeaderView::ResizeToContents);
    games->header()->setResizeMode (1, QHeaderView::ResizeToContents);
    games->header()->setResizeMode (2, QHeaderView::ResizeToContents);
}

/******************************************************************************/
/*****************    SLOTS USED BY LEVEL SELECTION DIALOG    *****************/
/******************************************************************************/

void KGrSLDialog::slGame()
{

    if (slGameIndex < 0) {
        // Ignore the "highlighted" signal caused by inserting in an empty box.
        return;
    }

    if (games->selectedItems().size() <= 0) {
        return;
    }

    slGameIndex = (dynamic_cast<KGrGameListItem *>
                        (games->selectedItems().first()))->id();
    int n = slGameIndex;				// Game selected.
    int N = defaultGame;				// Current game.
    if (myGameList.at (n)->nLevels > 0) {
        number->setMaximum (myGameList.at (n)->nLevels);
        display->setMaximum (myGameList.at (n)->nLevels);
    }
    else {
        number->setMaximum (1);			// Avoid range errors.
        display->setMaximum (1);
    }

    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    int lev = 1;

    // Set a default level number for the selected game.
    switch (slAction) {
    case SL_ANY:
    case SL_REPLAY:
    case SL_SOLVE:
    case SL_UPDATE:
    case SL_DELETE:
    case SL_UPD_GAME:
        // If selecting the current game, use the current level number.
        if (n == N) {
            number->setValue (defaultLevel);
        }
        // Else use the last level played in the selected game (from KConfig).
        else {
            lev = gameGroup.readEntry ("Level_" + myGameList.at (n)->prefix, 1);
            number->setValue (lev);			// Else use level 1.
        }
        break;
    case SL_CREATE:
    case SL_SAVE:
    case SL_MOVE:
        if ((n == N) && (slAction != SL_CREATE)) {
            // Saving/moving level in current game: use current number.
            number->setValue (defaultLevel);
        }
        else {
            // Saving new/edited level or relocating a level: use "nLevels + 1".
            number->setMaximum (myGameList.at (n)->nLevels + 1);
            display->setMaximum (myGameList.at (n)->nLevels + 1);
            number->setValue (number->maximum());
        }
        break;
    default:
        number->setValue (1);				// Default is level 1.
        break;
    }

    slShowLevel (number->value());

    int levCnt = myGameList.at (n)->nLevels;
    if (myGameList.at (n)->rules == 'K')
        gameD->setText (i18np ("1 level, uses KGoldrunner rules.",
                                "%1 levels, uses KGoldrunner rules.", levCnt));
    else
        gameD->setText (i18np ("1 level, uses Traditional rules.",
                                "%1 levels, uses Traditional rules.", levCnt));
    gameN->setText (myGameList.at (n)->name);
    QString s;
    if (myGameList.at (n)->about.isEmpty()) {
        s = i18n ("Sorry, there is no further information about this game.");
    }
    else {
        s = (i18n (myGameList.at (n)->about.constData()));
    } 
    gameAbout->setText (s);
}

void KGrSLDialog::slShowLevel (int i)
{
    // Display the level number as the slider is moved.
    display->setValue (i);
}

void KGrSLDialog::slUpdate (const QString & text)
{
    // Move the slider when a valid level number is entered.
    QString s = text;
    bool ok = false;
    int n = s.toInt (&ok);
    if (ok) {
        number->setValue (n);
        slPaintLevel();
    }
    else
        KGrMessage::information (this, i18n ("Select Level"),
                i18n ("This level number is not valid. It can not be used."));
}

void KGrSLDialog::slPaintLevel()
{
    // Repaint the thumbnail sketch of the level whenever the level changes.
    if (slGameIndex < 0) {
        return;					// Owner has no games.
    }
    // Fetch level-data and save layout, name and label in the thumbnail.
    QString dir = (myGameList.at (slGameIndex)->owner == USER) ?
                                  userDir : systemDir;
    thumbNail->setLevelData (dir, myGameList.at (slGameIndex)->prefix,
                                  number->value(), slName);
    thumbNail->repaint();			// Will call "paintEvent (e)".
}

void KGrSLDialog::slotHelp()
{
    // Help for "Select Game and Level" dialog box.
    QString s =
        i18n ("The main button at the bottom echoes the "
        "menu action you selected. Click it after choosing "
        "a game and level - or use \"Cancel\".");

    if (slAction == SL_START) {
        s += i18n ("\n\nIf this is your first time in KGoldrunner, select the "
            "tutorial game, which gives you hints as you go.\n\n"
            "Otherwise, just click on the name of a game in the table, "
            "then, to start at level 001, click on the main button at the "
            "bottom. Play begins when you move the mouse or press a key.");
   }
   else {
        switch (slAction) {
        case SL_UPDATE:
            s += i18n ("\n\nYou can select System levels for editing (or "
                "copying), but you must save the result in a game you have "
                "created.  Use the left mouse-button as a paintbrush and the "
                "editor toolbar buttons as a palette.  Use the 'Erase' button "
                "or the right mouse-button to erase.  You can drag the mouse "
                "with a button held down and paint or erase multiple squares.");
            break;
        case SL_CREATE:
            s += i18n("\n\nYou can add a name and hint to your new level here, "
                "but you must save the level you have created into one of "
                "your own games. By default your new level will go at the "
                "end of your game, but you can also select a level number and "
                "save into the middle of your game.");
            break;
        case SL_SAVE:
            s += i18n("\n\nYou can create or edit a name and hint here, before "
                "saving. If you change the game or level, you can do a copy "
                "or \"Save As\", but you must always save into one of your "
                "own games. If you save a level into the middle of a series, "
                "the other levels are automatically re-numbered.");
            break;
        case SL_DELETE:
            s += i18n ("\n\nYou can only delete levels from one of your own "
                "games. If you delete a level from the middle of a series, "
                "the other levels are automatically re-numbered.");
            break;
        case SL_MOVE:
            s += i18n ("\n\nTo move (re-number) a level, you must first select "
                "it by using \"Edit Any Level...\", then you can use "
                "\"Move Level...\" to move it to a new number or even a "
                "different game. Other levels are automatically re-numbered as "
                "required. You can only move levels within your own games.");
            break;
        case SL_UPD_GAME:
            s += i18n ("\n\nWhen editing game info you only need to choose a "
                "game, then you can go to a dialog where you edit the "
                "details of the game.");
            break;
        default:
            break;
        }
        s += i18n ("\n\nClick on the table to choose a game.  "
            "In the table and below it you can see more information about the "
            "selected game, including how many levels there are, how difficult "
            "the game is and what "
            "rules the enemies follow (see the KGoldrunner Handbook).\n\n"
            "You select "
            "a level number by typing it or using the spin box or scroll bar.  "
            "As you vary the game or level, the thumbnail area shows a "
            "preview of your choice.");
   }

   KGrMessage::information (slParent, i18n ("Help: Select Game & Level"), s);
}

/*******************************************************************************
*************************  ITEM FOR THE LIST OF GAMES  *************************
*******************************************************************************/

KGrGameListItem::KGrGameListItem (const QStringList & data,
                                  const int internalId)
        : QTreeWidgetItem (data)
{
    mInternalId = internalId;
}

int KGrGameListItem::id() const
{
    return mInternalId;
}

void KGrGameListItem::setId (const int internalId)
{
    mInternalId = internalId;
}

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

KGrThumbNail::KGrThumbNail (QWidget * parent)
    :
    QFrame (parent),
    io     (new KGrGameIO (parent))
{
    // Let the parent do all the work.  We need a class here so that
    // QFrame::paintEvent (QPaintEvent *) can be re-implemented and
    // the thumbnail can be automatically re-painted when required.
}

KGrThumbNail::~KGrThumbNail()
{
    delete io;
}

void KGrThumbNail::setLevelData (const QString & dir, const QString& prefix,
                                 int level, QLabel * sln)
{
    KGrLevelData d;
    QString filePath;

    IOStatus stat = io->fetchLevelData (dir, prefix, level, d, filePath);
    if (stat == OK) {
        // Keep a safe copy of the layout.  Translate and display the name.
        levelLayout = d.layout;
        sln->setText ((d.name.size() > 0) ? i18n ((const char *) d.name) : "");
    }
    else {
        // Level-data inaccessible or not found.
        levelLayout = "";
        sln->setText ("");
    }
}

void KGrThumbNail::paintEvent (QPaintEvent * /* event (unused) */)
{
    QPainter    p (this);
    QFile	openFile;
    QPen	pen = p.pen();
    char	obj = FREE;
    int		fw = 1;				// Set frame width.
    int		n = width() / FIELDWIDTH;	// Set thumbnail cell-size.

    QColor backgroundColor = QColor ("#000038"); // Midnight blue.
    QColor brickColor =      QColor ("#9c0f0f"); // Oxygen's brick-red.
    QColor concreteColor =   QColor ("#585858"); // Dark grey.
    QColor ladderColor =     QColor ("#a0a0a0"); // Steely grey.
    QColor poleColor =       QColor ("#a0a0a0"); // Steely grey.
    QColor heroColor =       QColor ("#00ff00"); // Green.
    QColor enemyColor =      QColor ("#0080ff"); // Bright blue.
    QColor gold;
    gold.setNamedColor ("gold");		 // Gold.

    pen.setColor (backgroundColor);
    p.setPen (pen);

    if (levelLayout.isEmpty()) {
        // There is no file, so fill the thumbnail with "FREE" cells.
        p.drawRect (QRect (fw, fw, FIELDWIDTH*n, FIELDHEIGHT*n));
        return;
    }

    for (int j = 0; j < FIELDHEIGHT; j++)
    for (int i = 0; i < FIELDWIDTH; i++) {

        obj = levelLayout.at (j*FIELDWIDTH + i);

        // Set the colour of each object.
        switch (obj) {
        case BRICK:
        case FBRICK:
            pen.setColor (brickColor); p.setPen (pen); break;
        case CONCRETE:
            pen.setColor (concreteColor); p.setPen (pen); break;
        case LADDER:
            pen.setColor (ladderColor); p.setPen (pen); break;
        case BAR:
            pen.setColor (poleColor); p.setPen (pen); break;
        case HERO:
            pen.setColor (heroColor); p.setPen (pen); break;
        case ENEMY:
            pen.setColor (enemyColor); p.setPen (pen); break;
        default:
            // Set the background colour for FREE, HLADDER and NUGGET.
            pen.setColor (backgroundColor); p.setPen (pen); break;
        }

        // Draw n x n pixels as n lines of length n.
        p.drawLine (i*n+fw, j*n+fw, i*n+(n-1)+fw, j*n+fw);
        if (obj == BAR) {
            // For a pole, only the top line is drawn in white.
            pen.setColor (backgroundColor);
            p.setPen (pen);
        }
        for (int k = 1; k < n; k++) {
            p.drawLine (i*n+fw, j*n+k+fw, i*n+(n-1)+fw, j*n+k+fw);
        }

        // For a nugget, add just a vertical touch  of yellow (2-3 pixels).
        if (obj == NUGGET) {
            int k = (n/2)+fw;
            pen.setColor (gold);	// Gold.
            p.setPen (pen);
            p.drawLine (i*n+k, j*n+k, i*n+k, j*n+(n-1)+fw);
            p.drawLine (i*n+k+1, j*n+k, i*n+k+1, j*n+(n-1)+fw);
        }
    }

    // Finally, draw a small black border around the outside of the thumbnail.
    pen.setColor (Qt::black); 
    p.setPen (pen);
    p.drawRect (rect().left(), rect().top(), rect().right(), rect().bottom());
}

#include "kgrselector.moc"
