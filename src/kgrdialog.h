/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef KGRDIALOG_QT_H
#define KGRDIALOG_QT_H

#include <KLocale>
#include <KDialog>
#include <KMessageBox>


#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <QScrollBar>
// IDW #include <QSlider>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QList>
#include <QLabel>
#include <QTextEdit>

/**
@author Ian Wadham and Marco Krüger
*/

class KGrGame;
class KGrCollection;
class KGrThumbNail;
class KGrGameListItem;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class KGrSLDialog : public KDialog
{
Q_OBJECT
public:
    KGrSLDialog (int action, int requestedLevel, int collnIndex,
			QList<KGrCollection *> & gamesList, KGrGame * theGame,
			QWidget * parent = 0);
    ~KGrSLDialog();

    int selectedLevel()	{return (number->value());}
    int selectedGame()	{return (slCollnIndex);}

private slots:
    void slSetCollections (int cIndex);
    void slColln ();
    void slShowLevel (int i);
    void slUpdate (const QString & text);
    void slPaintLevel ();
    void slotHelp ();				// Will replace KDE slotHelp().

private:
    int			slAction;
    QList<KGrCollection *> collections;	// List of games.
    int			defaultLevel;
    int			defaultGame;
    int			slCollnIndex;
    KGrGame *		game;
    KGrCollection *	collection;
    QWidget *		slParent;

    QLabel *		collnL;
    QTreeWidget *	colln;
    QLabel *		collnN;
    QLabel *		collnD;
    QPushButton *	collnA;
    QTextEdit *		collnAbout;

    QLabel *		numberL;
    QSpinBox *		display;
    QScrollBar *	number;
    // IDW QSlider *		number;
    QPushButton *	levelNH;
    QLabel *		slName;
    KGrThumbNail *	thumbNail;
};

/*******************************************************************************
*************** DIALOG BOX TO CREATE/EDIT A LEVEL NAME AND HINT ****************
*******************************************************************************/

class KGrNHDialog : public KDialog
{
Q_OBJECT
public:
    KGrNHDialog (const QString & levelName, const QString & levelHint,
			QWidget * parent = 0);
    ~KGrNHDialog();

    const QString	getName()	{return (nhName->text());}
    const QString	getHint()	{return (mle->toPlainText ());}

private:
    QLineEdit *	nhName;
    QTextEdit *	mle;
};

/*******************************************************************************
***************** DIALOG TO CREATE OR EDIT A GAME (COLLECTION) *****************
*******************************************************************************/

class KGrECDialog : public KDialog
{
Q_OBJECT
public:
    KGrECDialog (int action, int collnIndex,
			QList<KGrCollection *> & gamesList,
			QWidget *parent = 0);
    ~KGrECDialog();

    const QString	getName()	{return (ecName->text());}
    const QString	getPrefix()	{return (ecPrefix->text());}
    const bool	isTrad()	{return (ecTradB->isChecked());}
    const QString	getAboutText()	{return (mle->toPlainText());}

private slots:
    void ecSetRules (const char settings);
    void ecSetKGr();	// Radio button slots.
    void ecSetTrad();

private:
    QList<KGrCollection *> collections;	// List of existing games.
    int			defaultGame;

    QLabel *		nameL;
    QLineEdit *		ecName;
    QLabel *		prefixL;
    QLineEdit *		ecPrefix;
    QButtonGroup *	ecGrp;
    QRadioButton *	ecKGrB;
    QRadioButton *	ecTradB;
    QLabel *		nLevL;

    QLabel *		mleL;
    QTextEdit *		mle;
};

/*******************************************************************************
***************  DIALOG TO SELECT A SAVED GAME TO BE RE-LOADED  ****************
*******************************************************************************/

#include <QFile>
#include <qtextstream.h>

class KGrLGDialog : public KDialog
{
Q_OBJECT
public:
    KGrLGDialog (QFile * savedGames, QList<KGrCollection *> & collections,
			QWidget * parent);
    const QString getCurrentText() {return (lgList->currentItem()->text());}

private slots:
    void lgSelect (QListWidgetItem * item);

private:
    QListWidget * lgList;
    int lgHighlight;
};

/*******************************************************************************
******************  PORTABLE MESSAGE FUNCTIONS (Qt Version)  *******************
*******************************************************************************/

class KGrMessage : public QDialog
{
public:
    static void information (QWidget * parent, const QString &caption,
			    const QString &text);
    static int warning (QWidget * parent, const QString &caption,
			    const QString &text, const QString &label0,
			    const QString &label1, const QString &label2 = "");
};

/*******************************************************************************
*************************  ITEM FOR THE LIST OF GAMES  *************************
*******************************************************************************/

class KGrGameListItem : public QTreeWidgetItem
{
public:
    KGrGameListItem (const QStringList & data, const int internalId = -1);
    int id () const;
    void setId (const int internalId);
private:
    int mInternalId;
};

#endif
