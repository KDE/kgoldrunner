/****************************************************************************
 *    Copyright 2003  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003  Ian Wadham <iandw.au@gmail.com>                       *
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

#include "kgrdialog.h"

#include "kgrglobals.h"

#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>

#include <KGlobalSettings>

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
        mle->setPlainText (QString::fromUtf8
                           (myGameList.at (defaultGame)->about.constData()));
    }
    else {
        mle->setPlainText ("");
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
    while (! gameText.atEnd()) {
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

    connect (lgList, SIGNAL (itemClicked(QListWidgetItem*)),
                this, SLOT (lgSelect(QListWidgetItem*)));
}

void KGrLGDialog::lgSelect (QListWidgetItem * item)
{
    lgHighlight = lgList->row (item);
}

/*******************************************************************************
***********************  CENTRALIZED MESSAGE FUNCTIONS  ************************
*******************************************************************************/

void KGrMessage::information (QWidget * parent,
                        const QString & caption, const QString & text,
                        const QString & dontShowAgain)
{
    // KDE does word-wrapping and will observe "\n" line-breaks.
    KMessageBox::information (parent, text, caption, dontShowAgain);
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
                            KGuiItem (label0), KGuiItem (label1),
                            KGuiItem (label2));
        if (ans == KMessageBox::Cancel)
            ans = 2;
        else
            ans = (ans == KMessageBox::Yes) ? 0 : 1;
    }
    return (ans);
}


