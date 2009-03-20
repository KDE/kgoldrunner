/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/

#include "kgrdialog.h"

#include "kgrconsts.h" // OBSOLESCENT - 30/1/09
#include "kgrglobals.h"
#include "kgrcanvas.h"
#include "kgrgame.h"

#include <KGlobalSettings>
#include <QTextStream>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QHeaderView>

/******************************************************************************/
/*****************    DIALOG BOX TO SELECT A GAME AND LEVEL   *****************/
/******************************************************************************/

// TODO - Eliminate parameter 5 and implement refs to it some other way.
KGrSLDialog::KGrSLDialog (int action, int requestedLevel, int gameIndex,
                        QList<KGrGameData *> & gameList, /* KGrGame * theGame,*/
                        QWidget * parent)
                : KDialog (parent)
{
    slAction     = action;
    defaultLevel = requestedLevel;
    defaultGame  = gameIndex;
    myGameList   = gameList;
    // TODO - Remove ref to KGrGame * ... gameControl  = theGame;
    // selectedGame = myGameList.at (defaultGame); // OBSOLESCENT? - 31/1/09
    slParent     = parent;

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

    collnL    = new QLabel
                (i18n ("<html><b>Please select a game:</b></html>"), dad);
    mainLayout->addWidget (collnL, 5);

    colln     = new QTreeWidget (dad);
    mainLayout->addWidget (colln, 50);
    colln->setColumnCount (4);
    colln->setHeaderLabels (QStringList() <<
                            i18n ("Name of Game") <<
                            i18n ("Rules") <<
                            i18n ("Levels") <<
                            i18n ("Skill"));
    colln->setRootIsDecorated (false);

    QHBoxLayout * hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing (6);
    hboxLayout1->setMargin (0);

    collnN    = new QLabel ("", dad);	// Name of selected game.
    QFont f = collnN->font();
    f.setBold (true);
    collnN->setFont (f);
    hboxLayout1->addWidget (collnN);

    QSpacerItem * spacerItem1 = new QSpacerItem
                        (21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout1->addItem (spacerItem1);

    collnD    = new QLabel ("", dad);		// Description of game.
    hboxLayout1->addWidget (collnD);
    mainLayout->addLayout (hboxLayout1, 5);

    collnAbout = new QTextEdit (dad);
    collnAbout->setReadOnly (true);
    mainLayout->addWidget (collnAbout, 25);

    QFrame * separator = new QFrame (dad);
    separator->setFrameShape (QFrame::HLine);
    mainLayout->addWidget (separator);

    if ((action == SL_START) || (action == SL_UPD_GAME)) {
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

    number    = new QScrollBar (Qt::Vertical, dad); // IDW
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
    display   = new QSpinBox (numberPair);
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
    int cellSize = parent->width() / (5 * (FIELDWIDTH + 4));
    cellSize = (cellSize < 4) ? 4 : cellSize;
    thumbNail->	setFixedWidth  ((FIELDWIDTH  * cellSize) + 2);
    thumbNail->	setFixedHeight ((FIELDHEIGHT * cellSize) + 2);

    // Base the geometry of the dialog box on the playing area.
    int cell = parent->width() / (FIELDWIDTH + 4);
    dad->	setMinimumSize ((FIELDWIDTH*cell/2), (FIELDHEIGHT-3)*cell);

    // Set the default for the level-number in the scrollbar.
    number->	setTracking (true);
    number->setValue (requestedLevel);

    slSetCollections (defaultGame);

    // Vary the dialog according to the action.
    QString s1 = i18nc ("Default action at startup of game", "PLAY");
    QString s2 = i18nc ("Alternate action at startup of game", "Select Level");
    QString s3 = i18nc ("Alternate action at startup of game", "Use Menu");

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

    connect (colln,   SIGNAL (itemSelectionChanged()), this, SLOT (slColln()));

    connect (display, SIGNAL (valueChanged (const QString &)),
                this, SLOT (slUpdate (const QString &)));

    connect (number, SIGNAL(valueChanged (int)), this, SLOT(slShowLevel (int)));

    // Only enable name and hint dialog here if saving a new or edited level.
    // At other times the name and hint have not been loaded or initialised yet.
    if ((slAction == SL_CREATE) || (slAction == SL_SAVE)) {
        // TODO - Have to re-implement connection to editNameAndHint().
        // connect (levelNH,     SIGNAL (clicked()),
                 // gameControl, SLOT   (editNameAndHint()));
    }
    else {
        levelNH->setEnabled (false);
        levelNH->hide();
    }

    connect (colln, SIGNAL(itemSelectionChanged()), this, SLOT(slPaintLevel()));
    connect (number,  SIGNAL (sliderReleased()), this, SLOT (slPaintLevel()));

    connect (this,    SIGNAL (helpClicked()), this, SLOT (slotHelp()));
}

KGrSLDialog::~KGrSLDialog()
{
}

/******************************************************************************/
/*****************    LOAD THE LIST OF GAMES (COLLECTIONS)    *****************/
/******************************************************************************/

void KGrSLDialog::slSetCollections (int cIndex)
{
    int i;
    int imax = myGameList.count();

    // Set values in the list box that holds details of available games.
    // The list is displayed in order of skill then the kind of rules.
    colln->clear();
    slCollnIndex = -1;

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
                    colln->addTopLevelItem (thisGame);

                    if (slCollnIndex < 0) {
                        slCollnIndex = i; // There is at least one game.
                    }
                    if (i == cIndex) {
                        // Mark the currently selected game (default 0).
                        colln->setCurrentItem (thisGame);
                    }
                }
            } // End "for" loop.
        }
    }

    if (slCollnIndex < 0) {
        return;				// The game-list is empty (unlikely).
    }

    // Fetch and display information on the selected game.
    slColln();

    // Make the column for the game's name a bit wider.
    colln->header()->setResizeMode (0, QHeaderView::ResizeToContents);
    colln->header()->setResizeMode (1, QHeaderView::ResizeToContents);
    colln->header()->setResizeMode (2, QHeaderView::ResizeToContents);
}

