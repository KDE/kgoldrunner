/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
***************************************************************************/
#ifndef KGRSELECTOR_H
#define KGRSELECTOR_H

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

class KGrGameData;
class KGrThumbNail;
class KGrGameListItem;
class KGrGameIO;

/******************************************************************************/
/*******************    DIALOG TO SELECT A GAME AND LEVEL   *******************/
/******************************************************************************/

class KGrSLDialog : public KDialog
{
Q_OBJECT
public:
    KGrSLDialog (int action, int requestedLevel, int gameIndex,
                 QList<KGrGameData *> & gameList,
                 const QString & pSystemDir, const QString & pUserDir,
                 QWidget * parent = 0);
    ~KGrSLDialog();

    bool selectLevel (int & selectedGame, int & selectedLevel);

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
    QPushButton *	gameA;
    QTextEdit *		gameAbout;

    QLabel *		numberL;
    QSpinBox *		display;
    QScrollBar *	number;
    // IDW QSlider *		number;
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
    KGrGameListItem (const QStringList & data, const int internalId = -1);
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
