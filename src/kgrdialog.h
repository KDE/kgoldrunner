/*
    SPDX-FileCopyrightText: 2003 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRDIALOG_QT_H
#define KGRDIALOG_QT_H

#include <QDialog>
#include <QList>
#include <QLineEdit>
#include <QListWidget>
#include <QRadioButton>
#include <QTextEdit>


class QButtonGroup;
class QLabel;

/**
@author Ian Wadham and Marco Krüger
*/

class KGrGameData;

/*******************************************************************************
*************** DIALOG BOX TO CREATE/EDIT A LEVEL NAME AND HINT ****************
*******************************************************************************/

class KGrNHDialog : public QDialog
{
Q_OBJECT
public:
    KGrNHDialog (const QString & levelName, const QString & levelHint,
                        QWidget * parent = nullptr);
    ~KGrNHDialog() override;

    const QString	getName()	{return (nhName->text());}
    const QString	getHint()	{return (mle->toPlainText());}

private:
    QLineEdit *	nhName;
    QTextEdit *	mle;
};

/*******************************************************************************
***************** DIALOG TO CREATE OR EDIT A GAME (COLLECTION) *****************
*******************************************************************************/

class KGrECDialog : public QDialog
{
Q_OBJECT
public:
    KGrECDialog (int action, int collnIndex,
                        QList<KGrGameData *> & gameList,
                        QWidget *parent = nullptr);
    ~KGrECDialog() override;

    const QString	getName()	{return (ecName->text());}
    const QString	getPrefix()	{return (ecPrefix->text());}
    bool  isTrad()	{return (ecTradB->isChecked());}
    const QString	getAboutText()	{return (mle->toPlainText());}

private Q_SLOTS:
    void ecSetRules (const char rules);
    void ecSetKGr();	// Radio button slots.
    void ecSetTrad();

private:
    QList<KGrGameData *> myGameList;	// List of existing games.
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

class QFile;

class KGrLGDialog : public QDialog
{
Q_OBJECT
public:
    KGrLGDialog (QFile * savedGames, QList<KGrGameData *> & gameList,
                        QWidget * parent);
    const QString getCurrentText() {return (lgList->currentItem()->text());}

private Q_SLOTS:
    void lgSelect (QListWidgetItem * item);

private:
    QListWidget * lgList;
    int lgHighlight;
};

#endif
