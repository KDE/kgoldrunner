/***************************************************************************
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "kgrgame.h"

#ifdef KGR_PORTABLE
// If compiling for portability, redefine KDE's i18n.
#define i18n tr
#endif

#include "kgrconsts.h"
#include "kgrobject.h"
#include "kgrfigure.h"
#include "kgrcanvas.h"
#include "kgrdialog.h"

// Obsolete - #include <iostream.h>
#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <kpushbutton.h>
#include <KStandardGuiItem>

#define USE_KSCOREDIALOG 1

#ifdef USE_KSCOREDIALOG
#include <KScoreDialog>
#include <QDate>
#else

#ifndef KGR_PORTABLE
#include <kglobalsettings.h>
#include <QByteArray>
#include <QTextStream>
#include <QLabel>
#include <QVBoxLayout>
#include <QDate>
#include <QSpacerItem>
#endif

#endif

/******************************************************************************/
/***********************    KGOLDRUNNER GAME CLASS    *************************/
/******************************************************************************/

KGrGame::KGrGame (KGrCanvas * theView, 
		const QString &theSystemDir, const QString &theUserDir) : 
	view(theView), systemDataDir(theSystemDir), userDataDir(theUserDir), level(0)
{
    //view = theView;
    //systemDataDir = theSystemDir;
    //userDataDir = theUserDir;

    // Set the game-editor OFF, but available.
    editMode = false;
    paintEditObj = false;
    editObj  = BRICK;
    shouldSave = false;

    hero = new KGrHero (view, 0, 0);	// The hero is born ... Yay !!!
    hero->setPlayfield (&playfield);

    setBlankLevel (true);		// Fill the playfield with blank walls.

    enemy = NULL;
    newLevel = true;			// Next level will be a new one.
    loading  = true;			// Stop input until it is loaded.

    modalFreeze = false;
    messageFreeze = false;

    connect (hero, SIGNAL (gotNugget(int)),   SLOT (incScore(int)));
    connect (hero, SIGNAL (caughtHero()),     SLOT (herosDead()));
    connect (hero, SIGNAL (haveAllNuggets()), SLOT (showHiddenLadders()));
    connect (hero, SIGNAL (leaveLevel()),     SLOT (goUpOneLevel()));

    dyingTimer = new QTimer (this);
    connect (dyingTimer, SIGNAL (timeout()),  SLOT (finalBreath()));

    // Get the mouse position every 40 msec.  It is used to steer the hero.
    mouseSampler = new QTimer (this);
    connect (mouseSampler, SIGNAL(timeout()), SLOT (readMousePos ()));
    mouseSampler->start (40);

    srand (time(0)); 			// Initialise random number generator.
}

KGrGame::~KGrGame()
{
   //release collections
   while (!collections.isEmpty())
	delete collections.takeFirst();
}

/******************************************************************************/
/*************************  GAME SELECTION PROCEDURES  ************************/
/******************************************************************************/

void KGrGame::startLevelOne()
{
    startLevel (SL_START, 1);
}

void KGrGame::startAnyLevel()
{
    startLevel (SL_ANY, level);
}

void KGrGame::startNextLevel()
{
    startLevel (SL_ANY, level + 1);
}

void KGrGame::startLevel (int startingAt, int requestedLevel)
{
    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }
    // Use dialog box to select game and level: startingAt = ID_FIRST or ID_ANY.
    int selectedLevel = selectLevel (startingAt, requestedLevel);
    if (selectedLevel > 0) {	// If OK, start the selected game and level.
 	newGame (selectedLevel, selectedGame);
    } else {
      level = 0;
    }
}

/******************************************************************************/
/************************  MAIN GAME EVENT PROCEDURES  ************************/
/******************************************************************************/

void KGrGame::incScore (int n)
{
  score = score + n;		// SCORING: trap enemy 75, kill enemy 75,
  emit showScore (score);	// collect gold 250, complete the level 1500.
}

void KGrGame::herosDead()
{
    if ((level < 1) || (lives <= 0))
	return;			// Game over: we are in the "ENDE" screen.

    // Lose a life.
    if (--lives > 0) {
	// Still some life left, so PAUSE and then re-start the level.
	emit showLives (lives);
	KGrObject::frozen = true;	// Freeze the animation and let
	dyingTimer->setSingleShot(true);
	dyingTimer->start (1500);	// the player see what happened.
	view->fadeOut();
    }
    else {
	// Game over: display the "ENDE" screen.
	emit showLives (lives);
	freeze();
	QString gameOver = "<NOBR><B>" + i18n("GAME OVER !!!") + "</B></NOBR>";
	KGrMessage::information (view, collection->name, gameOver);
	checkHighScore();	// Check if there is a high score for this game.

	enemyCount = 0;
	//todo enemies.clear();	// Stop the enemies catching the hero again ...
	while (!enemies.isEmpty())
	        delete enemies.takeFirst();

	view->deleteEnemySprites();
	unfreeze();		//    ... NOW we can unfreeze.
	newLevel = true;
	level = 0;
	loadLevel (level);	// Display the "ENDE" screen.
	newLevel = false;
    }
}

void KGrGame::finalBreath()
{
    // Fix bug 95202:	Avoid re-starting if the player selected
    //			edit mode before the 1.5 seconds were up.
    if (! editMode) {
	enemyCount = 0;		// Hero is dead: re-start the level.
	loadLevel (level);
    }
    KGrObject::frozen = false;	// Unfreeze the game, but don't move yet.
}

void KGrGame::showHiddenLadders()
{
  int i,j;
  for (i=1;i<21;i++)
    for (j=1;j<29;j++)
      if (playfield[j][i]->whatIam()==HLADDER)
	((KGrHladder *)playfield[j][i])->showLadder();
  //view->updateCanvas();
  initSearchMatrix();
}
// 
void KGrGame::goUpOneLevel()
{
    lives++;			// Level completed: gain another life.
    emit showLives (lives);
    incScore (1500);

    if (level >= collection->nLevels) {
	freeze();
	KGrMessage::information (view, collection->name,
	    i18n("<b>CONGRATULATIONS !!!!</b>"
	    "<p>You have conquered the last level in the "
            "<b>\"%1\"</b> game !!</p>", collection->name));
	checkHighScore();	// Check if there is a high score for this game.

	unfreeze();
	level = 0;		// Game completed: display the "ENDE" screen.
    }
    else {
	level++;		// Go up one level.
	emit showLevel (level);
    }

    enemyCount = 0;
    //enemies.clear();
    while (!enemies.isEmpty())
        delete enemies.takeFirst();

    view->deleteEnemySprites();
    newLevel = true;
    loadLevel (level);
    newLevel = false;
}

void KGrGame::loseNugget()
{
    hero->loseNugget();		// Enemy trapped/dead and holding a nugget.
}

KGrHero * KGrGame::getHero()
{
    return (hero);		// Return a pointer to the hero.
}

int KGrGame::getLevel()		// Return the current game-level.
{
    return (level);
}

bool KGrGame::inMouseMode()
{
    return (mouseMode);		// Return true if game is under mouse control.
}

bool KGrGame::inEditMode()
{
    return (editMode);		// Return true if the game-editor is active.
}

bool KGrGame::isLoading()
{
    return (loading);		// Return true if a level is being loaded.
}

void KGrGame::setMouseMode (bool on_off)
{
    mouseMode = on_off;		// Set Mouse OR keyboard control.
}

void KGrGame::freeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
	emit gameFreeze (true);	// Do visual feedback in the GUI.
    }
    KGrObject::frozen = true;	// Halt the game, by blocking all timer events.
}

void KGrGame::unfreeze()
{
    if ((! modalFreeze) && (! messageFreeze)) {
	emit gameFreeze (false);// Do visual feedback in the GUI.
    }
    KGrObject::frozen = false;	// Restart the game.  Because frozen == false,
    restart();			// the game goes on running after the next step.
}

void KGrGame::setMessageFreeze (bool on_off)
{
    if (on_off) {		// Freeze the game action during a message.
	messageFreeze = false;
	if (! KGrObject::frozen) {
	    messageFreeze = true;
	    freeze();
	}
    }
    else {			// Unfreeze the game action after a message.
	if (messageFreeze) {
	    unfreeze();
	    messageFreeze = false;
	}
    }
}

void KGrGame::setBlankLevel(bool playable)
{
    for (int j=0;j<20;j++)
      for (int i=0;i<28;i++) {
	if (playable) {
	    //playfield[i+1][j+1] = new KGrFree (freebg, nuggetbg, false, view);
	    playfield[i+1][j+1] = new KGrFree (FREE,i+1,j+1,view);
	}
	else {
	    //playfield[i+1][j+1] = new KGrEditable (freebg, view);
	    playfield[i+1][j+1] = new KGrEditable (FREE);
	    view->paintCell (i+1, j+1, FREE);
	}
	editObjArray[i+1][j+1] = FREE;
      }
    for (int j=0;j<30;j++) {
      //playfield[j][0]=new KGrBeton(QPixmap ());
      playfield[j][0]=new KGrObject (BETON);
      editObjArray[j][0] = BETON;
      //playfield[j][21]=new KGrBeton(QPixmap ());
      playfield[j][21]=new KGrObject (BETON);
      editObjArray[j][21] = BETON;
    }
    for (int i=0;i<22;i++) {
      //playfield[0][i]=new KGrBeton(QPixmap ());
      playfield[0][i]=new KGrObject (BETON);
      editObjArray[0][i] = BETON;
      //playfield[29][i]=new KGrBeton(QPixmap ());
      playfield[29][i]=new KGrObject (BETON);
      editObjArray[29][i] = BETON;
    }
    //for (int j=0;j<22;j++)
      //for (int i=0;i<30;i++) {
	//playfield[i][j]->move(16+i*16,16+j*16);
    //}
}

