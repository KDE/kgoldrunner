/***************************************************************************
    Copyright 2003 Marco Krüger
    Copyright 2003 Ian Wadham <ianw@netspace.net.au>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef KGRDIALOG_QT_H
#define KGRDIALOG_QT_H

// If portable version, use QDialog and QMessageBox.
// If KDE version, use KDialogBase and KMessageBox.

#ifdef KGR_PORTABLE
#include <qdialog.h>
#define KGR_DIALOG QDialog
#include <qmessagebox.h>

#else
#include <klocale.h>
#include <kdialog.h>
#define KGR_DIALOG KDialog
#include <kmessagebox.h>
#endif

#include <QLayout>

#include <QListWidget>
#include <QScrollBar>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QList>
#include <QLabel>
#include <QTextEdit>

/**
@author Ian Wadham and Marco Krüger
*/

class KGrCanvas;
class KGrGame;
class KGrCollection;
class KGrThumbNail;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class KGrSLDialog : public KGR_DIALOG	// KGR_PORTABLE sets QDialog/KDialogBase
{
Q_OBJECT
public:
    KGrSLDialog (int action, int requestedLevel, int collnIndex,
			QList<KGrCollection *> & gamesList, KGrGame * theGame,
			QWidget * parent = 0, const char *name = 0);
    ~KGrSLDialog();

    int selectedLevel()	{return (number->value());}
    int selectedGame()	{return (slCollnIndex);}

private slots:
    void slSetCollections (int cIndex);
    //void slColln (int i);
    void slColln (QListWidgetItem *);
    void slAboutColln ();
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
    QListWidget *	colln;
    QLabel *		collnN;
    QLabel *		collnD;
    QPushButton *	collnA;

    QLabel *		numberL;
    QLineEdit *		display;
    QScrollBar *	number;
    QPushButton *	levelNH;
    QLabel *		slName;
    KGrThumbNail *	thumbNail;

#ifdef KGR_PORTABLE
    QPushButton *	OK;
    QPushButton *	HELP;
    QPushButton *	CANCEL;
#endif
};

/*******************************************************************************
*************** DIALOG BOX TO CREATE/EDIT A LEVEL NAME AND HINT ****************
*******************************************************************************/

class KGrNHDialog : public KGR_DIALOG	// KGR_PORTABLE sets QDialog/KDialogBase
{
Q_OBJECT
public:
    KGrNHDialog (const QString & levelName, const QString & levelHint,
			QWidget * parent = 0, const char * name = 0);
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

class KGrECDialog : public KGR_DIALOG	// KGR_PORTABLE sets QDialog/KDialogBase
{
Q_OBJECT
public:
    KGrECDialog (int action, int collnIndex,
			QList<KGrCollection *> & gamesList,
			QWidget *parent = 0, const char *name = 0);
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

#ifdef KGR_PORTABLE
    QPushButton *	OK;
    QPushButton *	CANCEL;
#endif
};

/*******************************************************************************
***************  DIALOG TO SELECT A SAVED GAME TO BE RE-LOADED  ****************
*******************************************************************************/

#include <QFile>
#include <qtextstream.h>

class KGrLGDialog : public KGR_DIALOG	// KGR_PORTABLE sets QDialog/KDialogBase
{
Q_OBJECT
public:
    KGrLGDialog (QFile * savedGames, QList<KGrCollection *> & collections,
			QWidget * parent, const char * name);
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
    static void information (QWidget * parent, const QString &caption, const QString &text);
    static int warning (QWidget * parent, QString caption, QString text,
			QString label0, QString label1, QString label2 = "");
    static void wrapped (QWidget * parent, QString caption, QString text);
};

#endif
