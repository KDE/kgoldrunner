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

#include "kgrcanvas.h"

// Graphics files for moving figures and background.
#include "hero.xpm"
#include "enemy1.xpm"
#include "enemy2.xpm"
#include "kgraphics.h"

#include <QPixmap>
#include <QList>
#include <QLabel>
#include <QMouseEvent>

#include <QDebug>

#include <KConfig>

KGrCanvas::KGrCanvas (QWidget * parent, const double scale,
			const QString & systemDataDir)
			: KGameCanvasWidget (parent),
			  topLeft (0, 0), bgw (4 * STEP), bgh (4 * STEP)
{
    resizeCount = 0;		// IDW

    qDebug() << "Called KGrCanvas::KGrCanvas ..." << this->size();
    m = new QCursor ();		// For handling the mouse.

    scaleStep = STEP;		// Initial scale is 1:1.
    baseScale = scaleStep;
    baseFontSize = fontInfo().pointSize() + 2;
    scaleStep = (int) ((scale * STEP) + 0.05);
    qDebug() << "Scale" << scale << "Scaled Step" << scaleStep;

    nCellsW = FIELDWIDTH;
    nCellsH = FIELDHEIGHT;
    border = 4;			// Make border at least tiles wide all around.
    lineDivider = 8;		// Make lines of inner border 1/8 tile wide.

    heroSprite = 0;

    // Create an empty list of enemy sprites.
    enemySprites = new QList<KGrSprite *> ();

    picsDataDir = systemDataDir + "../pics/";
    filepathSVG = "";

    qDebug() << "Calling initView() ...";
    initView();			// Set up the graphics, etc.

    // Initialise the KGoldrunner grid.
    for (int x = 0; x < FIELDWIDTH; x++) {
	for (int y = 0; y < FIELDHEIGHT; y++) {
	    tileNo[x][y] = freebg;
	}
    }

    title = 0;
}

KGrCanvas::~KGrCanvas()
{
    tileset->clear();
    heroFrames->clear();
    enemyFrames->clear();
    delete tileset;
    delete heroFrames;
    delete enemyFrames;
}

void KGrCanvas::drawTheScene (bool changePixmaps)
{
    // The pixmaps for tiles and sprites have to be re-loaded
    // if and only if the theme or cell size has changed.

    double scale = (double) imgW / (double) bgw;
    qDebug() << "Called KGrCanvas::drawTheScene() - Images:" << imgW<<"x"<<imgH;

    // Draw the tiles in the playfield.
    if (playfield) {
	makeTiles (changePixmaps);

	// Set all cells to same tiles (i.e. tile-numbers) as before.
	for (int x = 0; x < FIELDWIDTH; x++) {
	    for (int y = 0; y < FIELDHEIGHT; y++) {
		playfield->setTile (x, y, tileNo[x][y]);
	    }
	}
    }

    /* ******************************************************************** */
    /* The pixmaps for hero and enemies are arranged in strips of 36: walk  */
    /* right (8), walk left (8), climb right along bar (8), climb left (8), */
    /* climb up ladder (2) and fall (2) --- total 36.  The appendFrames()   */
    /* method extracts the frames from the strip, scales                    */
    /* them and adds them to a list of frames (QPixmaps).                   */
    /* ******************************************************************** */

    if (changePixmaps) {
	heroFrames->clear();
	enemyFrames->clear();

	if (runnerGraphics == SVG) {
	    // Draw the hero's animation frames.
	    appendSVGFrames ("hero_%1", heroFrames, 36);

	    // Draw the enemies' animation frames.
	    appendSVGFrames ("enemy_%1", enemyFrames, 36);          // Plain.
	    appendSVGFrames ("gold_enemy_%1", enemyFrames, 36);     // Has gold.
	}
	else {
	    // Draw the hero's animation frames.
	    appendXPMFrames (QImage (hero_xpm), heroFrames, 36);

	    // Draw the enemies' animation frames.
	    appendXPMFrames (QImage(enemy1_xpm), enemyFrames, 36);  // Plain.
	    appendXPMFrames (QImage (enemy2_xpm), enemyFrames, 36); // Has gold.
	}
    }

    int spriteframe;
    QPoint spriteloc;

    // Draw the hero.
    if (heroSprite) {
    	spriteframe = heroSprite->currentFrame();
    	spriteloc = heroSprite->currentLoc();
    	heroSprite->addFrames (heroFrames, topLeft, scale);

	// Force a re-draw of both pixmap and position.
    	heroSprite->move (0, 0, (spriteframe > 0) ? 0 : 1);
    	heroSprite->move (spriteloc.x(), spriteloc.y(), spriteframe);
    }

    if (enemySprites) {
	for (int i = 0; i < enemySprites->size(); ++i) {
    	    KGrSprite * thisenemy = enemySprites->at(i);
	    if (thisenemy) {
		spriteframe = thisenemy->currentFrame();
    		spriteloc = thisenemy->currentLoc();
		thisenemy->addFrames (enemyFrames, topLeft, scale);

		// Force re-draw of both pixmap and position.
		thisenemy->move (0, 0, (spriteframe > 0) ? 0 : 1);
    		thisenemy->move (spriteloc.x(), spriteloc.y(), spriteframe);
	    }
	}
    }

    // Recreate the border.
    makeBorder ();

    // Force the title size and position to be re-calculated.
    QString t = title->text();
    makeTitle ();
    setTitle (t);
}