void KGrGame::newGame (const int lev, const int gameIndex)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;		// "loadLevel (level)" will reset it.

    if (editMode) {
	emit setEditMenu (false);	// Disable edit menu items and toolbar.

	editMode = false;
	paintEditObj = false;
	editObj = BRICK;

	view->setHeroVisible (true);
    }

    newLevel = true;
    level = lev;
    collnIndex = gameIndex;
    collection = collections.at (collnIndex);
    owner = collection->owner;

    lives = 5;				// Start with 5 lives.
    score = 0;
    startScore = 0;

    emit showLives (lives);
    emit showScore (score);
    emit showLevel (level);

    enemyCount = 0;

    //enemies.clear();
    while (!enemies.isEmpty())
        delete enemies.takeFirst();

    view->deleteEnemySprites();

    newLevel = true;;
    loadLevel (level);
    newLevel = false;
}

void KGrGame::startTutorial()
{
    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }

    int i, index;
    int imax = collections.count();
    bool found = false;

    index = 0;
    for (i = 0; i < imax; i++) {
	index = i;			// Index within owner.
	if (collections.at(i)->prefix == "tute") {
	    found = true;
	    break;
	}
    }
    if (found) {
	// Start the tutorial.
	collection = collections.at (index);
	owner = collection->owner;
	emit markRuleType (collection->settings);
	collnIndex = index;
	level = 1;
	newGame (level, collnIndex);
    }
    else {
	KGrMessage::information (view, i18n("Start Tutorial"),
	    i18n("Cannot find the tutorial game (file-prefix '%1') in "
	    "the '%2' files.",
	    QString("tute"), QString("games.dat")));
    }
}

void KGrGame::showHint()
{
    // Put out a hint for this level.
    QString caption = i18n("Hint");

    if (levelHint.length() > 0)
	myMessage (view, caption, levelHint);
    else
	myMessage (view, caption,
			i18n("Sorry, there is no hint for this level."));
}

int KGrGame::loadLevel (int levelNo)
{
    view->fadeIn();
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

    // Read the level data.
    LevelData d;
    if (! readLevelData (levelNo, d)) {
	loading = false;
	return 0;
    }

    view->setLevel(levelNo);
    nuggets = 0;
    enemyCount=0;
    startScore = score;				// What we will save, if asked.

    int i, j;
    // Load the level-layout, hero and enemies.
    for (j = 1; j <= FIELDHEIGHT; j++) {
	for (i = 1; i <= FIELDWIDTH; i++) {
	    changeObject (d.layout.at ((j-1)*FIELDWIDTH + (i-1)), i , j);
	}
    }

    // If there is a name, translate the UTF-8 coded QByteArray right now.
    levelName = (d.name.size() > 0) ? i18n((const char *) d.name) : "";

    // Indicate on the menus whether there is a hint for this level.
    int len = d.hint.length();
    emit hintAvailable (len > 0);

    // If there is a hint, translate it right now.
    levelHint = (len > 0) ? i18n((const char *) d.hint) : "";

    // Disconnect edit-mode slots from signals from "view".
    disconnect (view, SIGNAL (mouseClick(int)), 0, 0);
    disconnect (view, SIGNAL (mouseLetGo(int)), 0, 0);

    if (newLevel) {
	hero->setEnemyList (&enemies);
	QListIterator<KGrEnemy *> i (enemies);
	while (i.hasNext()) {
	    KGrEnemy * enemy = i.next();
	    enemy->setEnemyList (&enemies);
	}
    }

    hero->setNuggets (nuggets);
    setTimings();

    // Make a new sequence of all possible x co-ordinates for enemy rebirth.
    if (KGrFigure::reappearAtTop && (enemies.count() > 0)) {
	KGrEnemy::makeReappearanceSequence();
    }

    // Set direction-flags to use during enemy searches.
    initSearchMatrix();

    // Re-draw the playfield frame, level title and figures.
    view->setTitle (getTitle());

    // Check if this is a tutorial collection and not on the "ENDE" screen.
    if ((collection->prefix.left(4) == "tute") && (levelNo != 0)) {
	// At the start of a tutorial, put out an introduction.
	if (levelNo == 1) {
	    myMessage (view, collection->name,
			i18n(collection->about.toUtf8().constData()));
	}
	// Put out an explanation of this level.
	myMessage (view, getTitle(), levelHint);
    }

    // If in mouse mode, not keyboard mode, put the mouse pointer on the hero.
    if (mouseMode) {
	view->setMousePos (startI, startJ);
    }

    // Connect play-mode slot to signal from "view".
    connect (view, SIGNAL(mouseClick(int)), SLOT(doDig(int)));

    // Re-enable player input.
    loading = false;

    return 1;
}

bool KGrGame::readLevelData (int levelNo, LevelData & d)
{
    KGrGameIO io;
    // If system game or ENDE screen, choose system dir, else choose user dir.
    const QString dir = ((owner == SYSTEM) || (levelNo == 0)) ?
					systemDataDir : userDataDir;
    IOStatus stat = io.fetchLevelData (dir, collection->prefix, levelNo, d);

    switch (stat) {
    case NotFound:
	KGrMessage::information (view, i18n("Read Level Data"),
	    i18n("Cannot find file '%1'.", d.filePath));
	break;
    case NoRead:
    case NoWrite:
	KGrMessage::information (view, i18n("Read Level Data"),
	    i18n("Cannot open file '%1' for read-only.", d.filePath));
	break;
    case UnexpectedEOF:
	KGrMessage::information (view, i18n("Read Level Data"),
	    i18n("Reached end of file '%1' without finding level data.",
	    d.filePath));
	break;
    case OK:
	break;
    }

    return (stat == OK);
}

void KGrGame::changeObject (unsigned char kind, int i, int j)
{
  delete playfield[i][j];
  switch(kind) {
  case FREE:	createObject(new KGrFree (FREE,i,j,view),FREE,i,j);break;
  case LADDER:	createObject(new KGrObject (LADDER),LADDER,i,j);break;
  case HLADDER:	createObject(new KGrHladder (HLADDER,i,j,view),FREE,i,j);break;
  case BRICK:	createObject(new KGrBrick (BRICK,i,j,view),BRICK,i,j);break;
  case BETON:	createObject(new KGrObject (BETON),BETON,i,j);break;
  case FBRICK:	createObject(new KGrObject (FBRICK),BRICK,i,j);break;
  case POLE:	createObject(new KGrObject (POLE),POLE,i,j);break;
  case NUGGET:	createObject(new KGrFree (NUGGET,i,j,view),NUGGET,i,j);
				nuggets++;break;
  case HERO:	createObject(new KGrFree (FREE,i,j,view),FREE,i,j);
    hero->init(i,j);
    startI = i; startJ = j;
    hero->started = false;
    hero->showFigure();
    break;
  case ENEMY:	createObject(new KGrFree (FREE,i,j,view),FREE,i,j);
    if (newLevel){
      // Starting a level for the first time.
      enemy = new KGrEnemy (view, i, j);
      enemy->setPlayfield(&playfield);
      enemy->enemyId = enemyCount++;
      enemies.append(enemy);
      connect(enemy, SIGNAL(lostNugget()), SLOT(loseNugget()));
      connect(enemy, SIGNAL(trapped(int)), SLOT(incScore(int)));
      connect(enemy, SIGNAL(killed(int)),  SLOT(incScore(int)));
    } else {
      // Starting a level again after losing.
      enemy=enemies.at(enemyCount);
      enemy->enemyId=enemyCount++;
      enemy->setNuggets(0);
      enemy->init(i,j);	// Re-initialise the enemy's state information.
    }
    enemy->showFigure();
    break;
  default :  createObject(new KGrBrick(BRICK,i,j,view),BRICK,i,j);break;
  }
}

void KGrGame::createObject (KGrObject *o, char picType, int x, int y)
{
    playfield[x][y] = o;
    view->paintCell (x, y, picType);		// Pic maybe not same as object.
}

void KGrGame::setTimings ()
{
    Timing *	timing;
    int		c = -1;

    if (KGrFigure::variableTiming) {
	c = enemies.count();			// Timing based on enemy count.
	c = (c > 5) ? 5 : c;
	timing = &(KGrFigure::varTiming[c]);
    }
    else {
	timing = &(KGrFigure::fixedTiming);	// Fixed timing.
    }

    KGrHero::WALKDELAY		= timing->hwalk;
    KGrHero::FALLDELAY		= timing->hfall;
    KGrEnemy::WALKDELAY		= timing->ewalk;
    KGrEnemy::FALLDELAY		= timing->efall;
    KGrEnemy::CAPTIVEDELAY	= timing->ecaptive;
    KGrBrick::HOLETIME		= timing->hole;
}