/******************************************************************************/
/*****************    SLOTS USED BY LEVEL SELECTION DIALOG    *****************/
/******************************************************************************/

void KGrSLDialog::slColln()
{

    if (slCollnIndex < 0) {
        // Ignore the "highlighted" signal caused by inserting in an empty box.
        return;
    }

    if (colln->selectedItems().size() <= 0) {
        return;
    }

    slCollnIndex = (dynamic_cast<KGrGameListItem *>
                        (colln->selectedItems().first()))->id();
    int n = slCollnIndex;				// Game selected.
    int N = defaultGame;				// Current game.
    if (myGameList.at (n)->nLevels > 0) {
        number->setMaximum (myGameList.at (n)->nLevels);
        display->setMaximum (myGameList.at (n)->nLevels);
    }
    else {
        number->setMaximum (1);			// Avoid range errors.
        display->setMaximum (1);
    }

    // Set a default level number for the selected game.
    switch (slAction) {
    case SL_ANY:
    case SL_UPDATE:
    case SL_DELETE:
    case SL_UPD_GAME:
        // If selecting the current game, use the current level number.
        if (n == N)
            number->setValue (defaultLevel);
        else
            number->setValue (1);			// Else use level 1.
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
        collnD->setText (i18np ("1 level, uses KGoldrunner rules.",
                                "%1 levels, uses KGoldrunner rules.", levCnt));
    else
        collnD->setText (i18np ("1 level, uses Traditional rules.",
                                "%1 levels, uses Traditional rules.", levCnt));
    collnN->setText (myGameList.at (n)->name);
    QString s;
    if (myGameList.at (n)->about.isEmpty()) {
        s = i18n ("Sorry, there is no further information about this game.");
    }
    else {
        s = (i18n (myGameList.at (n)->about.constData()));
    } 
    collnAbout->setText (s);
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
    int n = slCollnIndex;
    if (n < 0) {
        return;					// Owner has no games.
    }
    // Fetch level-data and save layout, name and label in the thumbnail.
    // TODO - Revive the thumbnail display.
    // TODO - Have to find some other way of getting the directory's filepath.
    // QString	dir = gameControl->getDirectory (myGameList.at (n)->owner);
    // thumbNail->setLevelData (dir, myGameList.at (n)->prefix,
                                // number->value(), slName);
    // thumbNail->repaint();			// Will call "paintEvent (e)".
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
            "tutorial game or click \"Cancel\" and click that item in "
            "the Game or Help menu. The tutorial game gives you hints "
            "as you go.\n\n"
            "Otherwise, just click on the name of a game (in the list box), "
            "then, to start at level 001, click on the main button at the "
            "bottom. Play begins when you move the mouse or press a key.");
   }
   else {
        switch (slAction) {
        case SL_UPDATE:
            s += i18n ("\n\nYou can select System levels for editing (or "
                "copying), but you must save the result in a game you have "
                "created. Use the mouse as a paintbrush and the editor "
                "toolbar buttons as a palette. Use the 'Empty Space' button "
                "to erase.");
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
                "\"Move Level...\" to assign it a new number or even a different "
                "game. Other levels are automatically re-numbered as "
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
        s += i18n ("\n\nClick on the list box to choose a game.  "
            "Below the list box you can see more information about the "
            "selected game, including how many levels there are and what "
            "rules the enemies follow (see the Settings menu).\n\n"
            "You select "
            "a level number by typing it or using the scroll bar.  As "
            "you vary the game or level, the thumbnail area shows a "
            "preview of your choice.");
   }

   KGrMessage::information (slParent, i18n ("Help: Select Game & Level"), s);
}