void KGrCanvas::appendXPMFrames (const QImage & image,
		QList<QPixmap> * frames, const int nFrames)
{
    QPixmap   pm;
    for (int i = 0; i < nFrames; i++) {
        pm = QPixmap::fromImage (image.copy (i * bgw, 0, bgw, bgh));
        frames->append (pm.scaledToHeight (imgH, Qt::FastTransformation));
    }
}

void KGrCanvas::appendSVGFrames (const QString & elementPattern,
		QList<QPixmap> * frames, const int nFrames)
{
    QImage img (imgW, imgH, QImage::Format_ARGB32_Premultiplied);
    QPainter q (&img);
    for (int i = 1; i <= nFrames; i++) {
	QString s = elementPattern.arg(i);	// e.g. "hero_1", "hero_2", etc.
	img.fill (0);
	svg.render (&q, s);
	frames->append (QPixmap::fromImage (img));
    }
}

void KGrCanvas::changeTheme (const QString & themeFilepath)
{
    qDebug() << endl << "New Theme -" << themeFilepath;
    if (! m_themeFilepath.isEmpty() && (themeFilepath == m_themeFilepath)) {
	qDebug() << "NO CHANGE OF THEME ...";
	return;					// No change of theme.
    }

    KConfig theme (themeFilepath, KConfig::OnlyLocal);	// Read graphics config.
    KConfigGroup group = theme.group ("KDEGameTheme");
    QString f = group.readEntry ("FileName", "");

    //Check if the theme asks us to draw our border, and set the specified color
    themeDrawBorder = group.readEntry ("DrawCanvasBorder", 0);
    if (themeDrawBorder) {
      QString themeBorderColor = group.readEntry ("BorderColor", "");
      if (!themeBorderColor.isEmpty()) {
        borderColor.setNamedColor(themeBorderColor);
      }
    }
    //If specified, also set the title color
    QString themeTextColor = group.readEntry ("TextColor", "");
    if (!themeTextColor.isEmpty()) {
      textColor.setNamedColor(themeTextColor);
    }

    if (f.endsWith (".svg") || f.endsWith (".svgz")) {
	// Load a SVG theme (KGoldrunner 3+ and KDE 4+).
	filepathSVG = themeFilepath.left (themeFilepath.lastIndexOf("/")+1) + f;
	loadSVGTheme ();
	tileGraphics = SVG;
	backgroundGraphics = SVG;
	runnerGraphics = SVG;
    }
    else {
	// Load a XPM theme (KGoldrunner 2 and KDE 3).
	int colorIndex = group.readEntry ("ColorIndex", 0);
	changeColours (& colourScheme [colorIndex * 12]);
	tileGraphics = XPM;
	backgroundGraphics = XPM;
	runnerGraphics = XPM;
        themeDrawBorder = 1;
    }

    // If there is no theme previously loaded, KGrCanvas is just starting up
    // and the play-area will show later, during the initial resizeEvent().
    if (! m_themeFilepath.isEmpty()) {
	bool changePixmaps = true;
	drawTheScene (changePixmaps);	// Not startup, so re-draw play-area.
    }

    // Save the user's selected theme in KDE's config-group data for the game.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("ThemeFilepath", themeFilepath);
    gameGroup.sync();			// Ensure that the entry goes to disk.
    m_themeFilepath = themeFilepath;
}