void KGrGame::initSearchMatrix()
{
  // Called at start of level and also when hidden ladders appear.
  int i,j;

  for (i=1;i<21;i++){
    for (j=1;j<29;j++)
      {
	// If on ladder, can walk L, R, U or D.
	if (playfield[j][i]->whatIam()==LADDER)
	  playfield[j][i]->searchValue = CANWALKLEFT + CANWALKRIGHT +
					CANWALKUP + CANWALKDOWN;
	else
	  // If on solid ground, can walk L or R.
	  if ((playfield[j][i+1]->whatIam()==BRICK)||
	      (playfield[j][i+1]->whatIam()==HOLE)||
	      (playfield[j][i+1]->whatIam()==USEDHOLE)||
	      (playfield[j][i+1]->whatIam()==BETON))
	    playfield[j][i]->searchValue=CANWALKLEFT+CANWALKRIGHT;
	  else
	    // If on pole or top of ladder, can walk L, R or D.
	    if ((playfield[j][i]->whatIam()==POLE)||
		(playfield[j][i+1]->whatIam()==LADDER))
	      playfield[j][i]->searchValue=CANWALKLEFT+CANWALKRIGHT+CANWALKDOWN;
	    else
	      // Otherwise, gravity takes over ...
	      playfield[j][i]->searchValue=CANWALKDOWN;

	// Clear corresponding bits if there are solids to L, R, U or D.
	if(playfield[j][i-1]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKUP;
	if(playfield[j-1][i]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKLEFT;
	if(playfield[j+1][i]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKRIGHT;
	if(playfield[j][i+1]->blocker)
	  playfield[j][i]->searchValue &= ~CANWALKDOWN;
      }
  }
}

void KGrGame::startPlaying () {
    if (! hero->started) {
	// Start the enemies and the hero.
	for (--enemyCount; enemyCount>=0; --enemyCount) {
	    enemy=enemies.at(enemyCount);
	    enemy->startSearching();
	}
	hero->start();
    }
}

QString KGrGame::getDirectory (Owner o)
{
    return ((o == SYSTEM) ? systemDataDir : userDataDir);
}

QString KGrGame::getFilePath (Owner o, KGrCollection * colln, int lev)
{
    QString filePath;

    if (lev == 0) {
	// End of game: show the "ENDE" screen.
	o = SYSTEM;
	filePath = "level000.grl";
    }
    else {
	filePath.setNum (lev);		// Convert INT -> QString.
	filePath = filePath.rightJustified (3,'0'); // Add 0-2 zeros at left.
	filePath.append (".grl");	// Add KGoldrunner level-suffix.
	filePath.prepend (colln->prefix);	// Add collection file-prefix.
    }

    filePath.prepend (((o == SYSTEM)? systemDataDir : userDataDir) + "levels/");

    return (filePath);
}

QString KGrGame::getTitle()
{
    QString levelTitle;
    if (level == 0) {
	// Generate a special title: end of game or creating a new level.
	if (! editMode)
	    levelTitle = "E N D --- F I N --- E N D E";
	else
	    levelTitle = i18n("New Level");
    }
    else {
	// Generate title string "Collection-name - NNN - Level-name".
	levelTitle.setNum (level);
	levelTitle = levelTitle.rightJustified (3,'0');
	levelTitle = collection->name + " - " + levelTitle;
	if (levelName.length() > 0) {
	    levelTitle = levelTitle + " - " + levelName;
	}
    }
    return (levelTitle);
}

void KGrGame::readMousePos()
{
    QPoint p;
    int i, j;

    // If loading a level for play or editing, ignore mouse-position input.
    if (loading) return;

    // If game control is currently by keyboard, ignore the mouse.
    if ((! mouseMode) && (! editMode)) return;

    p = view->getMousePos ();
    i = p.x(); j = p.y();

    if (editMode) {
	// Editing - check if we are in paint mode and have moved the mouse.
	if (paintEditObj && ((i != oldI) || (j != oldJ))) {
	    insertEditObj (i, j, editObj);
	    //view->updateCanvas();
	    oldI = i;
	    oldJ = j;
	}
	if (paintAltObj && ((i != oldI) || (j != oldJ))) {
	    insertEditObj (i, j, FREE);
	    //view->updateCanvas();
	    oldI = i;
	    oldJ = j;
	}
	// Highlight the cursor position
	
    }
    else {
	// Playing - if  the level has started, control the hero.
	if (KGrObject::frozen) return;	// If game is stopped, do nothing.

	hero->setDirection (i, j);

	// Start playing when the mouse moves off the hero.
	if ((! hero->started) && ((i != startI) || (j != startJ))) {
	    startPlaying();
	}
    }
}

void KGrGame::doDig (int button) {

    // If game control is currently by keyboard, ignore the mouse.
    if (editMode) return;
    if (! mouseMode) return;

    // If loading a level for play or editing, ignore mouse-button input.
    if ((! loading) && (! KGrObject::frozen)) {
	if (! hero->started) {
	    startPlaying();	// If first player-input, start playing.
	}
	switch (button) {
	case Qt::LeftButton:	hero->digLeft  (); break;
	case Qt::RightButton:	hero->digRight (); break;
	default:		break;
	}
    }
}

void KGrGame::heroAction (KBAction movement)
{
    switch (movement) {
    case KB_UP:		hero->setKey (UP); break;
    case KB_DOWN:	hero->setKey (DOWN); break;
    case KB_LEFT:	hero->setKey (LEFT); break;
    case KB_RIGHT:	hero->setKey (RIGHT); break;
    case KB_STOP:	hero->setKey (STAND); break;
    case KB_DIGLEFT:	hero->setKey (STAND); hero->digLeft  (); break;
    case KB_DIGRIGHT:	hero->setKey (STAND); hero->digRight (); break;
    }
}

/******************************************************************************/
/**************************  SAVE AND RE-LOAD GAMES  **************************/
/******************************************************************************/

void KGrGame::saveGame()		// Save game ID, score and level.
{
    if (editMode) {myMessage (view, i18n("Save Game"),
	i18n("Sorry, you cannot save your game play while you are editing. "
	"Please try menu item \"%1\".",
        i18n("&Save Edits...")));
	return;
    }
    if (hero->started) {myMessage (view, i18n("Save Game"),
	i18n("Please note: for reasons of simplicity, your saved game "
	"position and score will be as they were at the start of this "
	"level, not as they are now."));
    }

    QDate today = QDate::currentDate();
    QTime now =   QTime::currentTime();
    QString saved;
    QString day;
    day = today.shortDayName(today.dayOfWeek());
    saved = saved.sprintf
		("%-6s %03d %03ld %7ld    %s %04d-%02d-%02d %02d:%02d\n",
		collection->prefix.myStr(), level, lives, startScore,
		day.myStr(),
		today.year(), today.month(), today.day(),
		now.hour(), now.minute());

    QFile file1 (userDataDir + "savegame.dat");
    QFile file2 (userDataDir + "savegame.tmp");

    if (! file2.open (QIODevice::WriteOnly)) {
	KGrMessage::information (view, i18n("Save Game"),
		i18n("Cannot open file '%1' for output.",
		 userDataDir + "savegame.tmp"));
	return;
    }
    QTextStream text2 (&file2);
    text2 << saved;

    if (file1.exists()) {
	if (! file1.open (QIODevice::ReadOnly)) {
	    KGrMessage::information (view, i18n("Save Game"),
		i18n("Cannot open file '%1' for read-only.",
		 userDataDir + "savegame.dat"));
	    return;
	}

	QTextStream text1 (&file1);
	int n = 30;			// Limit the file to the last 30 saves.
	while ((! text1.endData()) && (--n > 0)) {
	    saved = text1.readLine() + '\n';
	    text2 << saved;
	}
	file1.close();
    }

    file2.close();

    if (safeRename (userDataDir+"savegame.tmp", userDataDir+"savegame.dat")) {
	KGrMessage::information (view, i18n("Save Game"),
				i18n("Your game has been saved."));
    }
    else {
	KGrMessage::information (view, i18n("Save Game"),
				i18n("Error: Failed to save your game."));
    }
}

bool KGrGame::safeRename (const QString & oldName, const QString & newName)
{
    QFile newFile (newName);
    if (newFile.exists()) {
	// On some file systems we cannot rename if a file with the new name
	// already exists.  We must delete the existing file, otherwise the
	// upcoming QFile::rename will fail, according to Qt4 docs.  This
	// seems to be true with reiserfs at least.
	if (! newFile.remove()) {
	    KGrMessage::information (view, i18n("Rename File"),
		i18n("Cannot delete previous version of file '%1'.", newName));
	    return false;
	}
    }
    QFile oldFile (oldName);
    if (! oldFile.rename (newName)) {
	KGrMessage::information (view, i18n("Rename File"),
	    i18n("Cannot rename file '%1' to '%2'.", oldName, newName));
	return false;
    }
    return true;
}

void KGrGame::loadGame()		// Re-load game, score and level.
{
    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }

    QFile savedGames (userDataDir + "savegame.dat");
    if (! savedGames.exists()) {
	// Use myMessage() because it stops the game while the message appears.
	myMessage (view, i18n("Load Game"),
			i18n("Sorry, there are no saved games."));
	return;
    }

    if (! savedGames.open (QIODevice::ReadOnly)) {
	KGrMessage::information (view, i18n("Load Game"),
	    i18n("Cannot open file '%1' for read-only.",
	     userDataDir + "savegame.dat"));
	return;
    }

    // Halt the game during the loadGame() dialog.
    modalFreeze = false;
    if (!KGrObject::frozen) {
	modalFreeze = true;
	freeze();
    }

    QString s;

    KGrLGDialog * lg = new KGrLGDialog (&savedGames, collections, view);

    if (lg->exec() == QDialog::Accepted) {
	s = lg->getCurrentText();
    }

    bool found = false;
    QString pr;
    int  lev;
    int i;
    int imax = collections.count();

    if (! s.isNull()) {
	pr = s.mid (21, 7);			// Get the collection prefix.
	pr = pr.left (pr.indexOf (" ", 0, Qt::CaseInsensitive));

	for (i = 0; i < imax; i++) {		// Find the collection.
	    if (collections.at(i)->prefix == pr) {
		collection = collections.at(i);
		collnIndex  = i;
		owner = collections.at(i)->owner;
		found = true;
		break;
	    }
	}
	if (found) {
	    // Set the rules for the selected game.
	    emit markRuleType (collection->settings);
	    lev   = s.mid (28, 3).toInt();
	    newGame (lev, collnIndex);		// Re-start the selected game.
	    lives = s.mid (32, 3).toLong();	// Update the lives.
	    emit showLives (lives);
	    score = s.mid (36, 7).toLong();	// Update the score.
	    emit showScore (score);
	}
	else {
	    KGrMessage::information (view, i18n("Load Game"),
		i18n("Cannot find the game with prefix '%1'.", pr));
	}
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
        unfreeze();
	modalFreeze = false;
    }

    delete lg;
}

/******************************************************************************/
/**************************  HIGH-SCORE PROCEDURES  ***************************/
/******************************************************************************/

void KGrGame::checkHighScore()
{
#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog(
	    KScoreDialog::Name | KScoreDialog::Level | 
	    KScoreDialog::Date | KScoreDialog::Score, 
	    view);
    scoreDialog.setConfigGroup(collection->prefix);
    KScoreDialog::FieldInfo scoreInfo;
    scoreInfo[KScoreDialog::Level].setNum(level);
    scoreInfo[KScoreDialog::Score].setNum(score);
    QDate today = QDate::currentDate();
    scoreInfo[KScoreDialog::Date] = today.toString("ddd yyyy MM dd");
    if (scoreDialog.addScore(scoreInfo)) {
	scoreDialog.exec();
    }
#else
    bool	prevHigh  = true;
    qint16	prevLevel = 0;
    qint32	prevScore = 0;
    QString	thisUser  = i18n("Unknown");
    int		highCount = 0;

    // Don't keep high scores for tutorial games.
    if (collection->prefix.left(4) == "tute")
	return;

    if (score <= 0)
	return;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
	high1.setFileName (systemDataDir + "hi_" + collection->prefix + ".dat");
	if (! high1.exists()) {
	    prevHigh = false;
	}
    }

    // If a previous high score file exists, check the current score against it.
    if (prevHigh) {
	if (! high1.open (QIODevice::ReadOnly)) {
	    QString high1_name = high1.fileName();
	    KGrMessage::information (view, i18n("Check for High Score"),
		i18n("Cannot open file '%1' for read-only.", high1_name));
	    return;
	}

	// Read previous users, levels and scores from the high score file.
	s1.setDevice (&high1);
	bool found = false;
	highCount = 0;
	while (! s1.endData()) {
	    char * prevUser;
	    char * prevDate;
	    s1 >> prevUser;
	    s1 >> prevLevel;
	    s1 >> prevScore;
	    s1 >> prevDate;
	    delete prevUser;
	    delete prevDate;
	    highCount++;
	    if (score > prevScore) {
		found = true;			// We have a high score.
		break;
	    }
	}

	// Check if higher than one on file or fewer than 10 previous scores.
	if ((! found) && (highCount >= 10)) {
	    return;				// We did not have a high score.
	}
    }

    /* ************************************************************* */
    /* If we have come this far, we have a new high score to record. */
    /* ************************************************************* */

    QFile high2 (userDataDir + "hi_" + collection->prefix + ".tmp");
    QDataStream s2;

    if (! high2.open (QIODevice::WriteOnly)) {
	KGrMessage::information (view, i18n("Check for High Score"),
		i18n("Cannot open file '%1' for output.",
		 userDataDir + "hi_" + collection->prefix + ".tmp"));
	return;
    }

    // Dialog to ask the user to enter their name.
    QDialog *		hsn = new QDialog (view,
			Qt::WindowTitleHint);
    hsn->setObjectName("hsNameDialog");

    int margin = 10;
    int spacing = 10;
    QVBoxLayout *	mainLayout = new QVBoxLayout (hsn);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QLabel *		hsnMessage  = new QLabel (
			i18n("<b>Congratulations !!!</b><br>  "
			"You have achieved a high "
			"score in this game.  <br>Please enter your name so that "
			"it may be enshrined <br>in the KGoldrunner Hall of Fame."),
			hsn);
    QLineEdit *		hsnUser = new QLineEdit (hsn);
    QPushButton *	OK = new KPushButton (KStandardGuiItem::ok(), hsn);

    mainLayout->	addWidget (hsnMessage);
    mainLayout->	addWidget (hsnUser);
    mainLayout->	addWidget (OK);

    hsn->		setWindowTitle (i18n("Save High Score"));

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    hsn->		move (p.x() + 50, p.y() + 50);

    OK->		setShortcut (Qt::Key_Return);
    hsnUser->		setFocus();		// Set the keyboard input on.

    connect	(hsnUser, SIGNAL (returnPressed ()), hsn, SLOT (accept ()));
    connect	(OK,      SIGNAL (clicked ()),       hsn, SLOT (accept ()));

    while (true) {
	hsn->exec();
	thisUser = hsnUser->text();
	if (thisUser.length() > 0)
	    break;
	KGrMessage::information (view, i18n("Save High Score"),
			i18n("You must enter something.  Please try again."));
    }

    delete hsn;

    QDate today = QDate::currentDate();
    QString hsDate;
    QString day = today.shortDayName(today.dayOfWeek());
    hsDate = hsDate.sprintf
		("%s %04d-%02d-%02d",
		day.myStr(),
		today.year(), today.month(), today.day());

    s2.setDevice (&high2);

    if (prevHigh) {
	high1.reset();
	bool scoreRecorded = false;
	highCount = 0;
	while ((! s1.endData()) && (highCount < 10)) {
	    char * prevUser;
	    char * prevDate;
	    s1 >> prevUser;
	    s1 >> prevLevel;
	    s1 >> prevScore;
	    s1 >> prevDate;
	    if ((! scoreRecorded) && (score > prevScore)) {
		highCount++;
		// Recode the user's name as UTF-8, in case it contains
		// non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
		s2 << thisUser.toUtf8().constData();
		s2 << (qint16) level;
		s2 << (qint32) score;
		s2 << hsDate.myStr();
		scoreRecorded = true;
	    }
	    if (highCount < 10) {
		highCount++;
		s2 << prevUser;
		s2 << prevLevel;
		s2 << prevScore;
		s2 << prevDate;
	    }
	    delete prevUser;
	    delete prevDate;
	}
	if ((! scoreRecorded) && (highCount < 10)) {
	    // Recode the user's name as UTF-8, in case it contains
	    // non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
	    s2 << thisUser.toUtf8().constData();
	    s2 << (qint16) level;
	    s2 << (qint32) score;
	    s2 << hsDate.myStr();
	}
	high1.close();
    }
    else {
	// Recode the user's name as UTF-8, in case it contains
	// non-ASCII chars (e.g. "Krüger" is encoded as "KrÃ¼ger").
	s2 << thisUser.toUtf8().constData();
	s2 << (qint16) level;
	s2 << (qint32) score;
	s2 << hsDate.myStr();
    }

    high2.close();

    if (safeRename (high2.fileName(),
		userDataDir + "hi_" + collection->prefix + ".dat")) {
	KGrMessage::information (view, i18n("Save High Score"),
				i18n("Your high score has been saved."));
    }
    else {
	KGrMessage::information (view, i18n("Save High Score"),
				i18n("Error: Failed to save your high score."));
    }

    showHighScores();
    return;
#endif
}

