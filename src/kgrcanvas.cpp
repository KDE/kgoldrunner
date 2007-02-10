/***************************************************************************
                          kgrcanvas.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krger
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifdef KGR_PORTABLE
// If compiling for portability, redefine KDE's i18n.
#define i18n tr
#endif

#include "kgrconsts.h"

#include "kgrdialog.h"
#include "kgrcanvas.h"
#include "kgrgame.h"

// Graphics files for moving figures and background.
#include "hero.xpm"
#include "enemy1.xpm"
#include "enemy2.xpm"
#include "kgraphics.h"

#include <QPixmap>
#include <QList>
#include <QLabel>
#include <QList>
#include <QMouseEvent>

#include <KSvgRenderer>

// class KGoldrunner;

KGrCanvas::KGrCanvas (QWidget * parent, const double scale,
			const QString & systemDataDir)
			: KGameCanvasWidget (parent)
{
    //setBackgroundMode (Qt::NoBackground);
    m = new QCursor ();		// For handling the mouse.

    scaleStep = STEP;		// Initial scale is 1:1.
    baseScale = scaleStep;
    baseFontSize = fontInfo().pointSize() + 2;
    scaleStep = (int) ((scale * STEP) + 0.05);
    qDebug() << "Scale" << scale << "Scaled Step" << scaleStep;

    border = 4;			// Allow 2 tile-widths on each side for border.
    cw = 4*STEP;		// Playfield cell width (= four steps).
    bw = border*cw/2;		// Total border width (= two cells).
    lw = cw/8;			// Line width (for edge of border).
    mw = bw - lw;		// Border main-part width.

    heroSprite = 0;
    enemySprites = 0;

    svg = 0;
    picsDataDir = systemDataDir + "../pics/";
    filePrefixSVG = "";
    initView();			// Set up the graphics, etc.
}

KGrCanvas::~KGrCanvas()
{
}

void KGrCanvas::changeLandscape (const QString & name)
{
    qDebug() << "Looking for landscape:" << name;
    for (int i = 0; strcmp (colourScheme [i], "") != 0; i++) {
	if (colourScheme [i] == name) {

	    qDebug() << "Found it: going to change colours.";
	    // Change XPM colours and re-draw the tile-pictures used by QCanvas.
	    changeColours (& colourScheme [i]);

	    // Set all cells to same tile-numbers as before, but new colours.
	    int tileNo [FIELDWIDTH] [FIELDHEIGHT];
	    int offset = border / 2;

	    for (int x = 0; x < FIELDWIDTH; x++) {
		for (int y = 0; y < FIELDHEIGHT; y++) {
		    tileNo[x][y] = playfield->tile (x + offset, y + offset);
		}
	    }

	    qDebug() << "Going to make tiles.";
	    makeTiles();
	    qDebug() << "Made tiles OK.";

	    for (int x = 0; x < FIELDWIDTH; x++) {
		for (int y = 0; y < FIELDHEIGHT; y++) {
		    playfield->setTile (x + offset, y + offset, tileNo[x][y]);
		}
	    }

	    makeBorder();

	    QString t = title->text();
	    makeTitle ();
	    setTitle (t);

	    return;
	}
    }
}

bool KGrCanvas::changeSize (int d)
{
    if ((d < 0) && (scaleStep <= STEP)) {
	// Note: Smaller scales lose detail (e.g. the joints in brickwork).
	KGrMessage::information (this, i18n("Change Size"),
	i18n("Sorry, you cannot make the play area any smaller."));
	return false;
    }

    if ((d >= 0) && (scaleStep >= 2 * STEP)) {
	// Note: Larger scales go off the edge of the monitor.
	KGrMessage::information (this, i18n("Change Size"),
	i18n("Sorry, you cannot make the play area any larger."));
	return false;
    }

    double   wmScale = 1.0;
    scaleStep = (d < 0) ? (scaleStep - 1) : (scaleStep + 1);
    wmScale = (wmScale * scaleStep) / STEP;

    if (playfield) {
	//Scale background
	// Set all cells to same tile-numbers as before.
	 int tileNo [FIELDWIDTH] [FIELDHEIGHT];
	 int offset = border / 2;

	for (int x = 0; x < FIELDWIDTH; x++) {
	    for (int y = 0; y < FIELDHEIGHT; y++) {
		tileNo[x][y] = playfield->tile (x + offset, y + offset);
	    }
	}

	imgW = (bgw * scaleStep) / STEP;
	imgH = (bgh * scaleStep) / STEP;

	qDebug() << "Going to make tiles: resize.";
	makeTiles();
	qDebug() << "Made tiles OK.";

	for (int x = 0; x < FIELDWIDTH; x++) {
	    for (int y = 0; y < FIELDHEIGHT; y++) {
		playfield->setTile (x + offset, y + offset, tileNo[x][y]);
	    }
	}
    }

   int spriteframe;
   QPoint spriteloc;

    //hero
    if (heroSprite) {
    	spriteframe = heroSprite->currentFrame();
    	spriteloc = heroSprite->currentLoc();
    	heroSprite->clearFrames();
    	heroSprite->addFrames(QPixmap (hero_xpm), 16, 16, 20, wmScale );
	// Force re-draw of both pixmap and position at the new scale.
    	heroSprite->move (0, 0, (spriteframe > 0) ? 0 : 1);
    	heroSprite->move (spriteloc.x(), spriteloc.y(), spriteframe);
    }

    //enemies
    if (enemySprites)
    {
	for (int i = 0; i < enemySprites->size(); ++i) {
    	    KGrSprite * thisenemy = enemySprites->at(i);
	    if (thisenemy) {
		spriteframe = thisenemy->currentFrame();
    		spriteloc = thisenemy->currentLoc();
    		thisenemy->clearFrames();
    		thisenemy->addFrames(QPixmap (enemy1_xpm), 16, 16, 20, wmScale);
		// Now adds the frames for enemies with no gold ...
    		thisenemy->addFrames(QPixmap (enemy2_xpm), 16, 16, 20, wmScale);
		// Force re-draw of both pixmap and position at the new scale.
		thisenemy->move (0, 0, (spriteframe > 0) ? 0 : 1);
    		thisenemy->move (spriteloc.x(), spriteloc.y(), spriteframe);
	    }
	}
    }

    //recreate border
    makeBorder ();
    // Force the title size and position to be re-calculated.
    QString t = title->text();
    makeTitle ();
    setTitle (t);


    // Fit the QCanvasView and its frame to the canvas.
    int frame = 0;//frameWidth()*2;
    setFixedSize ((FIELDWIDTH  + 4) * 4 * scaleStep + frame, (FIELDHEIGHT + 4) * 4 * scaleStep + frame);
    return true;

}

void KGrCanvas::paintCell (int x, int y, char type, int offset)
{
    int tileNumber = 0;

    switch (type) {
    case FREE:    tileNumber = freebg; break;	// Free space.
    case NUGGET:  tileNumber = nuggetbg; break;	// Nugget.
    case POLE:    tileNumber = polebg; break;	// Pole or bar.
    case LADDER:  tileNumber = ladderbg; break;	// Ladder.
    case HLADDER: tileNumber = hladderbg; break;// Hidden ladder (for editing).
    case HERO:    tileNumber = edherobg; break;	// Static hero (for editing).
    case ENEMY:   tileNumber = edenemybg; break;// Static enemy (for editing).
    case BETON:   tileNumber = betonbg;	 break;	// Concrete.
    case BRICK:   tileNumber = brickbg;	 break;	// Solid brick.
    case FBRICK:  tileNumber = fbrickbg; break;	// False brick (for editing).
    default:      tileNumber = freebg;  break;
    }

    tileNumber = tileNumber + offset;	// Offsets 1-9 are for digging sequence.

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    x++; y++;
    playfield->setTile (x, y, tileNumber);	// Paint cell with required pixmap.
}

void KGrCanvas::setBaseScale ()
{
    // Synchronise the desktop font size with the initial canvas scale.
    baseScale = scaleStep;
    QString t = title->text();
    makeTitle ();
    setTitle (t);
}

void KGrCanvas::setTitle (const QString &newTitle)
{
    title->setText (newTitle);
}

void KGrCanvas::makeTitle ()
{
    // This uses a calculated QLabel and QFont size because a QCanvasText
    // object does not always display scaled-up fonts cleanly (in Qt 3.1.1).

    if (title != 0)
	title->close (true);		// Close and delete previous title.

    title = new QLabel ("", this);
    title->setFixedWidth (((FIELDWIDTH * cw + 2 * bw) * scaleStep) / STEP);
    title->setFixedHeight ((mw * scaleStep) / STEP);
    title->move (0, 0);
    QPalette palette;
    palette.setColor(title->backgroundRole(), borderColor);
    palette.setColor(title->foregroundRole(), textColor);
    title->setPalette(palette);
    title->setFont (QFont (fontInfo().family(),
		 (baseFontSize * scaleStep) / baseScale, QFont::Bold));
    title->setAlignment (Qt::AlignCenter);
    title->setAttribute(Qt::WA_QuitOnClose, false); //Otherwise the close above might exit app
    title->raise();
    title->show();
}

void KGrCanvas::mousePressEvent ( QMouseEvent * mouseEvent )
{
    emit mouseClick(mouseEvent->button());
}

void KGrCanvas::mouseReleaseEvent ( QMouseEvent * mouseEvent )
{
    emit mouseLetGo(mouseEvent->button());
}

QPoint KGrCanvas::getMousePos ()
{
    int i, j;
    int fw = 0;//frameWidth();
    int cell = 4 * scaleStep;
    QPoint p = mapFromGlobal (m->pos());

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i = ((p.x() - fw) / cell) - 1; j = ((p.y() - fw) / cell) - 1;

    return (QPoint (i, j));
}

void KGrCanvas::setMousePos (int i, int j)
{
    int fw = 0;//frameWidth();
    int cell = 4 * scaleStep;

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i++; j++;
    //m->setPos (mapToGlobal (QPoint (i * 4 * STEP + 8, j * 4 * STEP + 8)));
    //m->setPos (mapToGlobal (QPoint (i * 5 * STEP + 10, j * 5 * STEP + 10)));
    m->setPos (mapToGlobal (
		QPoint (i * cell + fw + cell / 2, j * cell + fw + cell / 2)));
}

void KGrCanvas::makeHeroSprite (int i, int j, int startFrame)
{
    heroSprite = new KGrSprite (this);

    // Process the pixmap with the hero frames
    //////////////////////////////////////////////////////////////////////////
    // The pixmaps for hero and enemies are arranged in strips of 20: walk  //
    // right (4), walk left (4), climb right along bar (4), climb left (4), //
    // climb up ladder (2) and fall (2) --- total 20.                       //
    //////////////////////////////////////////////////////////////////////////
    // Our KGrSprite class will extract the frames from the strip (16x16 pix, 20 frames)
    heroSprite->addFrames(QPixmap (hero_xpm), 16, 16, 20, (1.0 * scaleStep) / STEP );

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i++; j++;
    heroSprite->move (i * 4 * STEP, j * 4 * STEP, startFrame);
    heroSprite->setZ (1);
    heroSprite->setVisible (true);
}

void KGrCanvas::setHeroVisible (bool newState)
{
    heroSprite->setVisible (newState);		// Show or hide the hero.
}

void KGrCanvas::makeEnemySprite (int i, int j, int startFrame)
{
    KGrSprite * enemySprite = new KGrSprite (this);

    //////////////////////////////////////////////////////////////////////////
    // The pixmaps for hero and enemies are arranged in strips of 20: walk  //
    // right (4), walk left (4), climb right along bar (4), climb left (4), //
    // climb up ladder (2) and fall (2) --- total 20.                       //
    //////////////////////////////////////////////////////////////////////////
    // Our KGrSprite class will extract the frames from the strip (16x16 pix, 20 frames)
    // First convert the pixmap for enemies with no gold ...
    enemySprite->addFrames(QPixmap (enemy1_xpm), 16, 16, 20, (1.0 * scaleStep) / STEP );

    // Now adds the frames for enemies with no gold ...
    enemySprite->addFrames(QPixmap (enemy2_xpm), 16, 16, 20, (1.0 * scaleStep) / STEP );

    enemySprites->append (enemySprite);

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i++; j++;
    enemySprite->move (i * 4 * STEP, j * 4 * STEP, startFrame);
    enemySprite->setZ (2);
    enemySprite->show();
}

void KGrCanvas::moveHero (int x, int y, int frame)
{
    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    heroSprite->move (x + 4 * STEP, y + 4 * STEP, frame);
}

void KGrCanvas::moveEnemy (int id, int x, int y, int frame, int nuggets)
{
    if (nuggets != 0) {				// If enemy is carrying gold,
	frame = frame + goldEnemy;		// show him with gold outline.
    }

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    enemySprites->at(id)->move (x + 4 * STEP, y + 4 * STEP, frame);
}

void KGrCanvas::deleteEnemySprites()
{
    while (!enemySprites->isEmpty())
            delete enemySprites->takeFirst();
}

QPixmap KGrCanvas::getPixmap (char type)
{
    QImage pic (bgw, bgh, QImage::Format_ARGB32_Premultiplied);
    QPainter p (& pic);
    int tileNumber;

    // Get a pixmap from the tile-array for use on an edit-button.
    switch (type) {
    case FREE:    tileNumber = freebg; break;	// Free space.
    case NUGGET:  tileNumber = nuggetbg; break;	// Nugget.
    case POLE:    tileNumber = polebg; break;	// Pole or bar.
    case LADDER:  tileNumber = ladderbg; break;	// Ladder.
    case HLADDER: tileNumber = hladderbg; break;// Hidden ladder (for editing).
    case HERO:    tileNumber = edherobg; break;	// Static hero (for editing).
    case ENEMY:   tileNumber = edenemybg; break;// Static enemy (for editing).
    case BETON:   tileNumber = betonbg;	 break;	// Concrete.
    case BRICK:   tileNumber = brickbg;	 break;	// Solid brick.
    case FBRICK:  tileNumber = fbrickbg; break;	// False brick (for editing).
    default:      tileNumber = freebg;  break;
    }

    // Copy a tile of width bgw and height bgh from the tile-array.
    p.drawImage (0, 0, bgPix, tileNumber * bgw, 0, bgw, bgh);
    p.end();

    return QPixmap::fromImage (pic);
}

void KGrCanvas::initView()
{
    changeColours (& colourScheme [0]);		// Set "KGoldrunner" colours.

    // TODO: Set up (properly) a default SVG theme OR one from user's KConfig.
    filePrefixSVG = "kgr_1";			// FORCE SVG (TESTING) ...

    // Set up the pixmaps for the editable objects.
    freebg	= 0;		// Free space.
    nuggetbg	= 1;		// Nugget.
    polebg	= 2;		// Pole or bar.
    ladderbg	= 3;		// Ladder.
    hladderbg	= 4;		// Hidden ladder (for editing).
    edherobg	= 5;		// Static hero (for editing).
    edenemybg	= 6;		// Static enemy (for editing).
    betonbg	= 7;		// Concrete.

    // The bricks have 10 pixmaps (showing various stages of digging).
    brickbg	= 8;		// Solid brick - 1st pixmap.
    fbrickbg	= 15;		// False brick - 8th pixmap (for editing).

    QPixmap pixmap = QPixmap (hgbrick_xpm);

    bgw = pixmap.width();	// Save dimensions for "getPixmap".
    bgh = pixmap.height();

    // Assemble the background and editing pixmaps into a strip (18 pixmaps).
    imgW = (bgw * scaleStep) / STEP;
    imgH = (bgh * scaleStep) / STEP;
    bgPix    = QImage ((brickbg + 10) * imgW, imgH,
			QImage::Format_ARGB32_Premultiplied);

    // Define the canvas as an array of tiles.  Default tile is 0 (free space).
    playfield = new KGrPlayField (this);

    makeTiles();		// Fill the strip with 18 tiles.

    setFixedSize (((FIELDWIDTH+border)  * bgw * scaleStep) / STEP,
		  ((FIELDHEIGHT+border) * bgh * scaleStep) / STEP);

    goldEnemy = 20;		// Offset of gold-carrying frames.

    // Draw the border around the playing area (z = 0).
    makeBorder();

    // Create a title item, in off-white colour, on top of the border.
    title = 0;
    makeTitle();

    // Create an empty list of enemy sprites.
    enemySprites = new QList<KGrSprite *> ();
}

void KGrCanvas::makeTiles ()
{
    qDebug() << "filePrefixSVG:" << filePrefixSVG;
    if (!filePrefixSVG.isEmpty()) {
	QString filePath = picsDataDir + filePrefixSVG + ".svg";
	// filePath = "/home/ianw/kgr.svg";	// TESTING ...
	qDebug() << "Opening SVG renderer, file:" << filePath;
	svg = new KSvgRenderer (filePath);

	// TODO: Remove this hack when brick-blaster is implemented.
	QString filePrefixSVGsave = filePrefixSVG;
	for (int i = 0; strcmp (colourScheme [i], "") != 0; i++) {
	    if (!strcmp(colourScheme [i], "Midnight")) {
		// HACK (till brick-blaster in): makes dug bricks dark in SVG.
		changeColours (& colourScheme [i]);
	    }
	}
	filePrefixSVG = filePrefixSVGsave;
    }

    bgPix.fill (0);
    qDebug() << "Scaling bgpix:" << scaleStep << imgW << imgH;
    bgPix = bgPix.scaledToHeight (imgH);
    qDebug() << "Scaling OK." << bgPix.size();

    QPainter p (& bgPix);

    // Draw the non-SVG tiles (for Editor): free, hidden ladder, hero and enemy.
    p.drawImage (freebg    * imgW, 0, QImage(hgbrick_xpm).scaledToHeight(imgH));
    p.drawImage (hladderbg * imgW, 0, QImage(hladder_xpm).scaledToHeight(imgH));
    p.drawImage (edherobg * imgW, 0, QImage(edithero_xpm).scaledToHeight(imgH));
    p.drawImage (edenemybg*imgW, 0, QImage(editenemy_xpm).scaledToHeight(imgH));

    // ... then add the 10 brick and digging pixmaps.
    p.drawImage (brickbg   * imgW, 0, QImage(bricks_xpm).scaledToHeight(imgH));

    // Make an empty background image.
    QImage * background = 0;

    qDebug() << "Is it SVG?";
    if ((! svg) || (! svg->isValid())) {
	qDebug() << "No.";
	// If not in SVG mode, draw nugget, bar, ladder and concrete.
	p.drawImage (nuggetbg*imgW, 0, QImage(nugget_xpm).scaledToHeight(imgH));
	p.drawImage (polebg  *imgW, 0, QImage(pole_xpm).scaledToHeight(imgH));
	p.drawImage (ladderbg*imgW, 0, QImage(ladder_xpm).scaledToHeight(imgH));
	p.drawImage (betonbg *imgW, 0, QImage(beton_xpm).scaledToHeight(imgH));
    }
    else {
	// Else, draw SVG background and tiles.
	qDebug() << "Yes.";
	qDebug () << "Load SVG backdrop ...";
	background = new QImage ((FIELDWIDTH+border) * imgW,
				(FIELDHEIGHT+border) * imgH,
				QImage::Format_ARGB32_Premultiplied);
	background->fill (0);
	QPainter b (background);
	svg->render (&b, "background");
	qDebug () << "SVG backdrop OK ...";

	// Draw SVG versions of nugget, bar, ladder, concrete and brick.
	QImage img (imgW, imgH, QImage::Format_ARGB32_Premultiplied);
	QPainter q (&img);
	img.fill (0);
	svg->render (&q, "gold");
	p.drawImage (nuggetbg * imgW,  0, img);
	img.fill (0);
	svg->render (&q, "bar");
	p.drawImage (polebg * imgW,  0, img);
	img.fill (0);
	svg->render (&q, "ladder");
	p.drawImage (ladderbg * imgW,  0, img);
	img.fill (0);
	svg->render (&q, "concrete");
	p.drawImage (betonbg * imgW, 0, img);
	img.fill (0);
	svg->render (&q, "brick");
	p.drawImage (brickbg * imgW, 0, img);
    }

    //Now set our tileset in the scene
    playfield->setTiles (background, bgPix,
		(FIELDWIDTH+border), (FIELDHEIGHT+border), imgW, imgH);

    delete svg;			// Finished with SVG rendering.
    svg = 0;
    delete background;
}

void KGrCanvas::makeBorder ()
{
    // Draw main part of border, in the order: top, bottom, left, right.
    // Allow some overlap to prevent slits appearing when using "changeSize".
    colour = borderColor;

    while (!borderRectangles.isEmpty())
            delete borderRectangles.takeFirst();

    KGameCanvasRectangle * nextRectangle;

    // IF SVG, NO COLOURED BORDER: background fills canvas now.
    if (filePrefixSVG.isEmpty()) {
	nextRectangle = drawRectangle (11, 0, 0, FIELDWIDTH*cw + 2*bw, mw);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (11, 0, FIELDHEIGHT*cw + bw + lw,
						FIELDWIDTH*cw + 2*bw, mw);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (12, 0, bw - lw - 1,
						mw, FIELDHEIGHT*cw + 2*lw + 2);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (12, FIELDWIDTH*cw + bw + lw, bw - lw - 1,
						mw, FIELDHEIGHT*cw + 2*lw + 3);
	borderRectangles.append(nextRectangle);
    }

    // Draw inside edges of border, in the same way.
    colour = QColor (Qt::black);

    nextRectangle = drawRectangle (10, bw-lw, bw-lw, FIELDWIDTH*cw + 2*lw, lw+1);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (10, bw-lw, FIELDHEIGHT*cw + bw, FIELDWIDTH*cw + 2*lw, lw+1);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (10, bw - lw, bw, lw, FIELDHEIGHT*cw);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (10, FIELDWIDTH*cw + bw, bw, lw, FIELDHEIGHT*cw);
    borderRectangles.append(nextRectangle);
}

KGameCanvasRectangle * KGrCanvas::drawRectangle
			(int /* z (unused) */, int x, int y, int w, int h)
{
   double wmScale = (1.0 * scaleStep) / STEP;
   KGameCanvasRectangle * r = new KGameCanvasRectangle (colour,
			QSize((int)(w * wmScale), (int)(h * wmScale)), this);


    r->moveTo ((int)(x * wmScale), (int)(y * wmScale));
    r->show();

    return (r);
}