void KGrCanvas::resizeEvent (QResizeEvent * event /* (unused) */)
{
    t.start();
    resizeCount++;			// IDW
    qDebug() << endl << "KGrCanvas::resizeEvent:" << resizeCount << event->size();
    double w = (double) event->size().width()  / (nCellsW + border);
    double h = (double) event->size().height() / (nCellsH + border);
    int cellSize = (w < h) ? (int) (w + 0.05) : (int) (h + 0.05);
    // IDW cellSize = cellSize - (cellSize % 4);	// Set a multiple of 4.

    imgW = cellSize;
    imgH = cellSize;
    topLeft.setX ((event->size().width()  - (nCellsW * imgW)) / 2);
    topLeft.setY ((event->size().height() - (nCellsH * imgH)) / 2);

    bool changePixmaps = ((imgW != oldImgW) || (imgH != oldImgH));
    oldImgW = imgW;
    oldImgH = imgH;

    drawTheScene (changePixmaps);
    QWidget::resizeEvent (event);
}

void KGrCanvas::paintCell (int x, int y, char type, int offset)
{
    int tileNumber = 0;

    switch (type) {
    case FREE:    tileNumber = freebg; break;	// Free space.
    case NUGGET:  tileNumber = nuggetbg; break;	// Nugget.
    case POLE:    tileNumber = polebg; break;	// Pole or bar.
    case LADDER:  tileNumber = ladderbg; break;	// Ladder.
    case HLADDER: tileNumber = hladderbg; break;// Hidden ladder.
    case HERO:    tileNumber = edherobg; break;	// Static hero (for editing).
    case ENEMY:   tileNumber = edenemybg; break;// Static enemy (for editing).
    case BETON:   tileNumber = betonbg;	 break;	// Concrete.
    case BRICK:   tileNumber = brickbg;	 break;	// Solid brick.
    case FBRICK:  tileNumber = fbrickbg; break;	// False brick.
    default:      tileNumber = freebg;  break;
    }

    // In KGrGame, the top-left visible cell is [1,1]: in KGrPlayfield [0,0].
    x--; y--;
    tileNumber = tileNumber + offset;	// Offsets 1-9 are for digging sequence.
    tileNo [x][y] = tileNumber;		// Save the tile-number for repaint.

    playfield->setTile (x, y, tileNumber);	// Paint with required pixmap.
}

void KGrCanvas::setBaseScale ()
{
    // Synchronise the desktop font size with the initial canvas scale.
    baseScale = scaleStep;
    QString t = "";
    if (title) {
	t = title->text();
    }
    makeTitle ();
    setTitle (t);
}

void KGrCanvas::setTitle (const QString &newTitle)
{
    title->setText (newTitle);
}

void KGrCanvas::makeTitle ()
{
    if (title != 0)
	title->close();			// Close and delete previous title.

    title = new QLabel ("", this);
    int lw = imgW / lineDivider;	// Line width (as used in makeBorder()).
    title->resize (width(), topLeft.y() - lw);
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
    QPoint p = mapFromGlobal (m->pos());

    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    i = ((p.x() - topLeft.x()) / imgW) + 1;
    j = ((p.y() - topLeft.y()) / imgH) + 1;

    return (QPoint (i, j));
}

void KGrCanvas::setMousePos (int i, int j)
{
    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    i--; j--;
    m->setPos (mapToGlobal (QPoint (
				topLeft.x() + i * imgW + imgW / 2,
				topLeft.y() + j * imgH + imgH / 2)));
}

void KGrCanvas::makeHeroSprite (int i, int j, int startFrame)
{
    heroSprite = new KGrSprite (this);

    double scale = (double) imgW / (double) bgw;
    heroSprite->addFrames (heroFrames, topLeft, scale);

    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    i--; j--;
    heroSprite->move (i * bgw, j * bgh, startFrame);
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

    double scale = (double) imgW / (double) bgw;
    enemySprite->addFrames (enemyFrames, topLeft, scale);
    enemySprites->append (enemySprite);

    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    i--; j--;
    enemySprite->move (i * bgw, j * bgh, startFrame);
    enemySprite->setZ (2);
    enemySprite->show();
}

void KGrCanvas::moveHero (int x, int y, int frame)
{
    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    heroSprite->move (x - bgw, y - bgh, frame);
}

void KGrCanvas::moveEnemy (int id, int x, int y, int frame, int nuggets)
{
    if (nuggets != 0) {				// If enemy is carrying gold,
	frame = frame + goldEnemy;		// show him with gold outline.
    }

    // In KGoldrunner, the top-left visible cell is [1,1]: in KGrSprite [0,0].
    enemySprites->at(id)->move (x - bgw, y - bgh, frame);
}