void KGrGame::showHighScores()
{
#ifdef USE_KSCOREDIALOG
    KScoreDialog scoreDialog(
	    KScoreDialog::Name | KScoreDialog::Level | 
	    KScoreDialog::Date | KScoreDialog::Score, 
	    view);
    scoreDialog.exec();
#else
    // Don't keep high scores for tutorial games.
    if (collection->prefix.left(4) == "tute") {
	KGrMessage::information (view, i18n("Show High Scores"),
		i18n("Sorry, we do not keep high scores for tutorial games."));
	return;
    }

    qint16	prevLevel = 0;
    qint32	prevScore = 0;
    int		n = 0;

    // Look for user's high-score file or for a released high-score file.
    QFile high1 (userDataDir + "hi_" + collection->prefix + ".dat");
    QDataStream s1;

    if (! high1.exists()) {
	high1.setFileName (systemDataDir + "hi_" + collection->prefix + ".dat");
	if (! high1.exists()) {
	    KGrMessage::information (view, i18n("Show High Scores"),
		    i18n("Sorry, there are no high scores for the \"%1\" game yet.",
		         collection->name));
	    return;
	}
    }

    if (! high1.open (QIODevice::ReadOnly)) {
	QString high1_name = high1.fileName();
	KGrMessage::information (view, i18n("Show High Scores"),
	    i18n("Cannot open file '%1' for read-only.", high1_name));
	return;
    }

    QDialog *		hs = new QDialog (view,
			Qt::WindowTitleHint);
    hs->setObjectName("hsDialog");

    int margin = 10;
    int spacing = 10;
    QVBoxLayout *	mainLayout = new QVBoxLayout (hs);
    mainLayout->setSpacing(spacing);
    mainLayout->setMargin(margin);

    QLabel *		hsHeader = new QLabel (i18n (
					"<center><h2>KGoldrunner Hall of Fame</h2></center>"
					"<center><h3>\"%1\" Game</h3></center>",
					 collection->name),
			hs);
    QLabel *		hsColHeader  = new QLabel (
				i18n("    Name                          "
				"Level  Score       Date"), hs);
#ifdef KGR_PORTABLE
    QFont		f ("courier", 12);
#else
    QFont		f = KGlobalSettings::fixedFont();	// KDE version.
#endif
    f.			setFixedPitch (true);
    f.			setBold (true);
    hsColHeader->	setFont (f);

    QLabel *		hsLine [10];

    mainLayout->	addWidget (hsHeader);
    mainLayout->	addWidget (hsColHeader);

    hs->		setWindowTitle (i18n("High Scores"));

    // Set up the format for the high-score lines.
    f.			setBold (false);
    QString		line;
    const char *	hsFormat = "%2d. %-30.30s %3d %7ld  %s";

    // Read and display the users, levels and scores from the high score file.
    s1.setDevice (&high1);
    n = 0;
    while ((! s1.endData()) && (n < 10)) {
	char * prevUser;
	char * prevDate;
	s1 >> prevUser;
	s1 >> prevLevel;
	s1 >> prevScore;
	s1 >> prevDate;

	// QString::sprintf expects UTF-8 encoding in its string arguments, so
	// prevUser has been saved on file as UTF-8 to allow non=ASCII chars
	// in the user's name (e.g. "Krüger" is encoded as "KrÃ¼ger" in UTF-8).

	line = line.sprintf (hsFormat,
			     n+1, prevUser, prevLevel, prevScore, prevDate);
	hsLine [n] = new QLabel (line, hs);
	hsLine [n]->setFont (f);
	mainLayout->addWidget (hsLine [n]);

	delete prevUser;
	delete prevDate;
	n++;
    }

    QFrame * separator = new QFrame (hs);
    separator->setFrameStyle (QFrame::HLine + QFrame::Sunken);
    mainLayout->addWidget (separator);

    QHBoxLayout *hboxLayout1 = new QHBoxLayout();
    hboxLayout1->setSpacing (spacing);
    QSpacerItem * spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout1->addItem(spacerItem);
    QPushButton *	OK = new KPushButton (KStandardGuiItem::close(), hs);
    OK->		setShortcut (Qt::Key_Return);
    OK->		setMaximumWidth (100);
    hboxLayout1->addWidget (OK);
    mainLayout->	addLayout (hboxLayout1);

    QPoint		p = view->mapToGlobal (QPoint (0,0));
    hs->		move (p.x() + 50, p.y() + 50);

    // Start up the dialog box.
    connect		(OK, SIGNAL (clicked ()), hs, SLOT (accept ()));
    hs->		exec();

    delete hs;
#endif
}

