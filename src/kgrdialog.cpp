/***************************************************************************
    Copyright 2003 Marco Krger
    Copyright 2003 Ian Wadham <ianw@netspace.net.au>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifdef KGR_PORTABLE
// If compiling for portability, redefine KDE's i18n.
#define i18n tr
#endif

#include "kgrconsts.h"
#include "kgrcanvas.h"
#include "kgrgame.h"
#include "kgrdialog.h"

#ifndef KGR_PORTABLE
#include <kglobalsettings.h>
#include <QTextStream>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QSpacerItem>
#endif

/******************************************************************************/
/*****************    DIALOG BOX TO SELECT A GAME AND LEVEL   *****************/
/******************************************************************************/

#ifdef KGR_PORTABLE
KGrSLDialog::KGrSLDialog (int action, int requestedLevel, int collnIndex,
			QList<KGrCollection *> & gamesList, KGrGame * theGame,
			QWidget * parent, const char * name)
		: QDialog (parent, name, true,
			Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title)
#else
KGrSLDialog::KGrSLDialog (int action, int requestedLevel, int collnIndex,
			QList<KGrCollection *> & gamesList, KGrGame * theGame,
			QWidget * parent, const char * name)
		: KDialog (parent)
#endif
{
    slAction     = action;
    defaultLevel = requestedLevel;
    defaultGame  = collnIndex;
    collections  = gamesList;
    game         = theGame;
    collection   = collections.at(defaultGame);
    slParent     = parent;

#ifdef KGR_PORTABLE
    int margin		= 10;
    int spacing		= 10;
    QWidget * dad	= this;
#else
    int margin		= marginHint(); 
    int spacing		= spacingHint(); 
    QWidget * dad	= new QWidget(this);
	setMainWidget(dad);
	setCaption(i18n("Select Game"));
	setButtons(KDialog::Ok | KDialog::Cancel | KDialog::Help);
	setDefaultButton(KDialog::Ok);

#endif

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QHBoxLayout * hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);

    collnL    = new QLabel (i18n("List of games:"), dad);
    hboxLayout->addWidget (collnL);

    QSpacerItem * spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem);

    mainLayout->addLayout(hboxLayout);

    colln     = new QListWidget (dad);
    mainLayout->addWidget (colln);

    QHBoxLayout * hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing(6);
    hboxLayout1->setMargin(0);

    collnN    = new QLabel ("", dad);	// Name of selected collection.
    QFont f = collnN->font();
    f.setBold (true);
    collnN->setFont (f);

    hboxLayout1->addWidget(collnN);

    QSpacerItem * spacerItem1 = new QSpacerItem(21, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    hboxLayout1->addItem(spacerItem1);

    collnA = new QPushButton(i18n("More Info"), dad);
    hboxLayout1->addWidget(collnA);

    mainLayout->addLayout(hboxLayout1);

    collnD    = new QLabel ("", dad);		// Description of collection.
    mainLayout->addWidget (collnD);


    QFrame * separator = new QFrame (dad);
    separator->setFrameShape(QFrame::HLine);
    mainLayout->addWidget (separator);

    if ((action == SL_START) || (action == SL_UPD_GAME)) {
	dad->	setWindowTitle (i18n("Select Game"));
	QLabel * startMsg = new QLabel
	    ("<b>" + i18n("Level 1 of the selected game is:") + "</b>", dad);
	mainLayout->addWidget (startMsg);
    }
    else {
	dad->	setWindowTitle (i18n("Select Game/Level"));
	QLabel * selectLev = new QLabel (i18n("Select level:"), dad);
	mainLayout->addWidget (selectLev);
    }

    QGridLayout * grid = new QGridLayout;
    mainLayout->addLayout (grid);

    number    = new QScrollBar (Qt::Horizontal, dad);
    number->setRange(1, 150);
    number->setSingleStep(1);
    number->setPageStep(10);
    number->setValue(1);
    grid->addWidget (number, 1, 1);

    QWidget * numberPair = new QWidget(dad);
    QHBoxLayout *hboxLayout2 = new QHBoxLayout(numberPair);
    numberPair->setLayout(hboxLayout2);
    grid->addWidget (numberPair, 2, 1);
    hboxLayout2->setSpacing (spacing);
    numberL   = new QLabel (i18n("Level number:"), numberPair);
    display   = new QLineEdit (numberPair);

    levelNH   = new QPushButton (i18n("Edit Level Name && Hint"), dad);
    mainLayout->addWidget (levelNH);

    slName    = new QLabel ("", dad);
    grid->addWidget (slName, 3, 1);
    thumbNail = new KGrThumbNail (dad);
    grid->addWidget (thumbNail, 1, 2, 3, 1);

    // Set thumbnail cell size to about 1/5 of game cell size.
    int cellSize = parent->width() / (5 * (FIELDWIDTH + 4));
    thumbNail->	setFixedWidth  ((FIELDWIDTH  * cellSize) + 2);
    thumbNail->	setFixedHeight ((FIELDHEIGHT * cellSize) + 2);

#ifdef KGR_PORTABLE
    QWidget * buttons = new QWidget(this);
    QHBoxLayout *hboxLayout3 = new QHBoxLayout(buttons);
    buttons->setLayout(hboxLayout3);
    mainLayout->addWidget (buttons);
    buttons->setSpacing (spacing);
    // Buttons are for Qt-only portability.  NOT COMPILED in KDE environment.
    HELP      = new QPushButton (i18n("Help"), buttons);
    OK        = new QPushButton (i18n("&OK"), buttons);
    CANCEL    = new QPushButton (i18n("&Cancel"), buttons);

    QPoint p  = parent->mapToGlobal (QPoint (0,0));

    // Base the geometry of the dialog box on the playing area.
    int cell = parent->width() / (FIELDWIDTH + 4);
    dad->	move (p.x()+2*cell, p.y()+2*cell);
    dad->	setMinimumSize ((FIELDWIDTH*cell/2), (FIELDHEIGHT-1)*cell);

    OK->	setAccel (Qt::Key_Return);
    HELP->	setAccel (Qt::Key_F1);
    CANCEL->	setAccel (Qt::Key_Escape);
#endif

    // Set the default for the level-number in the scrollbar.
    number->	setTracking (true);
    number->setValue (requestedLevel);

    slSetCollections (defaultGame);

    // Vary the dialog according to the action.
    QString OKText = "";
    switch (slAction) {
    case SL_START:	// Must start at level 1, but can choose a collection.
			OKText = i18n("Start Game");
			number->setValue (1);
			number->setEnabled(false);
			display->setEnabled(false);
			number->hide();
			numberL->hide();
			display->hide();
			break;
    case SL_ANY:	// Can start playing at any level in any collection.
			OKText = i18n("Play Level");
			break;
    case SL_UPDATE:	// Can use any level in any collection as edit input.
			OKText = i18n("Edit Level");
			break;
    case SL_CREATE:	// Can save a new level only in a USER collection.
			OKText = i18n("Save New");
			break;
    case SL_SAVE:	// Can save an edited level only in a USER collection.
			OKText = i18n("Save Change");
			break;
    case SL_DELETE:	// Can delete a level only in a USER collection.
			OKText = i18n("Delete Level");
			break;
    case SL_MOVE:	// Can move a level only into a USER collection.
			OKText = i18n("Move To...");
			break;
    case SL_UPD_GAME:	// Can only edit USER collection details.
			OKText = i18n("Edit Game Info");
			number->setValue (1);
			number->setEnabled(false);
			display->setEnabled(false);
			number->hide();
			numberL->hide();
			display->hide();
			break;

    default:		break;			// Keep the default settings.
    }
    if (!OKText.isEmpty()) {
#ifdef KGR_PORTABLE
	OK->setText (OKText);
#else
	setButtonGuiItem( KDialog::Ok, KGuiItem(OKText));
#endif
    }

    // Set value in the line-edit box.
    slShowLevel (number->value());

    if (display->isEnabled()) {
	display->setFocus();			// Set the keyboard input on.
	display->selectAll();
	display->setCursorPosition (0);
    }

    // Paint a thumbnail sketch of the level.
    thumbNail->setFrameStyle (QFrame::Box | QFrame::Plain);
    thumbNail->setLineWidth (1);
    slPaintLevel();
    thumbNail->show();

    connect (colln,   SIGNAL (itemClicked (QListWidgetItem *)), this, SLOT (slColln (QListWidgetItem *)));
    connect (collnA,  SIGNAL (clicked ()), this, SLOT (slAboutColln ()));

    connect (display, SIGNAL (textChanged (const QString &)),
		this, SLOT (slUpdate (const QString &)));

    connect (number,  SIGNAL (valueChanged(int)), this, SLOT(slShowLevel(int)));

    // Only enable name and hint dialog here if saving a new or edited level.
    // At other times the name and hint have not been loaded or initialised yet.
    if ((slAction == SL_CREATE) || (slAction == SL_SAVE)) {
	connect (levelNH,  SIGNAL (clicked()), game, SLOT (editNameAndHint()));
    }
    else {
	levelNH->setEnabled (false);
	levelNH->hide();
    }

    connect (colln,   SIGNAL (itemClicked (QListWidgetItem *)), this, SLOT (slPaintLevel ()));
    connect (number,  SIGNAL (sliderReleased()), this, SLOT (slPaintLevel()));

#ifdef KGR_PORTABLE
    // Set the exits from this dialog box.
    connect (OK,      SIGNAL (clicked ()), this,   SLOT (accept ()));
    connect (CANCEL,  SIGNAL (clicked ()), this,   SLOT (reject ()));
    connect (HELP,    SIGNAL (clicked ()), this,   SLOT (slotHelp ()));
#endif
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
    int imax = collections.count();

    // Set values in the combo box that holds collection names.
    colln->clear();
    slCollnIndex = -1;

    for (i = 0; i < imax; i++) {
	colln->addItem(collections.at(i)->name);
	if (slCollnIndex < 0) {
	    slCollnIndex = i;		// There is at least one collection.
	}
    }

    if (slCollnIndex < 0) {
	return;				// There are no collections (unlikely).
    }
    // Mark the currently selected collection (or default 0).
    colln->setCurrentRow(cIndex);
    colln->setItemSelected ( colln->item (cIndex), true );

    // Fetch and display information on the selected collection.
    slColln ( colln->item (cIndex) );
}

