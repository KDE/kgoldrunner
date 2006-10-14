/***************************************************************************
                          kgrcanvas.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    copyright            : (C) 2002 by Marco Krüger and Ian Wadham
    email                : See menu "Help, About KGoldrunner"
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

class KGoldrunner;

KGrCanvas::KGrCanvas (QWidget * parent, const char *name)
	: QCanvasView (0, parent, name)
{
    setBackgroundMode (NoBackground);
    m = new QCursor ();		// For handling the mouse.

    scaleStep = STEP;		// Initial scale is 1:1.
    baseScale = scaleStep;
    baseFontSize = fontInfo().pointSize();

    border = 4;			// Allow 2 tile-widths on each side for border.
    cw = 4*STEP;		// Playfield cell width (= four steps).
    bw = border*cw/2;		// Total border width (= two cells).
    lw = cw/8;			// Line width (for edge of border).
    mw = bw - lw;		// Border main-part width.

    initView();			// Set up the graphics, etc.
}

KGrCanvas::~KGrCanvas()
{
}

void KGrCanvas::changeLandscape (const QString & name)
{
    for (int i = 0; strcmp (colourScheme [i], "") != 0; i++) {
	if (colourScheme [i] == name) {

	    // Change XPM colours and re-draw the tile-pictures used by QCanvas.
	    changeColours (& colourScheme [i]);
	    makeTiles();

	    // Set all cells to same tile-numbers as before, but new colours.
	    int tileNo [FIELDWIDTH] [FIELDHEIGHT];
	    int offset = border / 2;

	    for (int x = 0; x < FIELDWIDTH; x++) {
		for (int y = 0; y < FIELDHEIGHT; y++) {
		    tileNo[x][y] = field->tile (x + offset, y + offset);
		}
	    }

	    field->setTiles (bgPix, (FIELDWIDTH+border), (FIELDHEIGHT+border),
			bgw, bgh);		// Sets all tile-numbers to 0.

	    for (int x = 0; x < FIELDWIDTH; x++) {
		for (int y = 0; y < FIELDHEIGHT; y++) {
		    field->setTile (x + offset, y + offset, tileNo[x][y]);
		}
	    }

	    borderB->setBrush (QBrush (borderColor));
	    borderL->setBrush (QBrush (borderColor));
	    borderR->setBrush (QBrush (borderColor));

	    QString t = title->text();
	    makeTitle ();
	    setTitle (t);

	    // Repaint the playing area.
	    updateCanvas();
	    return;
	}
    }
}

bool KGrCanvas::changeSize (int d)
{
#ifdef QT3
    if ((d < 0) && (scaleStep <= STEP)) {
	// Note: Smaller scales lose detail (e.g. the joints in brickwork).
	KGrMessage::information (this, i18n("Change Size"),
	i18n("Sorry, you cannot make the play area any smaller."));
	return FALSE;
    }

    if ((d >= 0) && (scaleStep >= 2 * STEP)) {
	// Note: Larger scales go off the edge of the monitor.
	KGrMessage::information (this, i18n("Change Size"),
	i18n("Sorry, you cannot make the play area any larger."));
	return FALSE;
    }

    QWMatrix wm = worldMatrix();
    double   wmScale = 1.0;

    // Set the scale back to 1:1 and calculate the new scale factor.
    wm.reset();
    scaleStep = (d < 0) ? (scaleStep - 1) : (scaleStep + 1);

    // If scale > 1:1, scale up to the new factor (e.g. 1.25:1, 1.5:1, etc.)
    if (scaleStep > STEP) {
	wmScale = (wmScale * scaleStep) / STEP;
	wm.scale (wmScale, wmScale);
    }
    setWorldMatrix (wm);

    // Force the title size and position to be re-calculated.
    QString t = title->text();
    makeTitle ();
    setTitle (t);

    // Fit the QCanvasView and its frame to the canvas.
    int frame = frameWidth()*2;
    setFixedSize ((FIELDWIDTH  + 4) * 4 * scaleStep + frame,
		  (FIELDHEIGHT + 4) * 4 * scaleStep + frame);
    return TRUE;

#else
    KGrMessage::information (this, i18n( "Change Size" ),
    i18n( "Sorry, you cannot change the size of the playing area. "
    "That function requires Qt Library version 3 or later." ));
    return FALSE;
#endif
}

void KGrCanvas::updateCanvas()
{
    field->update();
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
    field->setTile (x, y, tileNumber);	// Paint cell with required pixmap.
}

void KGrCanvas::setBaseScale ()
{
    // Synchronise the desktop font size with the initial canvas scale.
    baseScale = scaleStep;
    QString t = title->text();
    makeTitle ();
    setTitle (t);
}

void KGrCanvas::setTitle (QString newTitle)
{
    title->setText (newTitle);
}

void KGrCanvas::makeTitle ()
{
    // This uses a calculated QLabel and QFont size because a QCanvasText
    // object does not always display scaled-up fonts cleanly (in Qt 3.1.1).

    if (title != 0)
	title->close (TRUE);		// Close and delete previous title.

    title = new QLabel ("", this);
    title->setFixedWidth (((FIELDWIDTH * cw + 2 * bw) * scaleStep) / STEP);
    title->setFixedHeight ((mw * scaleStep) / STEP);
    title->move (0, 0);
    title->setPaletteBackgroundColor (borderColor);
    title->setPaletteForegroundColor (textColor);
    title->setFont (QFont (fontInfo().family(),
		 (baseFontSize * scaleStep) / baseScale, QFont::Bold));
    title->setAlignment (Qt::AlignCenter);
    title->raise();
    title->show();
}

void KGrCanvas::contentsMousePressEvent (QMouseEvent * m) {
    emit mouseClick (m->button ());
}

void KGrCanvas::contentsMouseReleaseEvent (QMouseEvent * m) {
    emit mouseLetGo (m->button ());
}

QPoint KGrCanvas::getMousePos ()
{
    int i, j;
    int fw = frameWidth();
    int cell = 4 * scaleStep;
    QPoint p = mapFromGlobal (m->pos());

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i = ((p.x() - fw) / cell) - 1; j = ((p.y() - fw) / cell) - 1;

    return (QPoint (i, j));
}

void KGrCanvas::setMousePos (int i, int j)
{
    int fw = frameWidth();
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
    heroSprite = new QCanvasSprite (heroArray, field);

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    i++; j++;
    heroSprite->move (i * 4 * STEP, j * 4 * STEP, startFrame);
    heroSprite->setZ (1);
    heroSprite->setVisible (TRUE);
}

void KGrCanvas::setHeroVisible (bool newState)
{
    heroSprite->setVisible (newState);		// Show or hide the hero.
}

void KGrCanvas::makeEnemySprite (int i, int j, int startFrame)
{
    QCanvasSprite * enemySprite = new QCanvasSprite (enemyArray, field);

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
    updateCanvas();
}

void KGrCanvas::moveEnemy (int id, int x, int y, int frame, int nuggets)
{
    if (nuggets != 0) {				// If enemy is carrying gold,
	frame = frame + goldEnemy;		// show him with gold outline.
    }

    // In KGoldrunner, the top-left visible cell is [1,1] --- in QCanvas [2,2].
    enemySprites->at(id)->move (x + 4 * STEP, y + 4 * STEP, frame);
    updateCanvas();
}

void KGrCanvas::deleteEnemySprites()
{
    enemySprites->clear();
}

QPixmap KGrCanvas::getPixmap (char type)
{
    QPixmap pic (bgw, bgh, bgd);
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
    p.drawPixmap (0, 0, bgPix, tileNumber * bgw, 0, bgw, bgh);
    p.end();

    return (pic);
}

void KGrCanvas::initView()
{
    changeColours (& colourScheme [0]);		// Set "KGoldrunner" landscape.

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

    QPixmap pixmap;
    QImage image;

    pixmap = QPixmap (hgbrick_xpm);

    bgw = pixmap.width();	// Save dimensions for "getPixmap".
    bgh = pixmap.height();
    bgd = pixmap.depth();

    // Assemble the background and editing pixmaps into a strip (18 pixmaps).
    bgPix	= QPixmap ((brickbg + 10) * bgw, bgh, bgd);

    makeTiles();		// Fill the strip with 18 tiles.

    // Define the canvas as an array of tiles.  Default tile is 0 (free space).
    int frame = frameWidth()*2;
    field = new QCanvas ((FIELDWIDTH+border) * bgw, (FIELDHEIGHT+border) * bgh);
    field->setTiles (bgPix, (FIELDWIDTH+border), (FIELDHEIGHT+border),
			bgw, bgh);

    // Embed the canvas in the view and make it occupy the whole of the view.
    setCanvas (field);
    setVScrollBarMode (QScrollView::AlwaysOff);
    setHScrollBarMode (QScrollView::AlwaysOff);
    setFixedSize (field->width() + frame, field->height() + frame);

    //////////////////////////////////////////////////////////////////////////
    // The pixmaps for hero and enemies are arranged in strips of 20: walk  //
    // right (4), walk left (4), climb right along bar (4), climb left (4), //
    // climb up ladder (2) and fall (2) --- total 20.                       //
    //////////////////////////////////////////////////////////////////////////

    // Convert the pixmap strip for hero animation into a QCanvasPixmapArray.
    pixmap = QPixmap (hero_xpm);
    image = pixmap.convertToImage ();

#ifdef QT3
    QPixmap   pm;
    QValueList<QPixmap> pmList;

    for (int i = 0; i < 20; i++) {
	pm.convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
    }

    heroArray = new QCanvasPixmapArray (pmList);	// Hot spots all (0,0).
#else
    QPixmap * pm;
    QPoint  * pt;
    QList<QPixmap> pmList;
    QList<QPoint>  ptList;

    pt = new QPoint (0, 0);		// "Hot spot" not used in KGoldrunner.

    for (int i = 0; i < 20; i++) {
	pm = new QPixmap ();
	pm->convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
	ptList.append (pt);
    }

    heroArray = new QCanvasPixmapArray (pmList, ptList);
#endif

    // Convert pixmap strips for enemy animations into a QCanvasPixmapArray.
    // First convert the pixmaps for enemies with no gold ...
    pixmap = QPixmap (enemy1_xpm);
    image = pixmap.convertToImage ();

    pmList.clear();

#ifdef QT3
    for (int i = 0; i < 20; i++) {
	pm.convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
    }
#else
    ptList.clear();

    for (int i = 0; i < 20; i++) {
	pm = new QPixmap ();
	pm->convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
	ptList.append (pt);
    }
#endif

    // ... then convert the gold-carrying enemies.
    pixmap = QPixmap (enemy2_xpm);
    image = pixmap.convertToImage ();

#ifdef QT3
    for (int i = 0; i < 20; i++) {
	pm.convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
    }

    enemyArray = new QCanvasPixmapArray (pmList);	// Hot spots all (0,0).
#else
    for (int i = 0; i < 20; i++) {
	pm = new QPixmap ();
	pm->convertFromImage (image.copy (i * 16, 0, 16, 16));
	pmList.append (pm);
	ptList.append (pt);
    }

    enemyArray = new QCanvasPixmapArray (pmList, ptList);
#endif

    goldEnemy = 20;			// Offset of gold-carrying frames.

    // Draw the border around the playing area (z = 0).
    makeBorder();

    // Create a title item, in off-white colour, on top of the border.
    title = 0;
    makeTitle();

    // Create an empty list of enemy sprites.
#ifdef QT3
    enemySprites = new QPtrList<QCanvasSprite> ();
#else
    enemySprites = new QList<QCanvasSprite> ();
#endif
    enemySprites->setAutoDelete(TRUE);
}

void KGrCanvas::makeTiles ()
{
    QPainter p (& bgPix);

    // First draw the single pixmaps (8 tiles) ...
    p.drawPixmap (freebg    * bgw, 0, QPixmap (hgbrick_xpm));	// Free space.
    p.drawPixmap (nuggetbg  * bgw, 0, QPixmap (nugget_xpm));	// Nugget.
    p.drawPixmap (polebg    * bgw, 0, QPixmap (pole_xpm));	// Pole or bar.
    p.drawPixmap (ladderbg  * bgw, 0, QPixmap (ladder_xpm));	// Ladder.
    p.drawPixmap (hladderbg * bgw, 0, QPixmap (hladder_xpm));	// Hidden laddr.
    p.drawPixmap (edherobg  * bgw, 0, QPixmap (edithero_xpm));	// Static hero.
    p.drawPixmap (edenemybg * bgw, 0, QPixmap (editenemy_xpm));	// Static enemy.
    p.drawPixmap (betonbg   * bgw, 0, QPixmap (beton_xpm));	// Concrete.

    // ... then add the 10 brick pixmaps.
    p.drawPixmap (brickbg   * bgw, 0, QPixmap (bricks_xpm));	// Bricks.

    p.end();
}

void KGrCanvas::makeBorder ()
{
    // Draw main part of border, in the order: top, bottom, left, right.
    // Allow some overlap to prevent slits appearing when using "changeSize".
    colour = borderColor;

    // The first rectangle is actually a QLabel drawn by "makeTitle()".
    // borderT = drawRectangle (11, 0, 0, FIELDWIDTH*cw + 2*bw, mw);
    borderB = drawRectangle (11, 0, FIELDHEIGHT*cw + bw + lw,
						FIELDWIDTH*cw + 2*bw, mw);
    borderL = drawRectangle (12, 0, bw - lw - 1, mw, FIELDHEIGHT*cw + 2*lw + 2);
    borderR = drawRectangle (12, FIELDWIDTH*cw + bw + lw, bw - lw - 1,
						mw, FIELDHEIGHT*cw + 2*lw + 2);

    // Draw inside edges of border, in the same way.
    colour = QColor (black);
    drawRectangle (10, bw-lw, bw-lw-1, FIELDWIDTH*cw + 2*lw, lw+1);
    drawRectangle (10, bw-lw, FIELDHEIGHT*cw + bw, FIELDWIDTH*cw + 2*lw, lw+1);
    drawRectangle (10, bw - lw, bw, lw, FIELDHEIGHT*cw);
    drawRectangle (10, FIELDWIDTH*cw + bw, bw, lw, FIELDHEIGHT*cw);
}

QCanvasRectangle * KGrCanvas::drawRectangle (int z, int x, int y, int w, int h)
{
    QCanvasRectangle * r = new QCanvasRectangle (x, y, w, h, field);

    r->setBrush (QBrush (colour));
    r->setPen (QPen (NoPen));
    r->setZ (z);
    r->show();

    return (r);
}

void KGrCanvas::changeColours (const char * colours [])
{
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