/******************************************************************************/
/**************************  AUTHORS' DEBUGGING AIDS **************************/
/******************************************************************************/

void KGrGame::doStep()
{
    if (KGrObject::frozen) {	// The game must have been halted.
	restart();		// Do one step and halt again.
    }
}

void KGrGame::restart()
{
    bool temp;
    int i,j;

    if (editMode)		// Can't move figures when in Edit Mode.
	return;

    temp = KGrObject::frozen;

    KGrObject::frozen = false;	// Temporarily restart the game, by re-running
				// any timer events that have been blocked.

    readMousePos();		// Set hero's direction.
    hero->doStep();		// Move the hero one step.

    j = enemies.count();	// Move each enemy one step.
    for (i = 0; i < j; i++) {
	enemy = enemies.at(i);	// Need to use an index because called methods
	enemy->doStep();	// change the "current()" of the "enemies" list.
    }

    for (i=1; i<=28; i++)
	for (j=1; j<=20; j++) {
	    if ((playfield[i][j]->whatIam() == HOLE) ||
		(playfield[i][j]->whatIam() == USEDHOLE) ||
		(playfield[i][j]->whatIam() == BRICK))
		((KGrBrick *)playfield[i][j])->doStep();
    }

    KGrObject::frozen = temp;	// If frozen was true, halt again, which gives a
				// single-step effect, otherwise go on running.
}

void KGrGame::showFigurePositions()
{
    if (KGrObject::frozen) {
	hero->showState('p');
	QListIterator<KGrEnemy *> i(enemies);
        while (i.hasNext()){
		KGrEnemy * enemy = i.next();
		enemy->showState('p');
	}
    }
}

void KGrGame::showHeroState()
{
    if (KGrObject::frozen) {
	hero->showState('s');
    }
}

void KGrGame::showEnemyState(int enemyId)
{
    if (KGrObject::frozen) {
	QListIterator<KGrEnemy *> i(enemies);
        while (i.hasNext()){
		KGrEnemy * enemy = i.next();
		if (enemy->enemyId == enemyId) enemy->showState('s');
	}
    }
}

void KGrGame::showObjectState()
{
    QPoint p;
    int i, j;
    KGrObject * myObject;

    if (KGrObject::frozen) {
	p = view->getMousePos ();
	i = p.x(); j = p.y();
	myObject = playfield[i][j];
	switch (myObject->whatIam()) {
	    case BRICK:
	    case HOLE:
	    case USEDHOLE:
		 ((KGrBrick *)myObject)->showState(i, j); break;
	    default: myObject->showState(i, j); break;
	}
    }
}

void KGrGame::bugFix()
{
    if (KGrObject::frozen) {		// Toggle a bug fix on/off dynamically.
	KGrObject::bugFixed = (KGrObject::bugFixed) ? false : true;
	printf ("%s", (KGrObject::bugFixed) ? "\n" : "");
	printf (">>> Bug fix is %s\n", (KGrObject::bugFixed) ? "ON" : "OFF\n");
    }
}

void KGrGame::startLogging()
{
    if (KGrObject::frozen) {		// Toggle logging on/off dynamically.
	KGrObject::logging = (KGrObject::logging) ? false : true;
	printf ("%s", (KGrObject::logging) ? "\n" : "");
	printf (">>> Logging is %s\n", (KGrObject::logging) ? "ON" : "OFF\n");
    }
}

/******************************************************************************/
/************  GAME EDITOR FUNCTIONS ACTIVATED BY MENU OR TOOLBAR  ************/
/******************************************************************************/

void KGrGame::setEditObj (char newEditObj)
{
    editObj = newEditObj;
}

void KGrGame::createLevel()
{
    int	i, j;

    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }

    if (! ownerOK (USER)) {
	KGrMessage::information (view, i18n("Create Level"),
		i18n("You cannot create and save a level "
		"until you have created a game to hold "
		"it. Try menu item \"Create Game\"."));
	return;
    }

    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

    level = 0;
    initEdit();
    levelName = "";
    levelHint = "";

    // Clear the playfield.
    editObj = FREE;
    for (i = 1; i <= FIELDWIDTH; i++)
    for (j = 1; j <= FIELDHEIGHT; j++) {
	insertEditObj (i, j, FREE);
	editObjArray[i][j] = FREE;
    }

    insertEditObj (1, 1, HERO);
    editObjArray[1][1] = HERO;
    editObj = BRICK;

    showEditLevel();

    for (j = 1; j <= FIELDHEIGHT; j++)
    for (i = 1; i <= FIELDWIDTH; i++) {
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }

    // Re-enable player input.
    loading = false;

    //view->updateCanvas();				// Show the edit area.
    view->update();					// Show the level name.
}

void KGrGame::updateLevel()
{
    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }

    if (! ownerOK (USER)) {
	KGrMessage::information (view, i18n("Edit Level"),
		i18n("You cannot edit and save a level until you "
		"have created a game and a level. Try menu item \"Create Game\"."));
	return;
    }

    if (level < 0) level = 0;
    int lev = selectLevel (SL_UPDATE, level);
    if (lev == 0)
	return;

    if (owner == SYSTEM) {
	KGrMessage::information (view, i18n("Edit Level"),
	    i18n("It is OK to edit a system level, but you MUST save "
	    "the level in one of your own games. You're not just "
	    "taking a peek at the hidden ladders "
	    "and fall-through bricks, are you? :-)"));
    }

    loadEditLevel (lev);
}

void KGrGame::updateNext()
{
    if (! saveOK (false)) {				// Check unsaved work.
	return;
    }
    level++;
    updateLevel();
}

void KGrGame::loadEditLevel (int lev)
{
    // Ignore player input from keyboard or mouse while the screen is set up.
    loading = true;

    // Read the level data.
    LevelData d;
    if (! readLevelData (lev, d)) {
	loading = false;
	return;
    }

    level = lev;
    initEdit();

    int i, j;
    // Load the level.
    for (j = 1; j <= FIELDHEIGHT; j++)
    for (i = 1; i <= FIELDWIDTH;  i++) {
	editObj = d.layout.at ((j-1)*FIELDWIDTH + (i-1));
	insertEditObj (i, j, editObj);
	editObjArray[i][j] = editObj;
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }

    // Retain the original language of the name and hint when editing,
    // but convert non-ASCII, UTF-8 substrings to Unicode (eg. Ã¼ to ü).
    levelName = (d.name.size() > 0) ?
		QString::fromUtf8((const char *) d.name) : "";
    levelHint = (d.hint.size() > 0) ?
		QString::fromUtf8((const char *) d.hint) : "";

    editObj = BRICK;				// Reset default object.

    view->setTitle (getTitle());		// Show the level name.
    showEditLevel();				// Reconnect signals.

    // Re-enable player input.
    loading = false;
}

void KGrGame::editNameAndHint()
{
    if (! editMode)
	return;

    // Run a dialog box to create/edit the level name and hint.
    KGrNHDialog * nh = new KGrNHDialog (levelName, levelHint, view);

    if (nh->exec() == QDialog::Accepted) {
	levelName = nh->getName();
	levelHint = nh->getHint();
	shouldSave = true;
    }

    delete nh;
}

bool KGrGame::saveLevelFile()
{
    bool isNew;
    int action;
    int selectedLevel = level;

    int i, j;
    QString filePath;

    if (! editMode) {
	KGrMessage::information (view, i18n("Save Level"),
		i18n("Inappropriate action: you are not editing a level."));
	return (false);
    }

    // Save the current collection index.
    int N = collnIndex;

    if (selectedLevel == 0) {
	// New level: choose a number.
	action = SL_CREATE;
    }
    else {
	// Existing level: confirm the number or choose a new number.
	action = SL_SAVE;
    }

    // Pop up dialog box, which could change the collection or level or both.
    selectedLevel = selectLevel (action, selectedLevel);
    if (selectedLevel == 0)
	return (false);

    // Get the new collection (if changed).
    int n = collnIndex;

    // Set the name of the output file.
    filePath = getFilePath (owner, collection, selectedLevel);
    QFile levelFile (filePath);

    if ((action == SL_SAVE) && (n == N) && (selectedLevel == level)) {
	// This is a normal edit: the old file is to be re-written.
	isNew = false;
    }
    else {
	isNew = true;
	// Check if the file is to be inserted in or appended to the collection.
	if (levelFile.exists()) {
	    switch (KGrMessage::warning (view, i18n("Save Level"),
			i18n("Do you want to insert a level and "
			"move existing levels up by one?"),
			i18n("&Insert Level"), i18n("&Cancel"))) {

	    case 0:	if (! reNumberLevels (n, selectedLevel,
					    collections.at(n)->nLevels, +1)) {
			    return (false);
			}
			break;
	    case 1:	return (false);
			break;
	    }
	}
    }

    // Open the output file.
    if (! levelFile.open (QIODevice::WriteOnly)) {
	KGrMessage::information (view, i18n("Save Level"),
		i18n("Cannot open file '%1' for output.", filePath));
	return (false);
    }

    // Save the level.
    for (j = 1; j < 21; j++)
    for (i = 1; i < 29; i++) {
	levelFile.putChar (editObjArray[i][j]);
	lastSaveArray[i][j] = editObjArray[i][j];	// Copy for "saveOK()".
    }
    levelFile.putChar ('\n');

    // Save the level name, changing non-ASCII chars to UTF-8 (eg. ü to Ã¼).
    QByteArray levelNameC = levelName.toUtf8();
    int len1 = levelNameC.length();
    if (len1 > 0) {
	for (i = 0; i < len1; i++)
	    levelFile.putChar (levelNameC[i]);
	levelFile.putChar ('\n');			// Add a newline.
    }

    // Save the level hint, changing non-ASCII chars to UTF-8 (eg. ü to Ã¼).
    QByteArray levelHintC = levelHint.toUtf8();
    int len2 = levelHintC.length();
    char ch = '\0';

    if (len2 > 0) {
	if (len1 <= 0)
	    levelFile.putChar ('\n');		// Leave blank line for name.
	for (i = 0; i < len2; i++) {
	    ch = levelHintC[i];
	    levelFile.putChar (ch);		// Copy the character.
	}
	if (ch != '\n')
	    levelFile.putChar ('\n');		// Add a newline character.
    }

    levelFile.close ();
    shouldSave = false;

    if (isNew) {
	collections.at(n)->nLevels++;
	saveCollections (owner);
    }

    level = selectedLevel;
    emit showLevel (level);
    view->setTitle (getTitle());		// Display new title.
    //view->updateCanvas();			// Show the edit area.
    return (true);
}

