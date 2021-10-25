/*
    SPDX-FileCopyrightText: 2003 Marco Krüger <grisuji@gmx.de>
    SPDX-FileCopyrightText: 2003 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRSELECTOR_H
#define KGRSELECTOR_H

#include <QDialog>
#include <QList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

/**
@author Ian Wadham and Marco Krüger
*/

class KGrGameData;
class KGrThumbNail;
class KGrGameListItem;
class KGrGameIO;
class QSpinBox;
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
                 QWidget * parent = nullptr);
    ~KGrSLDialog() override;

    bool selectLevel (int & selectedGame, int & selectedLevel);

Q_SIGNALS:
    void editNameAndHint();

private Q_SLOTS:
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
    QSpinBox *		display;
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
    explicit KGrThumbNail (QWidget *parent = nullptr);
    ~KGrThumbNail() override;

    void setLevelData (const QString& dir, const QString& prefix,
                       int level, QLabel * sln);

    static QColor backgroundColor;
    static QColor brickColor;
    static QColor ladderColor;
    static QColor poleColor;

protected:
    void paintEvent (QPaintEvent * event) override;	// Draw a preview of a level.

private:
    KGrGameIO * io;
    QByteArray  levelName;
    QByteArray  levelLayout;
    QLabel *    lName;				// Place to write level-name.
};

#endif