/*******************************************************************************
*************** DIALOG BOX TO CREATE/EDIT A LEVEL NAME AND HINT ****************
*******************************************************************************/

KGrNHDialog::KGrNHDialog (const QString & levelName, const QString & levelHint,
                        QWidget * parent)
                : KDialog (parent)
{
    setCaption (i18n ("Edit Name & Hint"));
    setButtons (KDialog::Ok | KDialog::Cancel);
    setDefaultButton (KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget (this);
    setMainWidget (dad);

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QLabel *		nameL  = new QLabel (i18n ("Name of level:"), dad);
    mainLayout->addWidget (nameL);
                        nhName  = new QLineEdit (dad);
    mainLayout->addWidget (nhName);

    QLabel *		mleL = new QLabel (i18n ("Hint for level:"), dad);
    mainLayout->addWidget (mleL);

    // Set up a widget to hold the wrapped text, using \n for paragraph breaks.
                         mle = new QTextEdit (dad);
     mle->		setAcceptRichText (false);
     mainLayout->addWidget (mle);

    // Base the geometry of the text box on the playing area.
    QPoint		p = parent->mapToGlobal (QPoint (0,0));
    int			c = parent->width() / (FIELDWIDTH + 4);
    dad->		move (p.x()+4*c, p.y()+4*c);
    mle->		setMinimumSize ((FIELDWIDTH*c/2), (FIELDHEIGHT/2)*c);

    // Configure the text box.
    mle->		setAlignment (Qt::AlignLeft);

    nhName->		setText (levelName);
    mle->		setText (levelHint);
}

KGrNHDialog::~KGrNHDialog()
{
}

/*******************************************************************************
*************** DIALOG BOX TO CREATE OR EDIT A GAME (COLLECTION) ***************
*******************************************************************************/

KGrECDialog::KGrECDialog (int action, int gameIndex,
                        QList<KGrGameData *> & gamesList,
                        QWidget * parent)
                : KDialog (parent)
{
    myGameList  = gamesList;
    defaultGame  = gameIndex;

    setCaption (i18n ("Edit Game Info"));
    setButtons (KDialog::Ok | KDialog::Cancel);
    setDefaultButton (KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget (this);
    setMainWidget (dad);

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QHBoxLayout *hboxLayout5 = new QHBoxLayout();
    hboxLayout5->setSpacing (spacing);
    nameL    = new QLabel (i18n ("Name of game:"), dad);
    hboxLayout5->addWidget (nameL);
    ecName   = new QLineEdit (dad);
    hboxLayout5->addWidget (ecName);
    mainLayout->addLayout (hboxLayout5);

    QHBoxLayout *hboxLayout6 = new QHBoxLayout();
    hboxLayout6->setSpacing (spacing);
    prefixL  = new QLabel (i18n ("File name prefix:"), dad);
    hboxLayout6->addWidget (prefixL);
    ecPrefix = new QLineEdit (dad);
    hboxLayout6->addWidget (ecPrefix);
    mainLayout->addLayout (hboxLayout6);

    //In Qt4, QButtonGroup is no longer a widget...
    ecGrp    = new QButtonGroup (dad);
    ecTradB  = new QRadioButton (i18n ("Traditional rules"), dad);
    ecKGrB   = new QRadioButton (i18n ("KGoldrunner rules"), dad);
    ecGrp->addButton (ecTradB);
    ecGrp->addButton (ecKGrB);

    //..so we need to add the radio buttons directly to the layout
    mainLayout->addWidget (ecTradB);
    mainLayout->addWidget (ecKGrB);


    nLevL    = new QLabel (i18np ("1 level", "%1 levels", 0), dad);
    mainLayout->addWidget (nLevL);

    mleL     = new QLabel (i18n ("About this game:"), dad);
    mainLayout->addWidget (mleL);

    // Set up a widget to hold the wrapped text, using \n for paragraph breaks.
    mle	     = new QTextEdit (dad);
    mle->    setAcceptRichText (false);
    mainLayout->addWidget (mle);

    QPoint p = parent->mapToGlobal (QPoint (0,0));

    // Base the geometry of the dialog box on the playing area.
    int cell = qMin(parent->height() / (FIELDHEIGHT + 4), parent->width() / FIELDWIDTH + 4);
    dad->	move (p.x()+2*cell, p.y()+2*cell);
    dad->	setMinimumSize ((FIELDWIDTH*cell/2), (FIELDHEIGHT-1)*cell);

    if (action == SL_CR_GAME) {
         setCaption (i18n ("Create Game"));
    }
    else {
         setCaption (i18n ("Edit Game Info"));
    }

    QString OKText = "";
    if (action == SL_UPD_GAME) {		// Edit existing game.
        ecName->	setText (myGameList.at (defaultGame)->name);
        ecPrefix->	setText (myGameList.at (defaultGame)->prefix);
        if (myGameList.at (defaultGame)->nLevels > 0) {
            // Game already has some levels, so cannot change the prefix.
            ecPrefix->	setEnabled (false);
        }
        QString		s;
        nLevL->		setText (i18np ("1 level", "%1 levels",
                                        myGameList.at (defaultGame)->nLevels));
        OKText = i18n ("Save Changes");
    }
    else {					// Create a game.
        ecName->        setText ("");
        ecPrefix->      setText ("");
        nLevL->         setText (i18n ("0 levels"));
        OKText = i18n ("Save New");
    }
    setButtonGuiItem (KDialog::Ok, KGuiItem (OKText));

    if ((action == SL_CR_GAME) ||
        (myGameList.at (defaultGame)->rules == 'T')) {
        ecSetRules ('T');			// Traditional rules.
    }
    else {
        ecSetRules ('K');			// KGoldrunner rules.
    }

    // Configure the edit box.
    mle->		setAlignment (Qt::AlignLeft);

    if ((action == SL_UPD_GAME) &&
        (myGameList.at (defaultGame)->about.length() > 0)) {
        // Display and edit the game description in its original language.
        mle->		setText (myGameList.at (defaultGame)->about);
    }
    else {
        mle->		setText ("");
    }

    connect (ecKGrB,  SIGNAL (clicked()), this, SLOT (ecSetKGr()));
    connect (ecTradB, SIGNAL (clicked()), this, SLOT (ecSetTrad()));
}

KGrECDialog::~KGrECDialog()
{
}

void KGrECDialog::ecSetRules (const char rules)
{
    ecKGrB->	setChecked (false);
    ecTradB->	setChecked (false);
    if (rules == 'K')
        ecKGrB->	setChecked (true);
    else
        ecTradB->	setChecked (true);
}

void KGrECDialog::ecSetKGr()  {ecSetRules ('K');}	// Radio button slots.
void KGrECDialog::ecSetTrad() {ecSetRules ('T');}

/*******************************************************************************
***************  DIALOG TO SELECT A SAVED GAME TO BE RE-LOADED  ****************
*******************************************************************************/

KGrLGDialog::KGrLGDialog (QFile * savedGames,
                          QList<KGrGameData *> & gameList,
                          QWidget * parent)
                : KDialog (parent)
{
    setCaption (i18n ("Select Saved Game"));
    setButtons (KDialog::Ok | KDialog::Cancel);
    setDefaultButton (KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget (this);
    setMainWidget (dad);

    QVBoxLayout *	mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing (spacing);
    mainLayout->setMargin (margin);

    QLabel *		lgHeader = new QLabel (
                        i18n ("Game                       Level/Lives/Score   "
                        "Day    Date     Time  "), dad);

    lgList   = new QListWidget (dad);
    QFont		f = KGlobalSettings::fixedFont();	// KDE version.
                        f.setFixedPitch (true);
    lgList->		setFont (f);
                        f.setBold (true);
    lgHeader->		setFont (f);

    mainLayout->	addWidget (lgHeader);
    mainLayout->	addWidget (lgList);

    // Base the geometry of the list box on the playing area.
    QPoint		p = parent->mapToGlobal (QPoint (0,0));
    int			c = parent->width() / (FIELDWIDTH + 4);
    dad->		move (p.x()+2*c, p.y()+2*c);
    lgList->		setMinimumHeight ((FIELDHEIGHT/2)*c);

                        lgHighlight  = -1;

    QTextStream		gameText (savedGames);
    QString		s = "";
    QString		pr = "";
    int			i;
    int			imax = gameList.count();

    // Read the saved games into the list box.
    while (! gameText.endData()) {
        s = gameText.readLine();		// Read in one saved game.
        pr = s.left (s.indexOf (" ", 0,
                        Qt::CaseInsensitive));	// Get the game prefix.
        for (i = 0; i < imax; i++) {		// Get the game name.
            if (gameList.at (i)->prefix == pr) {
                s = s.insert (0,
                gameList.at (i)->name.leftJustified (20, ' ', true) + ' ');
                break;
            }
        }
        lgList-> addItem (s);
    }
    savedGames->close();

    // Mark row 0 (the most recently saved game) as the default selection.
    lgList->	setSelectionMode (QAbstractItemView::SingleSelection);
    lgList->	setCurrentRow (0);
    lgList->	setItemSelected  (lgList->currentItem(), true);
                lgHighlight = 0;

    connect (lgList, SIGNAL (itemClicked (QListWidgetItem *)),
                this, SLOT (lgSelect (QListWidgetItem *)));
}

void KGrLGDialog::lgSelect (QListWidgetItem * item)
{
    lgHighlight = lgList->row (item);
}

/*******************************************************************************
***********************  CENTRALIZED MESSAGE FUNCTIONS  ************************
*******************************************************************************/

void KGrMessage::information (QWidget * parent,
                        const QString &caption, const QString &text)
{
    // KDE does word-wrapping and will observe "\n" line-breaks.
    KMessageBox::information (parent, text, caption);
}

int KGrMessage::warning (QWidget * parent, const QString &caption,
                        const QString &text, const QString &label0,
                        const QString &label1, const QString &label2)
{
    // KDE does word-wrapping and will observe "\n" line-breaks.
    int ans = 0;
    if (label2.isEmpty()) {
        // Display a box with 2 buttons.
        ans = KMessageBox::questionYesNo (parent, text, caption,
                            KGuiItem (label0), KGuiItem (label1));
        ans = (ans == KMessageBox::Yes) ? 0 : 1;
    }
    else {
        // Display a box with 3 buttons.
        ans = KMessageBox::questionYesNoCancel (parent, text, caption,
                            KGuiItem (label0), KGuiItem (label1));
        if (ans == KMessageBox::Cancel)
            ans = 2;
        else
            ans = (ans == KMessageBox::Yes) ? 0 : 1;
    }
    return (ans);
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

#include "kgrdialog.moc"