void KGrGame::moveLevelFile ()
{
    if (level <= 0) {
	KGrMessage::information (view, i18n("Move Level"),
		i18n("You must first load a level to be moved. Use "
		     "the \"%1\" or \"%2\" menu.",
                     i18n("Game"), i18n("Editor")));
	return;
    }

    int action = SL_MOVE;

    int fromC = collnIndex;
    int fromL = level;
    int toC   = fromC;
    int toL   = fromL;

    if (! ownerOK (USER)) {
	KGrMessage::information (view, i18n("Move Level"),
		i18n("You cannot move a level until you "
		"have created a game and at least two levels. Try "
		"menu item \"Create Game\"."));
	return;
    }

    if (collections.at(fromC)->owner != USER) {
	KGrMessage::information (view, i18n("Move Level"),
		i18n("Sorry, you cannot move a system level."));
	return;
    }

    // Pop up dialog box to get the collection and level number to move to.
    while ((toC == fromC) && (toL == fromL)) {
	toL = selectLevel (action, toL);
	if (toL == 0)
	    return;

	toC = collnIndex;

	if ((toC == fromC) && (toL == fromL)) {
	    KGrMessage::information (view, i18n("Move Level"),
		    i18n("You must change the level or the game or both."));
	}
    }

    QString filePath1;
    QString filePath2;

    // Save the "fromN" file under a temporary name.
    filePath1 = getFilePath (USER, collections.at(fromC), fromL);
    filePath2 = filePath1;
    filePath2 = filePath2.append (".tmp");
    if (! safeRename (filePath1, filePath2))
	return;

    if (toC == fromC) {					// Same collection.
	if (toL < fromL) {				// Decrease level.
	    // Move "toL" to "fromL - 1" up by 1.
	    if (! reNumberLevels (toC, toL, fromL-1, +1)) {
		return;
	    }
	}
	else {						// Increase level.
	    // Move "fromL + 1" to "toL" down by 1.
	    if (! reNumberLevels (toC, fromL+1, toL, -1)) {
		return;
	    }
	}
    }
    else {						// Different collection.
	// In "fromC", move "fromL + 1" to "nLevels" down and update "nLevels".
	if (! reNumberLevels (fromC, fromL + 1,
				    collections.at(fromC)->nLevels, -1)) {
	    return;
	}
	collections.at(fromC)->nLevels--;

	// In "toC", move "toL + 1" to "nLevels" up and update "nLevels".
	if (! reNumberLevels (toC, toL, collections.at(toC)->nLevels, +1)) {
	    return;
	}
	collections.at(toC)->nLevels++;

	saveCollections (USER);
    }

    // Rename the saved "fromL" file to become "toL".
    filePath1 = getFilePath (USER, collections.at(toC), toL);
    safeRename (filePath2, filePath1); // IDW

    level = toL;
    collection = collections.at(toC);
    view->setTitle (getTitle());	// Re-write title.
    //view->updateCanvas();		// Re-display details of level.
    emit showLevel (level);
}

void KGrGame::deleteLevelFile ()
{
    int action = SL_DELETE;
    int lev = level;

    if (! ownerOK (USER)) {
	KGrMessage::information (view, i18n("Delete Level"),
		i18n("You cannot delete a level until you "
		"have created a game and a level. Try "
		"menu item \"Create Game\"."));
	return;
    }

    // Pop up dialog box to get the collection and level number.
    lev = selectLevel (action, level);
    if (lev == 0)
	return;

    QString filePath;

    // Set the name of the file to be deleted.
    int n = collnIndex;
    filePath = getFilePath (USER, collections.at(n), lev);
    QFile levelFile (filePath);

    // Delete the file for the selected collection and level.
    if (levelFile.exists()) {
	if (lev < collections.at(n)->nLevels) {
	    switch (KGrMessage::warning (view, i18n("Delete Level"),
				i18n("Do you want to delete a level and "
				"move higher levels down by one?"),
				i18n("&Delete Level"), i18n("&Cancel"))) {
	    case 0:	break;
	    case 1:	return; break;
	    }
	    levelFile.remove ();
	    if (! reNumberLevels (n, lev + 1, collections.at(n)->nLevels, -1)) {
		return;
	    }
	}
	else {
	    levelFile.remove ();
	}
    }
    else {
	KGrMessage::information (view, i18n("Delete Level"),
		i18n("Cannot find file '%1' to be deleted.", filePath));
	return;
    }

    collections.at(n)->nLevels--;
    saveCollections (USER);
    if (lev <= collections.at(n)->nLevels) {
	level = lev;
    }
    else {
	level = collections.at(n)->nLevels;
    }

    // Repaint the screen with the level that now has the selected number.
    if (editMode && (level > 0)) {
	loadEditLevel (level);			// Load level in edit mode.
    }
    else if (level > 0) {
	enemyCount = 0;				// Load level in play mode.
	//enemies.clear();
	while (!enemies.isEmpty())
        	delete enemies.takeFirst();

	view->deleteEnemySprites();
	newLevel = true;;
	loadLevel (level);
	newLevel = false;
    }
    else {
	createLevel();				// No levels left in collection.
    }
    emit showLevel (level);
}

void KGrGame::editCollection (int action)
{
    int lev = level;
    int n = -1;

    // If editing, choose a collection.
    if (action == SL_UPD_GAME) {
	lev = selectLevel (SL_UPD_GAME, level);
	if (lev == 0)
	    return;
	level = lev;
	n = collnIndex;
    }

    KGrECDialog * ec = new KGrECDialog (action, n, collections, view);

    while (ec->exec() == QDialog::Accepted) {	// Loop until valid.

	// Validate the collection details.
	QString ecName = ec->getName();
	int len = ecName.length();
	if (len == 0) {
	    KGrMessage::information (view, i18n("Save Game Info"),
		i18n("You must enter a name for the game."));
	    continue;
	}

	QString ecPrefix = ec->getPrefix();
	if ((action == SL_CR_GAME) || (collections.at(n)->nLevels <= 0)) {
	    // The filename prefix could have been entered, so validate it.
	    len = ecPrefix.length();
	    if (len == 0) {
		KGrMessage::information (view, i18n("Save Game Info"),
		    i18n("You must enter a filename prefix for the game."));
		continue;
	    }
	    if (len > 5) {
		KGrMessage::information (view, i18n("Save Game Info"),
		    i18n("The filename prefix should not "
		    "be more than 5 characters."));
		continue;
	    }

	    bool allAlpha = true;
	    for (int i = 0; i < len; i++) {
		if (! isalpha(ecPrefix.myChar(i))) {
		    allAlpha = false;
		    break;
		}
	    }
	    if (! allAlpha) {
		KGrMessage::information (view, i18n("Save Game Info"),
		    i18n("The filename prefix should be "
		    "all alphabetic characters."));
		continue;
	    }

	    bool duplicatePrefix = false;
	    KGrCollection * c;
	    int imax = collections.count();
	    for (int i = 0; i < imax; i++) {
		c = collections.at(i);
		if ((c->prefix == ecPrefix) && (i != n)) {
		    duplicatePrefix = true;
		    break;
		}
	    }

	    if (duplicatePrefix) {
		KGrMessage::information (view, i18n("Save Game Info"),
		    i18n("The filename prefix '%1' is already in use.",
		     ecPrefix));
		continue;
	    }
	}

	// Save the collection details.
	char settings = 'K';
	if (ec->isTrad()) {
	    settings = 'T';
	}
	if (action == SL_CR_GAME) {
	    collections.append (new KGrCollection (USER,
		ecName, ecPrefix, settings, 0, ec->getAboutText(), 'N'));
	}
	else {
	    collection->name		= ecName;
	    collection->prefix		= ecPrefix;
	    collection->settings	= settings;
	    collection->about		= ec->getAboutText();
	}

	saveCollections (USER);
	break;				// All done now.
    }

    delete ec;
}

/******************************************************************************/
/*********************  SUPPORTING GAME EDITOR FUNCTIONS  *********************/
/******************************************************************************/

bool KGrGame::saveOK (bool exiting)
{
    int		i, j;
    bool	result;
    QString	option2 = i18n("&Go on editing");

    result = true;

    if (editMode) {
	if (exiting) {					// If window is closing,
	    option2 = "";				// can't go on editing.
	}
	for (j = 1; j <= FIELDHEIGHT; j++)
	for (i = 1; i <= FIELDWIDTH; i++) {		// Check cell changes.
	    if ((shouldSave) || (editObjArray[i][j] != lastSaveArray[i][j])) {
		// If shouldSave == true, level name or hint was edited.
		switch (KGrMessage::warning (view, i18n("Editor"),
			i18n("You have not saved your work. Do "
			"you want to save it now?"),
			i18n("&Save"), i18n("&Do Not Save"), option2)) {
		case 0: result = saveLevelFile(); break;// Save and continue.
		case 1: shouldSave = false; break;	// Continue: don't save.
		case 2: result = false; break;		// Go back to editing.
		}
		return (result);
	    }
	}
    }
    return (result);
}