void KGrCanvas::changeColours (const char * colours [])
{
    if (strcmp (colours [1], "SVG") == 0) {
	filePrefixSVG = colours [2];	// KGoldrunner 3 landscape, SVG-based.
	qDebug() << "SVG landscape: looking for files '" + filePrefixSVG +".*'";
	return;
    }

    filePrefixSVG = "";			// KGoldrunner 2 landscape, xpm-based.
    recolourObject (hgbrick_xpm,   colours);
    recolourObject (nugget_xpm,    colours);
    recolourObject (pole_xpm,      colours);
    recolourObject (ladder_xpm,    colours);
    recolourObject (hladder_xpm,   colours);
    recolourObject (edithero_xpm,  colours);
    recolourObject (edithero_xpm,  colours);
    recolourObject (editenemy_xpm, colours);
    recolourObject (beton_xpm,     colours);
    recolourObject (bricks_xpm,    colours);

    borderColor = QColor (colours [1]);
    textColor =   QColor (colours [2]);

    KGrThumbNail::backgroundColor = QColor (QString(colours [3]).right(7));
    KGrThumbNail::brickColor =      QColor (QString(colours [6]).right(7));
    KGrThumbNail::ladderColor =     QColor (QString(colours [9]).right(7));
    KGrThumbNail::poleColor =       QColor (QString(colours [11]).right(7));
}

void KGrCanvas::recolourObject (const char * object [], const char * colours [])
{
    int i;
    for (i = 0; i < 9; i++) {
	object [i+1] = colours [i+3];
    }
}

#include "kgrcanvas.moc"