/******************************************************************************/
/*****************    SLOTS USED BY LEVEL SELECTION DIALOG    *****************/
/******************************************************************************/

void KGrSLDialog::slColln (QListWidgetItem * item)
{

    if (slCollnIndex < 0) {
	// Ignore the "highlighted" signal caused by inserting in an empty box.
	return;
    }
    int i = colln->row ( item );
    // User "highlighted" a new collection (with one click) ...
    colln->setItemSelected ( item, true );  // One click = selected

    slCollnIndex = i;
    int n = slCollnIndex;				// Collection selected.
    int N = defaultGame;				// Current collection.
    if (collections.at(n)->nLevels > 0)
	number->setMaximum (collections.at(n)->nLevels);
    else
	number->setMaximum (1);			// Avoid range errors.

    // Set a default level number for the selected collection.
    switch (slAction) {
    case SL_ANY:
    case SL_UPDATE:
    case SL_DELETE:
    case SL_UPD_GAME:
	// If selecting the current collection, use the current level number.
	if (n == N)
	    number->setValue (defaultLevel);
	else
	    number->setValue (1);			// Else use level 1.
	break;
    case SL_CREATE:
    case SL_SAVE:
    case SL_MOVE:
	if ((n == N) && (slAction != SL_CREATE)) {
	    // Saving/moving level in current collection: use current number.
	    number->setValue (defaultLevel);
	}
	else {
	    // Saving new/edited level or relocating a level: use "nLevels + 1".
	    number->setMaximum (collections.at(n)->nLevels + 1);
	    number->setValue (number->maximum());
	}
	break;
    default:
	number->setValue (1);				// Default is level 1.
	break;
    }

    slShowLevel (number->value());

#ifndef KGR_PORTABLE
    int levCnt = collections.at(n)->nLevels;
    if (collections.at(n)->settings == 'K')
	collnD->setText (i18np("1 level, uses KGoldrunner rules.",
				"%n levels, uses KGoldrunner rules.", levCnt));
    else
	collnD->setText (i18np("1 level, uses Traditional rules.",
				"%n levels, uses Traditional rules.", levCnt));
#else
    QString levCnt;
    levCnt = levCnt.setNum (collections.at(n)->nLevels);
    if (collections.at(n)->settings == 'K')
	collnD->setText (levCnt + i18n(" levels, uses KGoldrunner rules."));
    else
	collnD->setText (levCnt + i18n(" levels, uses Traditional rules."));
#endif
    collnN->setText (collections.at(n)->name);
}

