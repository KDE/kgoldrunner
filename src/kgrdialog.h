/****************************************************************************
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

#ifndef KGRDIALOG_QT_H
#define KGRDIALOG_QT_H

#include <QListWidget>
#include <QRadioButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QList>

#include <KLocalizedString>
#include <QDialog>
#include <KMessageBox>

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
                        QWidget * parent = 0);
    ~KGrNHDialog();

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
                        QWidget *parent = 0);
    ~KGrECDialog();

    const QString	getName()	{return (ecName->text());}
    const QString	getPrefix()	{return (ecPrefix->text());}
    bool  isTrad()	{return (ecTradB->isChecked());}
    const QString	getAboutText()	{return (mle->toPlainText());}

private slots:
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

private slots:
    void lgSelect (QListWidgetItem * item);

private:
    QListWidget * lgList;
    int lgHighlight;
};

#endif