void KGrCanvas::deleteEnemySprites()
{
    while (!enemySprites->isEmpty())
            delete enemySprites->takeFirst();
}

QPixmap KGrCanvas::getPixmap (char type)
{
    int tileNumber;

    // Get a pixmap from the tile-array for use on an edit-button.
    switch (type) {
    case FREE:    tileNumber = freebg; break;	// Free space.
    case NUGGET:  tileNumber = nuggetbg; break;	// Nugget.
    case POLE:    tileNumber = polebg; break;	// Pole or bar.
    case LADDER:  tileNumber = ladderbg; break;	// Ladder.
    case HLADDER: tileNumber = hladderbg; break;// Hidden ladder.
    case HERO:    tileNumber = edherobg; break;	// Static hero (for editing).
    case ENEMY:   tileNumber = edenemybg; break;// Static enemy (for editing).
    case BETON:   tileNumber = betonbg;	 break;	// Concrete.
    case BRICK:   tileNumber = brickbg;	 break;	// Solid brick.
    case FBRICK:  tileNumber = fbrickbg; break;	// False brick.
    default:      tileNumber = freebg;  break;
    }

    return tileset->at(tileNumber);
}

void KGrCanvas::initView()
{
    // Set up the pixmaps for the editable objects.
    freebg	= 0;		// Free space.
    nuggetbg	= 1;		// Nugget.
    polebg	= 2;		// Pole or bar.
    ladderbg	= 3;		// Ladder.
    hladderbg	= 4;		// Hidden ladder.
    edherobg	= 5;		// Static hero (for editing).
    edenemybg	= 6;		// Static enemy (for editing).
    betonbg	= 7;		// Concrete.
    fbrickbg	= 8;		// False brick.

    // The bricks have 10 pixmaps (showing various stages of digging).
    brickbg	= 9;		// Solid brick - 1st pixmap.


    // Set default tile-size (if not played before or no KMainWindow settings).
    imgW = (bgw * scaleStep) / STEP;
    imgH = (bgh * scaleStep) / STEP;
    oldImgW = 0;
    oldImgH = 0;

    // Define the canvas as an array of tiles.  Default tile is 0 (free space).
    playfield = new KGrPlayField(this);	// The drawing-area for the playfield.
    tileset     = new QList<QPixmap>();	// The tiles that can be set in there.
    heroFrames  = new QList<QPixmap>();	// Animation frames for the hero.
    enemyFrames = new QList<QPixmap>();	// Animation frames for enemies.
    goldEnemy = 36;			// Offset of gold-carrying frames.

    // Set a null graphics theme (will initialise in KGrCanvas::changeTheme()).
    m_themeFilepath = "";
    tileGraphics = NONE;
    backgroundGraphics = NONE;
    runnerGraphics = NONE;
}