void KGrSLDialog::slAboutColln ()
{
    // User clicked the "About" button ...
    int		n = slCollnIndex;
    QString	title = i18n("About \"%1\"", collections.at(n)->name);

    if (collections.at(n)->about.length() > 0) {
	// Convert game description to ASCII and UTF-8 codes, then translate it.
	KGrMessage::wrapped (slParent, title,
			i18n(collections.at(n)->about.toUtf8().constData()));
    }
    else {
	KGrMessage::wrapped (slParent, title,
	    i18n("Sorry, there is no further information about this game."));
    }
}

void KGrSLDialog::slShowLevel (int i)
{
    // Display the level number as the slider is moved.
    QString tmp;
    tmp.setNum(i);
    tmp = tmp.rightJustified(3,'0');
    display->setText(tmp);
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
	KGrMessage::information (this, i18n("Select Level"),
		i18n("This level number is not valid. It can not be used."));
}

void KGrSLDialog::slPaintLevel ()
{
    // Repaint the thumbnail sketch of the level whenever the level changes.
    int n = slCollnIndex;
    if (n < 0) {
	return;					// Owner has no collections.
    }
    QString	filePath = game->getFilePath
		(collections.at(n)->owner, collections.at(n), number->value());
    thumbNail->setFilePath (filePath, slName);
    thumbNail->repaint();			// Will call "drawContents (p)".
}