void KGrGame::initEdit()
{
    if (! editMode) {

	editMode = true;
	emit setEditMenu (true);	// Enable edit menu items and toolbar.

	// We were previously in play mode: stop the hero running or falling.
	hero->init (1, 1);
	view->setHeroVisible (false);
    }

    paintEditObj = false;

    // Set the default object and button.
    editObj = BRICK;
    emit defaultEditObj();	// Set default edit-toolbar button.

    oldI = 0;
    oldJ = 0;
    heroCount = 0;
    enemyCount = 0;
    //enemies.clear();
    while (!enemies.isEmpty())
        delete enemies.takeFirst();

    view->deleteEnemySprites();
    nuggets = 0;

    emit showLevel (level);
    emit showLives (0);
    emit showScore (0);

    deleteLevel();
    setBlankLevel(false);	// Fill playfield with Editable objects.

    view->setTitle (getTitle());// Show title of level.
    //view->updateCanvas();	// Show the edit area.

    shouldSave = false;		// Used to flag editing of name or hint.
}

void KGrGame::deleteLevel()
{
    int i,j;
    for (i = 1; i <= FIELDHEIGHT; i++)
    for (j = 1; j <= FIELDWIDTH; j++)
	delete playfield[j][i];
}

void KGrGame::insertEditObj (int i, int j, char obj)
{
    if ((i < 1) || (j < 1) || (i > FIELDWIDTH) || (j > FIELDHEIGHT))
	return;		// Do nothing: mouse pointer is out of playfield.

    if (editObjArray[i][j] == HERO) {
	// The hero is in this cell: remove him.
	editObjArray[i][j] = FREE;
	heroCount = 0;
    }

    if (editObj == HERO) {
	if (heroCount != 0) {
	    // Can only have one hero: remove him from his previous position.
	    for (int m = 1; m <= FIELDWIDTH; m++)
	    for (int n = 1; n <= FIELDHEIGHT; n++) {
		if (editObjArray[m][n] == HERO) {
		    setEditableCell (m, n, FREE);
		}
	    }
	}
	heroCount = 1;
    }

    setEditableCell (i, j, obj);
}

void KGrGame::setEditableCell (int i, int j, char type)
{
    ((KGrEditable *) playfield[i][j])->setType (type);
    view->paintCell (i, j, type);
    editObjArray[i][j] = type;
}

void KGrGame::showEditLevel()
{
    // Disconnect play-mode slots from signals from "view".
    disconnect (view, SIGNAL(mouseClick(int)), 0, 0);
    disconnect (view, SIGNAL(mouseLetGo(int)), 0, 0);

    // Connect edit-mode slots to signals from "view".
    connect (view, SIGNAL(mouseClick(int)), SLOT(doEdit(int)));
    connect (view, SIGNAL(mouseLetGo(int)), SLOT(endEdit(int)));
}

bool KGrGame::reNumberLevels (int cIndex, int first, int last, int inc)
{
    int i, n, step;
    QString file1, file2;

    if (inc > 0) {
	i = last;
	n = first - 1;
	step = -1;
    }
    else {
	i = first;
	n = last + 1;
	step = +1;
    }

    while (i != n) {
	file1 = getFilePath (USER, collections.at(cIndex), i);
	file2 = getFilePath (USER, collections.at(cIndex), i - step);
	if (! safeRename (file1, file2)) {
	    return (false);
	}
	i = i + step;
    }

    return (true);
}

void KGrGame::setLevel (int lev)
{
    level = lev;
    return;
}

/******************************************************************************/
/*********************   EDIT ACTION SLOTS   **********************************/
/******************************************************************************/

void KGrGame::doEdit (int button)
{
    // Mouse button down: start making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos ();
    i = p.x(); j = p.y();

    switch (button) {
    case Qt::LeftButton:
        paintEditObj = true;
        insertEditObj (i, j, editObj);
	//view->updateCanvas();
        oldI = i;
        oldJ = j;
        break;
    case Qt::RightButton:
        paintAltObj = true;
        insertEditObj (i, j, FREE);
	//view->updateCanvas();
        oldI = i;
        oldJ = j;
        break;
    default:
        break;
    }
}

void KGrGame::endEdit (int button)
{
    // Mouse button released: finish making changes.
    QPoint p;
    int i, j;

    p = view->getMousePos ();
    i = p.x(); j = p.y();

    switch (button) {
    case Qt::LeftButton:
        paintEditObj = false;
        if ((i != oldI) || (j != oldJ)) {
	    insertEditObj (i, j, editObj);
	    //view->updateCanvas();
	}
        break;
    case Qt::RightButton:
        paintAltObj = false;
        if ((i != oldI) || (j != oldJ)) {
	    insertEditObj (i, j, FREE);
	    //view->updateCanvas();
	}
        break;
    default:
        break;
    }
}

/******************************************************************************/
/**********************    LEVEL SELECTION DIALOG BOX    **********************/
/******************************************************************************/

int KGrGame::selectLevel (int action, int requestedLevel)
{
    int selectedLevel = 0;		// 0 = no selection (Cancel) or invalid.

    // Halt the game during the dialog.
    modalFreeze = false;
    if (! KGrObject::frozen) {
	modalFreeze = true;
	freeze();
    }

    // Create and run a modal dialog box to select a game and level.
    KGrSLDialog * sl = new KGrSLDialog (action, requestedLevel, collnIndex,
					collections, this, view);
    while (sl->exec() == QDialog::Accepted) {
	selectedGame = sl->selectedGame();
	selectedLevel = 0;	// In case the selection is invalid.
	if (collections.at(selectedGame)->owner == SYSTEM) {
	    switch (action) {
	    case SL_CREATE:	// Can save only in a USER collection.
	    case SL_SAVE:
	    case SL_MOVE:
		KGrMessage::information (view, i18n("Select Level"),
			i18n("Sorry, you can only save or move "
			"into one of your own games."));
		continue;			// Re-run the dialog box.
		break;
	    case SL_DELETE:	// Can delete only in a USER collection.
		KGrMessage::information (view, i18n("Select Level"),
			i18n("Sorry, you can only delete a level "
			"from one of your own games."));
		continue;			// Re-run the dialog box.
		break;
	    case SL_UPD_GAME:	// Can edit info only in a USER collection.
		KGrMessage::information (view, i18n("Edit Game Info"),
			i18n("Sorry, you can only edit the game "
			"information on your own games."));
		continue;			// Re-run the dialog box.
		break;
	    default:
		break;
	    }
	}

	selectedLevel = sl->selectedLevel();
	if ((selectedLevel > collections.at (selectedGame)->nLevels) &&
	    (action != SL_CREATE) && (action != SL_SAVE) &&
	    (action != SL_MOVE) && (action != SL_UPD_GAME)) {
	    KGrMessage::information (view, i18n("Select Level"),
		i18n("There is no level %1 in %2, "
		"so you cannot play or edit it.",
		 selectedLevel,
		 "\"" + collections.at(selectedGame)->name + "\""));
	    selectedLevel = 0;			// Set an invalid selection.
	    continue;				// Re-run the dialog box.
	}

	// If "OK", set the results.
	collection = collections.at (selectedGame);
	owner = collection->owner;
	collnIndex = selectedGame;
	// Set default rules for selected game.
	emit markRuleType (collection->settings);
	break;
    }

    // Unfreeze the game, but only if it was previously unfrozen.
    if (modalFreeze) {
	unfreeze();
	modalFreeze = false;
    }

    delete sl;
    return (selectedLevel);			// 0 = canceled or invalid.
}

bool KGrGame::ownerOK (Owner o)
{
    // Check that this owner has at least one collection.
    bool OK = false;

    QListIterator<KGrCollection *> i(collections);
    while (i.hasNext()){
	KGrCollection * c = i.next();
	if (c->owner == o) {
	    OK = true;
	    break;
	}		// Pit is blocked.  Find another way.
    }

    return (OK);
}

/******************************************************************************/
/**********************    CLASS TO DISPLAY THUMBNAIL   ***********************/
/******************************************************************************/

KGrThumbNail::KGrThumbNail (QWidget * parent, const char * name)
			: QFrame (parent)
{
    setObjectName(name);
    // Let the parent do all the work.  We need a class here so that
    // QFrame::drawContents (QPainter *) can be re-implemented and
    // the thumbnail can be automatically re-painted when required.
}

void KGrThumbNail::setLevelData (const QString& dir, const QString& prefix, int level,
					QLabel * sln)
{
    KGrGameIO io;
    LevelData d;

    IOStatus stat = io.fetchLevelData (dir, prefix, level, d);
    if (stat == OK) {
	// Keep a safe copy of the layout.  Translate and display the name.
	levelLayout = d.layout;
	sln->setText ((d.name.size() > 0) ? i18n((const char *) d.name) : "");
    }
    else {
	// Level-data inaccessible or not found.
	levelLayout = "";
	sln->setText ("");
    }
}

// This was previously a Q3Frame event
//     void KGrThumbNail::drawContents (QPainter * p)
//
// In Qt4 there is no longer such a method for QFrame.  The
// workaround is to reimplement paintEvent for this widget.