void KGrCanvas::makeTiles (bool changePixmaps)
{
    bool SVGmode = false;

    if (tileGraphics == SVG) {
	SVGmode = svg.isValid();
    }

    // TODO: Sort out whether QImage or QPixmap is best for loading the background.
    // Make an empty background image.
    // QImage * background = 0;
    QPixmap * background = 0;

    if (SVGmode && (backgroundGraphics == SVG)) {
	// Draw SVG background and tiles.
	// background = new QImage (this->width(), this->height(),
				// QImage::Format_ARGB32_Premultiplied);
	// background->fill (0);
	background = new QPixmap (this->width(), this->height());

	QPainter b (background);
	svg.render (&b, "background");
    }

    if (changePixmaps) {
	tileset->clear();

	if (SVGmode && (tileGraphics == SVG)) {
	    // Draw SVG versions of nugget, bar, ladder, concrete and brick.
	    QImage img (imgW, imgH, QImage::Format_ARGB32_Premultiplied);
	    QPainter q (&img);

	    appendSVGTile (img, q, "empty");
	    appendSVGTile (img, q, "gold");
	    appendSVGTile (img, q, "bar");
	    appendSVGTile (img, q, "ladder");
	    appendSVGTile (img, q, "hidden_ladder");
	    appendSVGTile (img, q, "hero_1");	// For use in edit mode.
	    appendSVGTile (img, q, "enemy_1");	// For use in edit mode.
	    appendSVGTile (img, q, "concrete");
	    appendSVGTile (img, q, "false_brick");
	    appendSVGTile (img, q, "brick");

            // Add SVG versions of blasted bricks.
            appendSVGTile (img, q, "brick_1");
            appendSVGTile (img, q, "brick_2");
            appendSVGTile (img, q, "brick_3");
            appendSVGTile (img, q, "brick_4");
            appendSVGTile (img, q, "brick_5");
            appendSVGTile (img, q, "brick_6");
            appendSVGTile (img, q, "brick_7");
            appendSVGTile (img, q, "brick_8");
            appendSVGTile (img, q, "brick_9");
	}
	else {
	    QImage bricks (bricks_xpm);
	    bricks = bricks.scaledToHeight(imgH);

	    tileset->append (QPixmap(hgbrick_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(nugget_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(pole_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(ladder_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(hladder_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(edithero_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(editenemy_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap(beton_xpm).scaledToHeight(imgH));
	    tileset->append (QPixmap::fromImage	// Extract false-brick image.
				    (bricks.copy (2 * imgW, 0, imgW, imgH)));

            // Make the brick-digging sprites (whole brick and blasted bricks).
            for (int i = 0; i < 10; i++) {
                tileset->append (QPixmap::fromImage
				    (bricks.copy (i * imgW, 0, imgW, imgH)));
            }
	}

    }	// END if (changePixmaps).

    // Now set our tileset in the scene.
    playfield->setBackground (background);
    playfield->setTiles (tileset, topLeft, nCellsW, nCellsH, imgW, imgH);

    delete background;
}

void KGrCanvas::appendSVGTile (QImage & img, QPainter & q, const QString & name)
{
    img.fill (0);
    svg.render (&q, name);
    tileset->append (QPixmap::fromImage (img));
}

void KGrCanvas::makeBorder ()
{
    // Draw main part of border, in the order: top, bottom, left, right.
    // Allow some overlap to prevent slits appearing when resizing.

    colour = borderColor;
    int cw = width();				// Size of canvas.
    int ch = width();
    int pw = nCellsW * imgW;			// Size of playfield.
    int ph = nCellsH * imgW;
    int tlX = topLeft.x();			// Top left of playfield.
    int tlY = topLeft.y();
    int lw = imgW / lineDivider;		// Line width.

    while (!borderRectangles.isEmpty())
            delete borderRectangles.takeFirst();

    KGameCanvasRectangle * nextRectangle;

    // If SVG/PNG background, coloured border is specified in the theme properties, otherwise the background fills the canvas.
    if (themeDrawBorder) {
	nextRectangle = drawRectangle (0, 0, cw, tlY - lw);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (0, tlY + ph + lw,
						cw, ch - tlY - ph - lw);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (0, tlY - lw, tlX - lw, ph + 2*lw);
	borderRectangles.append(nextRectangle);
	nextRectangle = drawRectangle (tlX + pw + lw, tlY - lw,
						cw - tlX -pw - lw, ph + 2*lw);
	borderRectangles.append(nextRectangle);
    }

    // Draw the inside edges of the border, in the same way.
    colour = QColor (Qt::black);

    nextRectangle = drawRectangle (tlX - lw, tlY - lw, pw + 2*lw, lw);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (tlX - lw, ph + tlY, pw + 2*lw, lw);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (tlX - lw, tlY, lw, ph);
    borderRectangles.append(nextRectangle);
    nextRectangle = drawRectangle (tlX + pw, tlY, lw, ph);
    borderRectangles.append(nextRectangle);
}

KGameCanvasRectangle * KGrCanvas::drawRectangle (int x, int y, int w, int h)
{
    KGameCanvasRectangle * r =
		new KGameCanvasRectangle (colour, QSize(w, h), this);
    r->moveTo (x, y);
    r->show();
    return (r);
}

void KGrCanvas::changeColours (const char * colours [])
{
    // KGoldrunner 2 landscape, xpm-based.
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
}

void KGrCanvas::loadSVGTheme()
{
    t.start();
    qDebug() << t.restart() << "msec" << "filepathSVG:" << filepathSVG;
    if (! filepathSVG.isEmpty()) {
	svg.load (filepathSVG);
	qDebug() << t.restart() << "msec" << "SVG loaded ...";
    }
    else {
	qDebug() << t.restart() << "msec" << "Not a SVG theme ...";
    }
}

void KGrCanvas::recolourObject (const char * object [], const char * colours [])
{
    int i;
    for (i = 0; i < 9; i++) {
	object [i+1] = colours [i+3];
    }
}

QSize KGrCanvas::sizeHint() const
{
    return QSize ((nCellsW  + border) * imgW, (nCellsH + border) * imgH);
}

#include "kgrcanvas.moc"
