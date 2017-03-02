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

#ifndef KGRSELECTOR_H
#define KGRSELECTOR_H

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <QList>

#include <KLocalizedString>
#include <QDialog>
#include <KMessageBox>
#include <QtWidgets/QSpinBox>

/**
@author Ian Wadham and Marco Krüger
*/

class KGrGameData;
class KGrThumbNail;
class KGrGameListItem;
class KGrGameIO;
class KIntSpinBox;
class QScrollBar;
class QPushButton;
class QLabel;
class QTextEdit;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class KGrSLDialog : public QDialog
{
Q_OBJECT
public:
    KGrSLDialog (int action, int requestedLevel, int gameIndex,
                 QList<KGrGameData *> & gameList,
                 const QString & pSystemDir, const QString & pUserDir,
                 QWidget * parent = 0);
    ~KGrSLDialog();

    bool selectLevel (int & selectedGame, int & selectedLevel);

signals:
    void editNameAndHint();

private slots:
    void slSetGames (int cIndex);
    void slGame();
    void slShowLevel (int i);
    void slUpdate (const QString & text);
    void slPaintLevel();
    void slotHelp();				// Will replace KDE slotHelp().

private:
    void                setupWidgets();

    int			slAction;
    QList<KGrGameData *> myGameList;	// List of games.
    int			defaultGame;
    int			defaultLevel;
    int			slGameIndex;
    QString             systemDir;
    QString             userDir;
    QWidget *		slParent;

    QLabel *		gameL;
    QTreeWidget *	games;
    QLabel *		gameN;
    QLabel *		gameD;
    QTextEdit *		gameAbout;

    QLabel *		numberL;
    QSpinBox *	    display;
    QScrollBar *	number;
    QPushButton *	levelNH;
    QLabel *		slName;
    KGrThumbNail *	thumbNail;
};

/*******************************************************************************
*************************  ITEM FOR THE LIST OF GAMES  *************************
*******************************************************************************/

class KGrGameListItem : public QTreeWidgetItem
{
public:
    explicit KGrGameListItem (const QStringList & data, const int internalId = -1);
    int id() const;
    void setId (const int internalId);
private:
    int mInternalId;
};

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

class KGrThumbNail : public QFrame
{
public:
    explicit KGrThumbNail (QWidget *parent = 0);
    ~KGrThumbNail();

    void setLevelData (const QString& dir, const QString& prefix,
                       int level, QLabel * sln);

    static QColor backgroundColor;
    static QColor brickColor;
    static QColor ladderColor;
    static QColor poleColor;

protected:
    void paintEvent (QPaintEvent * event);	// Draw a preview of a level.

private:
    KGrGameIO * io;
    QByteArray  levelName;
    QByteArray  levelLayout;
    QLabel *    lName;				// Place to write level-name.
};

#endif