void KGrThumbNail::paintEvent (QPaintEvent * /* event (unused) */)
{
    QPainter    p (this);
    QFile	openFile;
    QPen	pen = p.pen();
    char	obj = FREE;
    int		fw = 1;				// Set frame width.
    int		n = width() / FIELDWIDTH;	// Set thumbnail cell-size.

    QColor backgroundColor = QColor ("#000038"); // Midnight blue.
    QColor brickColor =      QColor ("#9c0f0f"); // Oxygen's brick-red.
    QColor concreteColor =   QColor ("#585858"); // Dark grey.
    QColor ladderColor =     QColor ("#a0a0a0"); // Steely grey.
    QColor poleColor =       QColor ("#a0a0a0"); // Steely grey.
    QColor heroColor =       QColor ("#00ff00"); // Green.
    QColor enemyColor =      QColor ("#0080ff"); // Bright blue.
    QColor gold;
    gold.setNamedColor ("gold");		 // Gold.

    pen.setColor (backgroundColor);
    p.setPen (pen);

    if (levelLayout.isEmpty()) {
	// There is no file, so fill the thumbnail with "FREE" cells.
	p.drawRect (QRect(fw, fw, FIELDWIDTH*n, FIELDHEIGHT*n));
	return;
    }

    for (int j = 0; j < FIELDHEIGHT; j++)
    for (int i = 0; i < FIELDWIDTH; i++) {

	obj = levelLayout.at (j*FIELDWIDTH + i);

	// Set the colour of each object.
	switch (obj) {
	case BRICK:
	case FBRICK:
	    pen.setColor (brickColor); p.setPen (pen); break;
	case BETON:
	    pen.setColor (concreteColor); p.setPen (pen); break;
	case LADDER:
	    pen.setColor (ladderColor); p.setPen (pen); break;
	case POLE:
	    pen.setColor (poleColor); p.setPen (pen); break;
	case HERO:
	    pen.setColor (heroColor); p.setPen (pen); break;
	case ENEMY:
	    pen.setColor (enemyColor); p.setPen (pen); break;
	default:
	    // Set the background colour for FREE, HLADDER and NUGGET.
	    pen.setColor (backgroundColor); p.setPen (pen); break;
	}

	// Draw n x n pixels as n lines of length n.
	p.drawLine (i*n+fw, j*n+fw, i*n+(n-1)+fw, j*n+fw);
	if (obj == POLE) {
	    // For a pole, only the top line is drawn in white.
	    pen.setColor (backgroundColor);
	    p.setPen (pen);
	}
	for (int k = 1; k < n; k++) {
	    p.drawLine (i*n+fw, j*n+k+fw, i*n+(n-1)+fw, j*n+k+fw);
	}

	// For a nugget, add just a vertical touch  of yellow (2-3 pixels).
	if (obj == NUGGET) {
	    int k = (n/2)+fw;
	    pen.setColor (gold);	// Gold.
	    p.setPen (pen);
	    p.drawLine (i*n+k, j*n+k, i*n+k, j*n+(n-1)+fw);
	    p.drawLine (i*n+k+1, j*n+k, i*n+k+1, j*n+(n-1)+fw);
	}
    }

    // Finally, draw a small black border around the outside of the thumbnail.
    pen.setColor (Qt::black); 
    p.setPen (pen);
    p.drawRect (rect().left(), rect().top(), rect().right(), rect().bottom());
}

/******************************************************************************/
/*************************   COLLECTIONS HANDLING   ***************************/
/******************************************************************************/

// NOTE: Macros "myStr" and "myChar", defined in "kgrgame.h", are used
//       to smooth out differences between Qt 1 and Qt2 QString classes.

bool KGrGame::initCollections ()
{
    // Initialise the list of collections of levels (i.e. the list of games).
    //collections.setAutoDelete(true);
    owner = SYSTEM;				// Use system levels initially.
    if (! loadCollections (SYSTEM))		// Load system collections list.
	return (false);				// If no collections, abort.
    loadCollections (USER);			// Load user collections list.
						// If none, don't worry.

    // DISABLED xxxxx  mapCollections();	// Check ".grl" file integrity.

    // Set the default collection (first one in the SYSTEM "games.dat" file).
    collnIndex = 0;
    collection = collections.at (collnIndex);
    level = 1;					// Default start is at level 1.

    return (true);
}

void KGrGame::mapCollections()
{
    QDir		d;
    KGrCollection *	colln;
    QString		d_path;
    QString		fileName1;
    QString		fileName2;

    // Find KGoldrunner level files, sorted by name (same as numerical order).
    QListIterator<KGrCollection *> i(collections);
    while (i.hasNext()) {
	colln = i.next();
	if (colln->owner == SYSTEM) {
	    // Skip these checks now we are using KGoldrunner 3 file formats.
	    continue;
	}
	d.setPath (getDirectory (colln->owner) + "levels/");
	d_path = d.path();
	if (! d.exists()) {
	    // There is no "levels" sub-directory: OK if game has no levels yet.
	    if (colln->nLevels > 0) {
		KGrMessage::information (view, i18n("Check Games & Levels"),
			i18n("There is no folder '%1' to hold levels for"
			" the '%2' game.", d_path, colln->name));
	    }
	    continue;
	}

	const QFileInfoList files = d.entryInfoList
			(QStringList(colln->prefix + "???.grl"),
			 QDir::Files, QDir::Name);

	if ((files.count() <= 0) && (colln->nLevels > 0)) {
	    KGrMessage::information (view, i18n("Check Games & Levels"),
		i18n("There are no files '%1/%2???.grl' for the %3 game.",
		 d_path,
		 colln->prefix,
		 "\"" + colln->name + "\""));
	    continue;
	}

	// If the prefix is "level", the first file is the "ENDE" screen.
	int lev = (colln->prefix == "level") ? 0 : 1;

        foreach (const QFileInfo& file, files) {
	    // Get the name of the file found on disk.
	    fileName1 = file.fileName();

	    while (true) {
		// Work out what the file name should be, based on the level no.
		fileName2.setNum (lev);			// Convert to QString.
		fileName2 = fileName2.rightJustified (3,'0'); // Add zeros.
		fileName2.append (".grl");		// Add level-suffix.
		fileName2.prepend (colln->prefix);	// Add colln. prefix.

		if (lev > colln->nLevels) {
		    KGrMessage::information (view,
			i18n("Check Games & Levels"),
			i18n("File '%1' is beyond the highest level for "
			"the %2 game and cannot be played.",
			 fileName1,
			 "\"" + colln->name + "\""));
		    break;
		}
		else if (fileName1 == fileName2) {
		    lev++;
		    break;
		}
		else if (fileName1.myStr() < fileName2.myStr()) {
		    KGrMessage::information (view,
			i18n("Check Games & Levels"),
			i18n("File '%1' is before the lowest level for "
			"the %2 game and cannot be played.",
			 fileName1,
			 "\"" + colln->name + "\""));
		    break;
		}
		else {
		    KGrMessage::information (view,
			i18n("Check Games & Levels"),
			i18n("Cannot find file '%1' for the %2 game.",
			 fileName2,
			 "\"" + colln->name + "\""));
		    lev++;
		}
	    }
	}
    }
}

bool KGrGame::loadCollections (Owner o)
{
    KGrGameIO io;
    QList<GameData *> gameList;
    IOStatus status = io.fetchGameListData (getDirectory (o), gameList);

    bool result = false;
    switch (status) {
    case NotFound:
	// If the user has not yet created a collection, don't worry.
	if (o == SYSTEM) {
	    KGrMessage::information (view, i18n("Load Game Info"),
		i18n("Cannot find game info file '%1'.",
		    gameList.last()->filePath));
	}
	break;
    case NoRead:
    case NoWrite:
	KGrMessage::information (view, i18n("Load Game Info"),
	    i18n("Cannot open file '%1' for read-only.", 
		gameList.last()->filePath));
	break;
    case UnexpectedEOF:
	KGrMessage::information (view, i18n("Load Game Info"),
	    i18n("Reached end of file '%1' before finding end of game-data.",
		gameList.last()->filePath));
	break;
    case OK:
	foreach (GameData * g, gameList) {
	    collections.append (new KGrCollection
		    (o, i18n((const char *) g->name), // Translate now.
			g->prefix, g->rules, g->nLevels,
			QString::fromUtf8((const char *) g->about), g->skill));
	}
	result = true;
	break;
    }

    while (! gameList.isEmpty())
        delete gameList.takeFirst();
    return (result);
}

bool KGrGame::saveCollections (Owner o)
{
    QString	filePath;

    if (o != USER) {
	KGrMessage::information (view, i18n("Save Game Info"),
	    i18n("You can only modify user games."));
	return false;
    }

    filePath = userDataDir + "games.dat";

    QFile c (filePath);

    // Open the output file.
    if (! c.open (QIODevice::WriteOnly)) {
	KGrMessage::information (view, i18n("Save Game Info"),
		i18n("Cannot open file '%1' for output.", filePath));
	return (false);
    }

    // Save the collections.
    KGrCollection *	colln;
    QString		line;
    int			i, len;
    char		ch;

    QListIterator<KGrCollection *> it(collections);
    while (it.hasNext()){
	colln = it.next();
    //for (colln = collections.first(); colln != 0; colln = collections.next()) {
	if (colln->owner == o) {
	    line.sprintf ("%03d %c %s %s\n", colln->nLevels, colln->settings,
				colln->prefix.myStr(),
				colln->name.toUtf8().constData());
	    len = line.length();
	    for (i = 0; i < len; i++)
			c.putChar (line.toUtf8()[i]);

	    len = colln->about.length();
	    if (len > 0) {
		QByteArray aboutC = colln->about.toUtf8();
		len = aboutC.length();		// Might be longer now.
		for (i = 0; i < len; i++) {
		    ch = aboutC[i];
		    if (ch != '\n') {
			c.putChar (ch);		// Copy the character.
		    }
		    else {
			c.putChar ('\\');	// Change newline to \ and n.
			c.putChar ('n');
		    }
		}
		c.putChar ('\n');		// Add a real newline.
	    }
	}
    }

    c.close();
    return (true);
}

/******************************************************************************/
/**********************    WORD-WRAPPED MESSAGE BOX    ************************/
/******************************************************************************/

void KGrGame::myMessage (QWidget * parent, const QString &title, const QString &contents)
{
    // Halt the game while the message is displayed.
    setMessageFreeze (true);

    KGrMessage::wrapped (parent, title, contents);

    // Unfreeze the game, but only if it was previously unfrozen.
    setMessageFreeze (false);
}


/******************************************************************************/
/***********************    COLLECTION DATA CLASS    **************************/
/******************************************************************************/

KGrCollection::KGrCollection (Owner o, const QString & n, const QString & p,
               const char s, int nl, const QString & a, const char sk = 'N')
{
    // Holds information about a collection of KGoldrunner levels (i.e. a game).
    owner = o; name = n; prefix = p; settings = s; nLevels = nl;
    about = a; skill = sk;
}

#include "kgrgame.moc"
// vi: set sw=4 :
