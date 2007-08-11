/***************************************************************************
                          kgrcanvas.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
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

#include "kgrcanvas.h"

#ifdef KGR_PORTABLE
// If compiling for portability, redefine KDE's i18n.
#define i18n tr
#endif

#include <QPixmap>
#include <QList>
#include <QLabel>
#include <QMouseEvent>

#include <KDebug>

#include <KConfig>

#include <cmath>

KGrCanvas::KGrCanvas (QWidget * parent, const double scale,
			const QString & systemDataDir)
			: KGameCanvasWidget (parent),
			  topLeft (0, 0), bgw (4 * STEP), bgh (4 * STEP),
			  m_fadingTimeLine(1000, this),
			  theme(systemDataDir)
{
    resizeCount = 0;		// IDW

    kDebug() << "Called KGrCanvas::KGrCanvas ..." << this->size();
    m = new QCursor ();		// For handling the mouse.

    scaleStep = STEP;		// Initial scale is 1:1.
    baseScale = scaleStep;
    baseFontSize = fontInfo().pointSize() + 2;
    scaleStep = (int) ((scale * STEP) + 0.05);
    kDebug() << "Scale" << scale << "Scaled Step" << scaleStep;

    nCellsW = FIELDWIDTH;
    nCellsH = FIELDHEIGHT;
    border = 4;			// Make border at least tiles wide all around.
    lineDivider = 8;		// Make lines of inner border 1/8 tile wide.

    heroSprite = 0;

    // Create an empty list of enemy sprites.
    enemySprites = new QList<KGrSprite *> ();

    kDebug() << "Calling initView() ...";
    initView();			// Set up the graphics, etc.

    // Initialise the KGoldrunner grid.
    for (int x = 0; x < FIELDWIDTH; x++) {
	for (int y = 0; y < FIELDHEIGHT; y++) {
	    tileNo[x][y] = freebg;
	}
    }

    title = 0;
    level = 0;
    setMinimumSize(FIELDWIDTH + 4, FIELDHEIGHT + 4);
    m_spotLight = new KGameCanvasPicture(this);
    m_fadingTimeLine.setCurveShape(QTimeLine::LinearCurve);
    m_fadingTimeLine.setUpdateInterval( 80 );
    connect(&m_fadingTimeLine, SIGNAL(valueChanged(qreal)), this, SLOT(drawSpotLight(qreal)));
}

KGrCanvas::~KGrCanvas()
{
    tileset->clear();
    heroFrames->clear();
    enemyFrames->clear();
    delete tileset;
    delete heroFrames;
    delete enemyFrames;
    delete m_spotLight;
}

void KGrCanvas::goToBlack()
{
	t.start(); // IDW
	// IDW kDebug() << "Go to black ...";
	drawSpotLight (0);
}

void KGrCanvas::fadeIn()
{
	t.start(); // IDW
	// IDW kDebug() << t.elapsed() << "msec" << "Fading in ..."; // IDW
	m_fadingTimeLine.setDirection(QTimeLine::Forward);
	m_fadingTimeLine.start();
}

void KGrCanvas::fadeOut()
{
	t.start(); // IDW
	// IDW kDebug() << t.elapsed() << "msec" << "Fading out ..."; // IDW
	m_fadingTimeLine.setDirection(QTimeLine::Backward);
	m_fadingTimeLine.start();
}

void KGrCanvas::drawTheScene (bool changePixmaps)
{
    // The pixmaps for tiles and sprites have to be re-loaded
    // if and only if the theme or the cell size has changed.

    // The background has to be re-loaded if the theme has
    // changed, the cell size has changed or the canvas size
    // has changed and the bg must fill it (! themeDrawBorder).

    double scale = (double) imgW / (double) bgw;
    kDebug() << "Called KGrCanvas::drawTheScene() - Images:" << imgW<<"x"<<imgH;
    if (imgW == 0) {
        return;
    }

    // Draw the tiles and background in the playfield.
    if (playfield) {
	makeTiles (changePixmaps);

	// Set each cell to same type of tile (i.e. tile-number) as before.
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

	*heroFrames << theme.hero(imgH);
	*enemyFrames << theme.enemy(imgH);
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
	    kDebug() << "accessing enemySprite" << i;
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

void KGrCanvas::changeTheme (const QString & themeFilepath)
{
    kDebug()<< "New Theme -" << themeFilepath;
    theme.load(themeFilepath);
    const bool changePixmaps = true;
    drawTheScene (changePixmaps);	// Not startup, so re-draw play-area.
}

void KGrCanvas::resizeEvent (QResizeEvent * event )
{
    resizeCount++;			// IDW
    kDebug()<< "KGrCanvas::resizeEvent:" << resizeCount << event->size();
    kDebug() << "Resize pending?" << QWidget::testAttribute (Qt::WA_PendingResizeEvent);
    // To reduce overheads, re-render only when no later resize is scheduled.
    if (QWidget::testAttribute (Qt::WA_PendingResizeEvent))  {
	return;
    }

    t.start(); // IDW
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
    palette.setColor(title->backgroundRole(), theme.borderColor());
    palette.setColor(title->foregroundRole(), theme.textColor());
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
    // kDebug() << "accessing enemySprite" << id;
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

    kDebug() << "accessing tile" << tileNumber;
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
}

void KGrCanvas::setLevel(unsigned int l)
{
    if (l != level) {
	level= l;
	if (theme.multipleBackgrounds()) {
	    loadBackground();
	}
    }
}

void KGrCanvas::loadBackground()
{
    kDebug() << "loadBackground called";
    bool fillCanvas = !theme.isBorderRequired();	// Background must fill canvas?
    int w = fillCanvas ? (this->width())  : (nCellsW * imgW);
    int h = fillCanvas ? (this->height()) : (nCellsH * imgH);
    if (theme.isWithBackground()) {
	QImage background = theme.background(w, h, level);
	playfield->setBackground (true, 
		&background, 
		theme.isBorderRequired() ? topLeft : QPoint(0, 0));
    } else {
	playfield->setBackground (true, 
		NULL, 
		theme.isBorderRequired() ? topLeft : QPoint(0, 0));
    }
}

void KGrCanvas::makeTiles (bool changePixmaps)
{
    // Make an empty background image.
    //bool fillCanvas = !theme.isBorderRequired();	// Background must fill canvas?
    //int w = fillCanvas ? (this->width())  : (nCellsW * imgW);
    //int h = fillCanvas ? (this->height()) : (nCellsH * imgH);
    // QImage background = theme.background(w, h, 0);
    loadBackground();

    if (changePixmaps) {
	tileset->clear();

	*tileset << theme.tiles(imgH);
    }

    //if (theme.isWithBackground()) {
//	playfield->setBackground (true, 
//			&background, 
//			theme.isBorderRequired() ? topLeft : QPoint(0, 0));
//    }

    // Now set our tileset in the scene.
    playfield->setTiles (tileset, topLeft, nCellsW, nCellsH, imgW, imgH);
}

void KGrCanvas::makeBorder ()
{
    // Draw main part of border, in the order: top, bottom, left, right.
    // Allow some overlap to prevent slits appearing when resizing.

    colour = theme.borderColor();
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

    // If SVG/PNG background, coloured border is specified in the theme
    // properties, otherwise the background fills the canvas.
    if (theme.isBorderRequired()) {
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

void KGrCanvas::drawSpotLight(qreal value)
{
    static int count = 0;
    if (value > 0.99) {
	// IDW kDebug() << t.elapsed() << "msec" << "Hide spotlight, value:" << value; // IDW
	count = 0;
	// Hide the spotlight animation -- all scene visible
	m_spotLight->hide();
	m_spotLight->lower();
	return;
    }
    count++;
    m_spotLight->raise();
    m_spotLight->show();
    QPicture picture;
    qreal w = qreal(nCellsW * imgW);
    qreal h = qreal(nCellsH * imgW);
    qreal x = qreal(topLeft.x());
    qreal y = qreal(topLeft.y());
    qreal dh = 0.5 * ::sqrt(w * w + h * h);

    QPainter p(&picture);
    if (value < 0.01) {
	// Draw a solid black background if the circle would be too small/
	p.fillRect(QRectF(x, y, w, h), QColor(0, 0, 0, 255));
    } else {
	static const qreal outerRatio = 1.00;
	static const qreal innerRatio = 0.85;
	static const qreal sqrt1_2 = 0.5 * ::sqrt(2);
	qreal wh = w * 0.5;
	qreal hh = h * 0.5;
	qreal radius = dh * value;
	qreal innerRadius = radius * innerRatio;
	qreal innerDistance = sqrt1_2 * innerRadius;
	qreal side = 2.0 * innerDistance;

	QPointF center(width() * 0.5, height() * 0.5);
	QRadialGradient gradient(center, radius, center);
	gradient.setColorAt(outerRatio, QColor(0, 0, 0, 255));
	gradient.setColorAt(innerRatio, QColor(0, 0, 0, 0));

	QBrush brush(gradient);
        QBrush blackbrush(QColor(0, 0, 0, 255));
	// Draw a transparent circle over the scene.
	p.setCompositionMode(QPainter::CompositionMode_SourceOver);
#if 0
	if (radius < wh) {
	    qreal diameter = radius * 2.0;
	    p.fillRect(QRectF(x, y, 1.0 + wh - radius, h), blackbrush);
	    p.fillRect(QRectF(x + wh + radius, y, wh - radius, h), blackbrush);
	    if (radius < h * 0.5) {
		p.fillRect(QRectF(x + wh - radius, y, diameter, 0.5 * h - radius + 1.0), blackbrush);
		p.fillRect(QRectF(x + wh - radius, y + h * 0.5 + radius, 2.0 * radius, 0.5 * h - radius), blackbrush);
		p.fillRect(QRectF(x + wh - radius, y + h * 0.5 - radius, diameter, diameter), brush);
	    } else {
		p.fillRect(QRectF(x + wh - radius, y, 2.0 * radius, h), brush);
	    }
	} else {
	    p.fillRect(QRectF(x, y, w, h), brush);
	}
#else
	if (radius < wh) {
	    // If the spotlight radius is smaller than half the scene width, draw
	    // black rectangles to its sides, which is faster.
	    qreal diameter = radius * 2.0;
	    p.fillRect(QRectF(x, y, 1.0 + wh - radius, h), blackbrush);
	    p.fillRect(QRectF(x + wh + radius, y, wh - radius, h), blackbrush);
	    if (radius < hh) {
		// If the spotlight radius is smaller than half the scene
		// height, draw black rectangles to its top and bottom sides,
		// which is faster.
		p.fillRect(QRectF(x + wh - radius, y, diameter, hh - radius + 1.0), blackbrush);
		p.fillRect(QRectF(x + wh - radius, y + hh + radius, diameter, hh - radius), blackbrush);
		
		// Draw the spotlight circle, but skip the transparent center.
		p.fillRect(QRectF(x + wh - radius, y + hh - radius, radius - innerDistance, diameter), brush);
		p.fillRect(QRectF(x + wh + innerDistance, y + hh - radius, radius - innerDistance, diameter), brush);
		p.fillRect(QRectF(x + wh - innerDistance, y + hh - radius, side, radius - innerDistance), brush);
		p.fillRect(QRectF(x + wh - innerDistance, y + hh + innerDistance, side, radius - innerDistance), brush);
	    } else {
		// Else draw the radial gradient
		p.fillRect(QRectF(x + wh - radius, y, diameter, h), brush);
	    }
	} else {
	    // If the spotlight is bigger than the scene, draw only the
	    // sections where the gradient is not transparent.
	    p.fillRect(QRectF(x, y, wh - innerDistance, h), brush);
	    p.fillRect(QRectF(x + wh + innerDistance, y, wh - innerDistance, h), brush);
	    if (innerDistance < hh) {
		p.fillRect(QRectF(x + wh - innerDistance, y, side, hh - innerDistance), brush);
		p.fillRect(QRectF(x + wh - innerDistance, y + hh + innerDistance, side, hh - innerDistance), brush);
	    }
	}
#endif
    }

    m_spotLight->setPicture(picture);
    // IDW kDebug() << t.elapsed() << "msec" << "Spotlight frame count: " << count << ", value:" << value; // IDW
}

KGameCanvasRectangle * KGrCanvas::drawRectangle (int x, int y, int w, int h)
{
    KGameCanvasRectangle * r =
		new KGameCanvasRectangle (colour, QSize(w, h), this);
    r->moveTo (x, y);
    r->show();
    return (r);
}

QSize KGrCanvas::sizeHint() const
{
    return QSize ((nCellsW  + border) * imgW, (nCellsH + border) * imgH);
}

#include "kgrcanvas.moc"