void KGrSLDialog::slotHelp ()
{
    // Help for "Select Game and Level" dialog box.
    QString s =
	i18n("The main button at the bottom echoes the "
	"menu action you selected. Click it after choosing "
	"a game and level - or use \"Cancel\".");

    if (slAction == SL_START) {
	s += i18n("\n\nIf this is your first time in KGoldrunner, select the "
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
	    s += i18n("\n\nYou can select System levels for editing (or "
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
	    s += i18n("\n\nYou can only delete levels from one of your own "
		 "games. If you delete a level from the middle of a series, "
		 "the other levels are automatically re-numbered.");
	    break;
	case SL_MOVE:
	    s += i18n("\n\nTo move (re-number) a level, you must first select "
		 "it by using \"Edit Any Level...\", then you can use "
		 "\"Move Level...\" to assign it a new number or even a different "
		 "game. Other levels are automatically re-numbered as "
		 "required. You can only move levels within your own games.");
	    break;
	case SL_UPD_GAME:
	    s += i18n("\n\nWhen editing game info you only need to choose a "
		 "game, then you can go to a dialog where you edit the "
		 "details of the game.");
	    break;
	default:
	    break;
	}
	s += i18n("\n\nClick on the list box to choose a game.  "
	     "Below the list box you can see \"More Info\" about the "
	     "selected game, how many levels there are and what "
	     "rules the enemies follow (see the Settings menu).\n\n"
	     "You select "
	     "a level number by typing it or using the scroll bar.  As "
	     "you vary the game or level, the thumbnail area shows a "
	     "preview of your choice.");
    }

    KGrMessage::wrapped (slParent, i18n("Help: Select Game & Level"), s);
}

/*******************************************************************************
*************** DIALOG BOX TO CREATE/EDIT A LEVEL NAME AND HINT ****************
*******************************************************************************/

#ifdef KGR_PORTABLE
KGrNHDialog::KGrNHDialog(const QString & levelName, const QString & levelHint,
			QWidget * parent, const char * name)
		: QDialog (parent, name, true,
			Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title)
#else
KGrNHDialog::KGrNHDialog(const QString & levelName, const QString & levelHint,
			QWidget * parent, const char * name)
		: KDialog (parent)
#endif
{
#ifdef KGR_PORTABLE
    int margin		= 10;
    int spacing		= 10;
    QWidget * dad	= this;
#else
	setCaption(i18n("Edit Name & Hint"));
	setButtons(KDialog::Ok | KDialog::Cancel);
	setDefaultButton(KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget(this);
	setMainWidget(dad);
#endif

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QLabel *		nameL  = new QLabel (i18n("Name of level:"), dad);
    mainLayout->addWidget (nameL);
			nhName  = new QLineEdit (dad);
    mainLayout->addWidget (nhName);

    QLabel *		mleL = new QLabel (i18n("Hint for level:"), dad);
    mainLayout->addWidget (mleL);

   // Set up a widget to hold the wrapped text, using \n for paragraph breaks.
			mle = new QTextEdit (dad);
    mle->		setTextFormat (Qt::PlainText);
    mainLayout->addWidget (mle);

#ifdef KGR_PORTABLE
    QWidget * buttons = new QWidget(dad);
    QHBoxLayout *hboxLayout4 = new QHBoxLayout(buttons);
    buttons->setLayout(hboxLayout4);
    mainLayout->addWidget (buttons);
    buttons->setSpacing (spacing);
    // Buttons are for Qt-only portability.  NOT COMPILED in KDE environment.
    QPushButton *	OK = new QPushButton (i18n("&OK"), buttons);
    QPushButton *	CANCEL = new QPushButton (i18n("&Cancel"), buttons);

    dad->		setCaption (i18n("Edit Name & Hint"));
#endif

    // Base the geometry of the text box on the playing area.
    QPoint		p = parent->mapToGlobal (QPoint (0,0));
    int			c = parent->width() / (FIELDWIDTH + 4);
    dad->		move (p.x()+4*c, p.y()+4*c);
    mle->		setMinimumSize ((FIELDWIDTH*c/2), (FIELDHEIGHT/2)*c);

    // Configure the text box.
    mle->		setAlignment (Qt::AlignLeft);

    nhName->		setText (levelName);
    mle->		setText (levelHint);

#ifdef KGR_PORTABLE
    // OK->		setAccel (Key_Return);	// No!  We need it in "mle" box.
    CANCEL->		setAccel (Qt::Key_Escape);

    connect (OK, SIGNAL (clicked ()), dad, SLOT (accept ()));
    connect (CANCEL, SIGNAL (clicked ()), dad, SLOT (reject ()));
#endif
}

KGrNHDialog::~KGrNHDialog()
{
}

/*******************************************************************************
*************** DIALOG BOX TO CREATE OR EDIT A GAME (COLLECTION) ***************
*******************************************************************************/

#ifdef KGR_PORTABLE
KGrECDialog::KGrECDialog (int action, int collnIndex,
			QList<KGrCollection *> & gamesList,
			QWidget * parent, const char * name)
		: QDialog (parent, name, true,
			Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title)
#else
KGrECDialog::KGrECDialog (int action, int collnIndex,
			QList<KGrCollection *> & gamesList,
			QWidget * parent, const char * name)
		: KDialog(parent)
#endif
{
    collections  = gamesList;
    defaultGame  = collnIndex;

#ifdef KGR_PORTABLE
    int margin		= 10;
    int spacing		= 10;
    QWidget * dad	= this;
#else
	setCaption(i18n("Edit Game Info"));
	setButtons(KDialog::Ok | KDialog::Cancel);
	setDefaultButton(KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget(this);
	setMainWidget(dad);
#endif

    QVBoxLayout * mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QHBoxLayout *hboxLayout5 = new QHBoxLayout();
    hboxLayout5->setSpacing (spacing);
    nameL    = new QLabel (i18n("Name of game:"), dad);
    hboxLayout5->addWidget(nameL);
    ecName   = new QLineEdit (dad);
    hboxLayout5->addWidget(ecName);
    mainLayout->addLayout(hboxLayout5);

    QHBoxLayout *hboxLayout6 = new QHBoxLayout();
    hboxLayout6->setSpacing (spacing);
    prefixL  = new QLabel (i18n("File name prefix:"), dad);
    hboxLayout6->addWidget(prefixL);
    ecPrefix = new QLineEdit (dad);
    hboxLayout6->addWidget(ecPrefix);
    mainLayout->addLayout(hboxLayout6);

    //In Qt4, QButtonGroup is no longer a widget...
    ecGrp    = new QButtonGroup (dad);
    ecTradB  = new QRadioButton (i18n("Traditional rules"), dad); //last parameter was ecGrp
    ecKGrB   = new QRadioButton (i18n("KGoldrunner rules"), dad); //last parameter was ecGrp
    ecGrp->addButton(ecTradB);
    ecGrp->addButton(ecKGrB);

    //..so we need to add the radio buttons directly to the layout
    mainLayout->addWidget (ecTradB);
    mainLayout->addWidget (ecKGrB);


    nLevL    = new QLabel (i18n( "0 levels" ), dad);
    mainLayout->addWidget (nLevL);

    mleL     = new QLabel (i18n("About this game:"), dad);
    mainLayout->addWidget (mleL);

   // Set up a widget to hold the wrapped text, using \n for paragraph breaks.
    mle	     = new QTextEdit (dad);
    mle->    setTextFormat (Qt::PlainText);
    mainLayout->addWidget (mle);

#ifdef KGR_PORTABLE
    QWidget * buttons = new QWidget(dad);
    QHBoxLayout *hboxLayout7 = new QHBoxLayout(buttons);
    buttons->setLayout(hboxLayout7);
    mainLayout->addWidget (buttons);
    buttons->setSpacing (spacing);
    // Buttons are for Qt-only portability.  NOT COMPILED in KDE environment.
    OK       = new QPushButton (i18n("&OK"), buttons);
    CANCEL   = new QPushButton (i18n("&Cancel"), buttons);

    QPoint p = parent->mapToGlobal (QPoint (0,0));

    // Base the geometry of the dialog box on the playing area.
    int cell = parent->width() / (FIELDWIDTH + 4);
    dad->	move (p.x()+2*cell, p.y()+2*cell);
    dad->	setMinimumSize ((FIELDWIDTH*cell/2), (FIELDHEIGHT-1)*cell);
#endif

    if (action == SL_CR_GAME) {
	     setCaption (i18n("Create Game"));
    }
    else {
	     setCaption (i18n("Edit Game Info"));
    }

    QString OKText = "";
    if (action == SL_UPD_GAME) {		// Edit existing collection.
	ecName->	setText (collections.at(defaultGame)->name);
	ecPrefix->	setText (collections.at(defaultGame)->prefix);
	if (collections.at(defaultGame)->nLevels > 0) {
	    // Collection already has some levels, so cannot change the prefix.
	    ecPrefix->	setEnabled (false);
	}
	QString		s;
#ifndef KGR_PORTABLE
	nLevL->		setText (i18np("1 level", "%n levels",
					collections.at(defaultGame)->nLevels));
#else
	nLevL->		setText (i18n("%1 levels",
				 collections.at(defaultGame)->nLevels));
#endif
	OKText = i18n("Save Changes");
    }
    else {					// Create a collection.
	ecName->        setText ("");
	ecPrefix->      setText ("");
	nLevL->         setText (i18n("0 levels"));
	OKText = i18n("Save New");
    }
#ifdef KGR_PORTABLE
    OK->setText (OKText);
#else
    setButtonGuiItem( KDialog::Ok, KGuiItem(OKText));
#endif

    if ((action == SL_CR_GAME) ||
	(collections.at(defaultGame)->settings == 'T')) {
	ecSetRules ('T');			// Traditional settings.
    }
    else {
	ecSetRules ('K');			// KGoldrunner settings.
    }

    // Configure the edit box.
    mle->		setAlignment (Qt::AlignLeft);

    if ((action == SL_UPD_GAME) &&
	(collections.at(defaultGame)->about.length() > 0)) {
	// Display and edit the game description in its original language.
	mle->		setText (collections.at(defaultGame)->about);
    }
    else {
	mle->		setText ("");
    }

    connect (ecKGrB,  SIGNAL (clicked ()), this, SLOT (ecSetKGr ()));
    connect (ecTradB, SIGNAL (clicked ()), this, SLOT (ecSetTrad ()));

#ifdef KGR_PORTABLE
    OK->		setGeometry (10,  145 + mle->height(), 100,  25);
    // OK->		setAccel (Key_Return);	// No!  We need it in "mle" box.

    CANCEL->		setGeometry (190,  145 + mle->height(), 100,  25);
    CANCEL->		setAccel (Qt::Key_Escape);

    dad->		resize (300, 175 + mle->height());

    connect (OK,     SIGNAL (clicked ()),	this, SLOT (accept()));
    connect (CANCEL, SIGNAL (clicked ()),	this, SLOT (reject()));
#endif
}

KGrECDialog::~KGrECDialog()
{
}

void KGrECDialog::ecSetRules (const char settings)
{
    ecKGrB->	setChecked (false);
    ecTradB->	setChecked (false);
    if (settings == 'K')
	ecKGrB->	setChecked (true);
    else
	ecTradB->	setChecked (true);
}

void KGrECDialog::ecSetKGr ()  {ecSetRules ('K');}	// Radio button slots.
void KGrECDialog::ecSetTrad () {ecSetRules ('T');}

/*******************************************************************************
***************  DIALOG TO SELECT A SAVED GAME TO BE RE-LOADED  ****************
*******************************************************************************/

#ifdef KGR_PORTABLE
KGrLGDialog::KGrLGDialog (QFile * savedGames,
			QList<KGrCollection *> & collections,
			QWidget * parent, const char * name)
		: QDialog (parent, name, true,
			Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title)
#else
KGrLGDialog::KGrLGDialog (QFile * savedGames,
			QList<KGrCollection *> & collections,
			QWidget * parent, const char * name)
		: KDialog (parent)
#endif
{
#ifdef KGR_PORTABLE
    int margin		= 10;
    int spacing		= 10;
    QWidget * dad	= this;
#else
	setCaption(i18n("Select Saved Game"));
	setButtons(KDialog::Ok | KDialog::Cancel);
	setDefaultButton(KDialog::Ok);
    int margin		= marginHint();
    int spacing		= spacingHint();
    QWidget * dad	= new QWidget(this);
	setMainWidget(dad);
#endif

    QVBoxLayout *	mainLayout = new QVBoxLayout (dad);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QLabel *		lgHeader = new QLabel (
			i18n("Game                       Level/Lives/Score   "
			"Day    Date     Time  "), dad);

    lgList   = new QListWidget (dad);
#ifdef KGR_PORTABLE
    QFont		f ("courier", 12);
#else
    QFont		f = KGlobalSettings::fixedFont();	// KDE version.
#endif
			f.setFixedPitch (true);
    lgList->		setFont (f);
			f.setBold (true);
    lgHeader->		setFont (f);

    mainLayout->	addWidget (lgHeader);
    mainLayout->	addWidget (lgList);

#ifdef KGR_PORTABLE
    QWidget *		buttons  = new QWidget(dad);
    QHBoxLayout *hboxLayout8 = new QHBoxLayout(buttons);
    buttons->setLayout(hboxLayout8);
    buttons->		setSpacing (spacing);
    // Buttons are for Qt-only portability.  NOT COMPILED in KDE environment.
    QPushButton *	OK       = new QPushButton (i18n("&OK"), buttons);
    QPushButton *	CANCEL   = new QPushButton (i18n("&Cancel"), buttons);
    mainLayout->	addWidget (buttons);

    dad->		setCaption (i18n("Select Saved Game"));

    // Base the geometry of the list box on the playing area.
    QPoint		p = parent->mapToGlobal (QPoint (0,0));
    int			c = parent->width() / (FIELDWIDTH + 4);
    dad->		move (p.x()+2*c, p.y()+2*c);
    lgList->		setMinimumHeight ((FIELDHEIGHT/2)*c);
    OK->		setMaximumWidth (4*c);
    CANCEL->		setMaximumWidth (4*c);

    OK->		setAccel (Qt::Key_Return);
    CANCEL->		setAccel (Qt::Key_Escape);
#endif

			lgHighlight  = -1;

    QTextStream		gameText (savedGames);
    QString		s = "";
    QString		pr = "";
    int			i;
    int			imax = collections.count();

    // Read the saved games into the list box.
    while (! gameText.endData()) {
	s = gameText.readLine();		// Read in one saved game.
	pr = s.left (s.indexOf (" ", 0, Qt::CaseInsensitive));	// Get the collection prefix.
	for (i = 0; i < imax; i++) {		// Get the collection name.
	    if (collections.at(i)->prefix == pr) {
		s = s.insert (0,
		    collections.at(i)->name.leftJustified (20, ' ', true) + ' ');
		break;
	    }
	}
	lgList-> addItem (s);
    }
    savedGames->close();

    // Mark row 0 (the most recently saved game) as the default selection.
    lgList->	setCurrentItem (0);
    lgList->	setItemSelected  (lgList->currentItem(), true);
		lgHighlight = 0;

    connect (lgList, SIGNAL ( itemClicked ( QListWidgetItem * )), this, SLOT (lgSelect (QListWidgetItem *)));
#ifdef KGR_PORTABLE
    connect (OK,     SIGNAL (clicked ()),        this, SLOT (accept ()));
    connect (CANCEL, SIGNAL (clicked ()),        this, SLOT (reject ()));
#endif
}

void KGrLGDialog::lgSelect (QListWidgetItem * item)
{
    lgHighlight = lgList->row(item);
}

/*******************************************************************************
***********************  CENTRALISED MESSAGE FUNCTIONS  ************************
*******************************************************************************/

void KGrMessage::information (QWidget * parent, const QString &caption, const QString &text)
{
#ifdef KGR_PORTABLE
    // Force Qt to do word-wrapping (but it ignores "\n" line-breaks).
    QMessageBox::information (parent, caption,
				"<qt>" + text + "</qt>");
#else
    // KDE does word-wrapping and will observe "\n" line-breaks.
    KMessageBox::information (parent, text, caption);
#endif
}

int KGrMessage::warning (QWidget * parent, const QString &caption, const QString &text,
			    const QString &label0, const QString &label1, const QString &label2)
{
    int ans = 0;
#ifdef KGR_PORTABLE
    // Display a box with 2 or 3 buttons, depending on if label2 is empty or not.
    // Force Qt to do word-wrapping (but it ignores "\n" line-breaks).
    ans = QMessageBox::warning (parent, caption,
				"<qt>" + text + "</qt>",
				label0, label1, label2,
				0, (label2.isEmpty()) ? 1 : 2);
#else
    // KDE does word-wrapping and will observe "\n" line-breaks.
    if (label2.isEmpty()) {
	// Display a box with 2 buttons.
	ans = KMessageBox::questionYesNo (parent, text, caption,
			    KGuiItem(label0), KGuiItem(label1));
	ans = (ans == KMessageBox::Yes) ? 0 : 1;
    }
    else {
	// Display a box with 3 buttons.
	ans = KMessageBox::questionYesNoCancel (parent, text, caption,
			    KGuiItem(label0), KGuiItem(label1));
	if (ans == KMessageBox::Cancel)
	    ans = 2;
	else
	    ans = (ans == KMessageBox::Yes) ? 0 : 1;
    }
#endif
    return (ans);
}

/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

void KGrMessage::wrapped (QWidget * parent, const QString &title, const QString &contents)
{
#ifndef KGR_PORTABLE
    KMessageBox::information (parent, contents, title);
#else
    QDialog *		mm = new QDialog (parent, "wrappedMessage", true,
			Qt::WStyle_Customize | Qt::WStyle_NormalBorder | Qt::WStyle_Title);

    int margin = 10;
    int spacing = 10;
    QVBoxLayout * mainLayout = new QVBoxLayout (mm);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    // Make text background grey not white (i.e. same as widget background).
    QPalette		pl = mm->palette();
    pl.setColor (QPalette::Base, mm->paletteBackgroundColor());
    mm->		setPalette (pl);

   // Set up a widget to hold the wrapped text, using \n for paragraph breaks.
    QTextEdit *		mle = new QTextEdit (mm);
    mle->		setTextFormat (Qt::PlainText);
    mainLayout->addWidget (mle);

    // Button is for Qt-only portability.  NOT COMPILED in KDE environment.
    QPushButton *	OK = new QPushButton (i18n("&OK"), mm);
    mainLayout->addWidget (OK, Qt::AlignHCenter);

    mm->		setCaption (title);

    // Base the geometry of the text box on the playing area.
    QPoint		p = parent->mapToGlobal (QPoint (0,0));
    int			c = parent->width() / (FIELDWIDTH + 4);
    mm->		move (p.x()+4*c, p.y()+4*c);
    mle->		setMinimumSize ((FIELDWIDTH*c/2), (FIELDHEIGHT/2)*c);
    OK->		setMaximumWidth (3*c);

    mle->		setFrameStyle (QFrame::NoFrame);
    mle->		setAlignment (Qt::AlignLeft);
    mle->		setReadOnly (true);
    mle->		setText (contents);

    OK->		setAccel (Qt::Key_Return);
    connect (OK, SIGNAL (clicked ()), mm, SLOT (accept ()));

    mm->		exec ();

    delete mm;
#endif	// KGR_PORTABLE
}

#include "kgrdialog.moc"
