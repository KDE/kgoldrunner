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
#include "klocale.h"

#include <QPixmap>
#include <QList>
#include <QLabel>
#include <QMouseEvent>

#include <KDebug>

#include <KConfig>

#include <cmath>


// Helper function: find how many tiles are needed to cover at least w pixels
static int numSections(int w, int sectionWidth)
{
    return 1 + w / sectionWidth; 
}

static QString scoreText(int score) 
{
    return ki18n ("Score: ").toString() + QString::number(score).rightJustified(7, '0');
}

static QString livesText(int lives) 
{
    return ki18n ("Lives: ").toString() + QString::number(lives).rightJustified(3, '0');
}

KGrCanvas::KGrCanvas (QWidget * parent, const double scale,
                        const QString & systemDataDir)
                        : KGameCanvasWidget (parent),
                          firstSceneDrawn (false),
                          topLeft (0, 0), bgw (4 * STEP), bgh (4 * STEP),
                          m_scoreText(0),
                          m_livesText(0),
                          m_scoreDisplay(0),
                          m_livesDisplay(0),
                          m_fadingTimeLine (1000, this),
                          theme (systemDataDir)

{
    resizeCount = 0;		// IDW

    kDebug() << "Called KGrCanvas::KGrCanvas ..." << this->size();
    m = new QCursor();		// For handling the mouse.

    // The default background is black.  It appears during partial repaints.
    setPalette (QPalette (Qt::black));
    setAutoFillBackground (true);

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
    enemySprites = new QList<KGrSprite *>();

    kDebug() << "Calling initView() ...";
    initView();			// Set up the graphics, etc.

    // Initialise the KGoldrunner grid.
    unsigned int seed = 42;
    for (int x = 0; x < FIELDWIDTH; x++) {
	for (int y = 0; y < FIELDHEIGHT; y++) {
	    tileNo[x][y] = KGrTheme::EmptyTile;
	    randomOffsets[x][y] = rand_r (&seed);
	}
    }

    title = 0;
    level = 0;

    // Set minimum size to 12x12 pixels per tile.
    setMinimumSize ((FIELDWIDTH + 4) * 12, (FIELDHEIGHT + 4) * 12);

    m_scoreText = new KGameCanvasText ("", Qt::white, QFont(), 
            KGameCanvasText::HLeft, KGameCanvasText::VTop, this);
    m_livesText = new KGameCanvasText ("", Qt::white, QFont(), 
            KGameCanvasText::HRight, KGameCanvasText::VTop, this);

    m_spotLight = new KGameCanvasPicture (this);
    m_fadingTimeLine.setCurveShape (QTimeLine::LinearCurve);
    m_fadingTimeLine.setUpdateInterval (60);
    connect (&m_fadingTimeLine, SIGNAL (valueChanged (qreal)),
                this, SLOT (drawSpotLight (qreal)));
    connect (&m_fadingTimeLine, SIGNAL (finished()),
                this, SIGNAL (fadeFinished()));
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
        drawSpotLight (0);
}

void KGrCanvas::fadeIn()
{
        m_fadingTimeLine.setDirection (QTimeLine::Forward);
        m_fadingTimeLine.start();
}

void KGrCanvas::fadeOut()
{
        m_fadingTimeLine.setDirection (QTimeLine::Backward);
        m_fadingTimeLine.start();
}

void KGrCanvas::drawTheScene (bool changePixmaps)
{
    t.restart();
    kDebug() << 0 << "msec.  Start drawTheScene";

    // The pixmaps for tiles and sprites have to be re-loaded
    // if and only if the theme or the cell size has changed.

    // The background has to be re-loaded if the theme has
    // changed, the cell size has changed or the canvas size
    // has changed and the bg must fill it (! themeDrawBorder).

    double scale = (double) imgW / (double) bgw;
    kDebug() << "Images:" << imgW<<"x"<<imgH;
    if (imgW == 0) {
        return;
    }

    // Draw the tiles and background in the playfield.
    if (playfield) {
	loadBackground();

	if (changePixmaps) {
	    tileset->clear();

	    *tileset << theme.tiles (imgH);
	}

	// Now set our tileset in the scene.
	playfield->setTiles (tileset, topLeft, nCellsW, nCellsH, imgW, imgH);

	// Set each cell to same type of tile (i.e. tile-number) as before.
	for (int x = 0; x < FIELDWIDTH; x++) {
	    for (int y = 0; y < FIELDHEIGHT; y++) {
		playfield->setTile (x, y, tileNumber(tileNo[x][y], x, y));
	    }
	}
    }
    kDebug() << t.restart() << "msec.  Tiles + background done.";

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

        *heroFrames << theme.hero (imgH);
        *enemyFrames << theme.enemy (imgH);
    }
    kDebug() << t.restart() << "msec.  Hero + enemies done.";

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
            // kDebug() << "accessing enemySprite" << i;
            KGrSprite * thisenemy = enemySprites->at (i);
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
    kDebug() << t.restart() << "msec.  Finished drawTheScene.";

    // Recreate the border.
    makeBorder();

    // Force the title size and position to be re-calculated.
    QString t = title->text();
    makeTitle();
    setTitle (t);

    // Create and position the score and lives text areas
    QFont f;
    f.setPixelSize (imgH - 2);
    f.setWeight (QFont::Bold);
    f.setStretch (QFont::Expanded);
    m_scoreText->setFont (f);
    m_scoreText->setColor (theme.textColor());
    int vTextPos = (1 + imgH) * nCellsH + 1;
    QPoint scorePos = topLeft + QPoint (imgW, vTextPos);

    m_livesText->setFont (f);
    m_livesText->setColor (theme.textColor());
    QPoint livesPos = topLeft + QPoint (imgW * (nCellsW - 1), vTextPos);
    QList< QPixmap > display = theme.displayTiles (imgW);
    delete m_scoreDisplay; m_scoreDisplay = 0;
    delete m_livesDisplay; m_livesDisplay = 0;
    // If the theme has display decoration support
    if (!display.isEmpty()) {
        QFontMetrics fm (f);
        int w = fm.width (scoreText (0));
        m_scoreDisplay = makeDisplay (display, w);
        m_scoreDisplay->moveTo (scorePos + QPoint (-imgW, 0));
        int sections = numSections (w, imgW);
        // Adjust score position to center them in the display
        int deltaW = (sections * imgW - w) / 2;
        scorePos.rx() += deltaW;
        w = fm.width(livesText(0));
        m_livesDisplay = makeDisplay (display, w);
        sections = numSections (w, imgW);
        m_livesDisplay->moveTo (livesPos + QPoint (-(sections + 1) * imgW, 0));
        // Adjust lives position to center them in the display
        deltaW = (sections * imgW - w) / 2;
        livesPos.rx() -= deltaW;
    }
    m_scoreText->moveTo (scorePos);
    m_livesText->moveTo (livesPos);
    m_scoreText->show();
    m_scoreText->raise();
    m_livesText->show();
    m_livesText->raise();

    // When the initial resizing, rendering and painting has been done,
    // future resizes and re-rendering will be caused by end-user activity.
    firstSceneDrawn = true;
}

bool KGrCanvas::changeTheme (const QString & themeFilepath)
{
    t.restart();
    kDebug() << 0 << "msec.  New Theme -" << themeFilepath;
    bool success = theme.load (themeFilepath);
    kDebug() << t.restart() << "msec.  Finish loading new theme.";
    if (success) {
        // Use the border color to fill empty rectangles during partial
        // repaints and soften their contrast with the rest of the picture.
        setPalette (QPalette (theme.borderColor()));
        setAutoFillBackground (true);

        const bool changePixmaps = true;
        drawTheScene (changePixmaps);
    }
    return success;
}

void KGrCanvas::resizeEvent (QResizeEvent * event)
{
    resizeCount++;
    kDebug()<< "RESIZE:" << resizeCount << event->size() <<
        "Resize pending?" << QWidget::testAttribute (Qt::WA_PendingResizeEvent)
        << "Spontaneous?" << event->spontaneous();
    
    double w = (double) event->size().width()  / (nCellsW + border);
    double h = (double) event->size().height() / (nCellsH + border);
    int cellSize = (w < h) ? (int) (w + 0.05) : (int) (h + 0.05);

    imgW = cellSize;
    imgH = cellSize;
    topLeft.setX ((event->size().width()  - (nCellsW * imgW)) / 2);
    topLeft.setY ((event->size().height() - (nCellsH * imgH)) / 2);

    bool changePixmaps = ((imgW != oldImgW) || (imgH != oldImgH));
    oldImgW = imgW;
    oldImgH = imgH;

    // The first 1, 2 or 3 resize events are caused by KMainWindow and friends,
    // so do not re-scale, render-SVG or re-draw until they are over.  After
    // that, any resize events are caused by the end-user and we must re-scale,
    // render-SVG and re-draw each time.
    if (firstSceneDrawn) {
        drawTheScene (changePixmaps);
        QWidget::resizeEvent (event);
    }
}

KGrTheme::TileType KGrCanvas::tileForType(char type)
{
    switch (type) {
    case NUGGET:
	return KGrTheme::GoldTile;
    case POLE:
	return KGrTheme::BarTile;
    case LADDER:
	return KGrTheme::LadderTile;
    case HLADDER:
	return KGrTheme::HiddenLadderTile;
    case HERO:
	return KGrTheme::HeroTile;
    case ENEMY:
	return KGrTheme::EnemyTile;
    case BETON:
	return KGrTheme::ConcreteTile;
    case BRICK:
	return KGrTheme::BrickTile;
    case FBRICK:
	return KGrTheme::FalseBrickTile;
    case FREE:
    default:
	return KGrTheme::EmptyTile;
    }
}

int KGrCanvas::tileNumber (KGrTheme::TileType type, int x, int y)
{
    // For Multiple block variant theming, the actual tile is chosen between
    // the available tiles provided, by using x and y coordinates to produce a
    // pseudorandom pattern.
    // However, we cannot do that for the brick digging sequence, so this is a
    // special case.
    const int offset = type - KGrTheme::BrickAnimation1Tile;
    const int count = theme.tileCount (KGrTheme::BrickAnimation1Tile);

    if (offset >= 0 && offset < count) {
	// Offsets 1-9 are for digging sequence.
	return theme.firstTile (KGrTheme::BrickAnimation1Tile) + offset;
    } 
    else {
	int number = theme.firstTile (type);
	int c = theme.tileCount (type);
	if (c > 1) {
	    return number + (randomOffsets[x][y] % c);
	} 
	else {
	    return theme.firstTile (type);
	}
    }
}

void KGrCanvas::paintCell (int x, int y, char type, int offset)
{
    KGrTheme::TileType tileType = tileForType (type);
    // In KGrGame, the top-left visible cell is [1,1]: in KGrPlayfield [0,0].
    x--; y--;
    switch (offset) {
    case 1: tileType = KGrTheme::BrickAnimation1Tile; break;
    case 2: tileType = KGrTheme::BrickAnimation2Tile; break;
    case 3: tileType = KGrTheme::BrickAnimation3Tile; break;
    case 4: tileType = KGrTheme::BrickAnimation4Tile; break;
    case 5: tileType = KGrTheme::BrickAnimation5Tile; break;
    case 6: tileType = KGrTheme::BrickAnimation6Tile; break;
    case 7: tileType = KGrTheme::BrickAnimation7Tile; break;
    case 8: tileType = KGrTheme::BrickAnimation8Tile; break;
    case 9: tileType = KGrTheme::BrickAnimation9Tile; break;
    case 0:
    default:
	break;
    }

    tileNo [x][y] = tileType;			// Save the tile-number for repaint.

    playfield->setTile (x, y, tileNumber (tileType, x, y));	// Paint with required pixmap.
}

void KGrCanvas::setBaseScale()
{
    // Synchronize the desktop font size with the initial canvas scale.
    baseScale = scaleStep;
    QString t = "";
    if (title) {
        t = title->text();
    }
    makeTitle();
    setTitle (t);
}

void KGrCanvas::setTitle (const QString &newTitle)
{
    title->setText (newTitle);
}

void KGrCanvas::makeTitle()
{
    if (title != 0)
        title->close();			// Close and delete previous title.

    title = new QLabel ("", this);
    int lw = imgW / lineDivider;	// Line width (as used in makeBorder()).
    title->resize (width(), topLeft.y() - lw);
    title->move (0, 0);
    QPalette palette;
    palette.setColor (title->backgroundRole(), theme.borderColor());
    palette.setColor (title->foregroundRole(), theme.textColor());
    title->setPalette (palette);
    title->setFont (QFont (fontInfo().family(),
                 (baseFontSize * scaleStep) / baseScale, QFont::Bold));
    title->setAlignment (Qt::AlignCenter);
    title->setAttribute (Qt::WA_QuitOnClose, false); //Otherwise the close above might exit app
    title->raise();
    title->show();
}

void KGrCanvas::updateScore (int score)
{
    if (m_scoreText) 
        m_scoreText->setText (scoreText(score));
}

void KGrCanvas::updateLives (int lives)
{
    // TODO use hero frames to show the lives?
    if (m_livesText) 
        m_livesText->setText (livesText(lives));
}

void KGrCanvas::mousePressEvent (QMouseEvent * mouseEvent)
{
    emit mouseClick (mouseEvent->button());
}

void KGrCanvas::mouseReleaseEvent (QMouseEvent * mouseEvent)
{
    emit mouseLetGo (mouseEvent->button());
}

QPoint KGrCanvas::getMousePos()
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
    enemySprites->at (id)->move (x - bgw, y - bgh, frame);
}

void KGrCanvas::deleteEnemySprites()
{
    while (!enemySprites->isEmpty())
        delete enemySprites->takeFirst();
}

QPixmap KGrCanvas::getPixmap (char type)
{
    return tileset->at (theme.firstTile(tileForType(type)));
}

void KGrCanvas::initView()
{
    // Set default tile-size (if not played before or no KMainWindow settings).
    imgW = (bgw * scaleStep) / STEP;
    imgH = (bgh * scaleStep) / STEP;
    oldImgW = 0;
    oldImgH = 0;

    // Define the canvas as an array of tiles.  Default tile is 0 (free space).
    playfield = new KGrPlayField (this); // The drawing-area for the playfield.
    tileset     = new QList<QPixmap>();	// The tiles that can be set in there.
    heroFrames  = new QList<QPixmap>();	// Animation frames for the hero.
    enemyFrames = new QList<QPixmap>();	// Animation frames for enemies.
    goldEnemy = 36;			// Offset of gold-carrying frames.
}

void KGrCanvas::setLevel (unsigned int l)
{
    if (l != level) {
        level= l;
        if (theme.backgroundCount() > 1) {
            loadBackground();
        }
    }
}

void KGrCanvas::loadBackground()
{
    kDebug() << "loadBackground called";
    bool fillCanvas = !theme.isBorderRequired(); // Background must fill canvas?
    int w = fillCanvas ? (this->width())  : (nCellsW * imgW);
    int h = fillCanvas ? (this->height()) : (nCellsH * imgH);
    if (theme.backgroundCount() > 0) {
        QPixmap background = theme.background (w, h, level);
        playfield->setBackground (true, background, 
                theme.isBorderRequired() ? topLeft : QPoint (0, 0));
    }
    else {
        playfield->setBackground (true, QPixmap(), 
                theme.isBorderRequired() ? topLeft : QPoint (0, 0));
    }
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
    while (!borderElements.isEmpty())
	delete borderElements.takeFirst();

    // a fancy border can be specified in the SVG file; if that is unavailable,
    // a simple border can be specified in the theme properties file.
    QList< QPixmap > l = theme.frameTiles (imgW);
    if (!l.isEmpty()) {
	kDebug() << "drawing the border tiles...";
	// Draw fancy border
	
	borderElements.append(makeBorderElement(l, tlX - imgW, tlY - imgW, 0));
	borderElements.append(makeBorderElement(l, tlX + pw, tlY - imgW, 2));
	borderElements.append(makeBorderElement(l, tlX - imgW, tlY + ph, 6));
	borderElements.append(makeBorderElement(l, tlX + pw, tlY + ph, 8));
	
	for (int i = 0; i < nCellsW * imgW; i += imgW) {
	    borderElements.append(makeBorderElement(l, tlX + i, tlY - imgW, 1));
	    borderElements.append(makeBorderElement(l, tlX + i, tlY + ph, 7));
	}
	for (int i = 0; i < nCellsH * imgH; i += imgH) {
	    borderElements.append(makeBorderElement(l, tlX - imgW, tlY + i, 3));
	    borderElements.append(makeBorderElement(l, tlX + pw, tlY + i, 5));
	}
    }
    else {
	kDebug() << "drawing the border rects...";
	KGameCanvasRectangle * nextRectangle;

	if (theme.isBorderRequired()) {
	    nextRectangle = drawRectangle (0, 0, cw, tlY - lw);
	    borderRectangles.append (nextRectangle);
	    nextRectangle = drawRectangle (0, tlY + ph + lw,
						    cw, ch - tlY - ph - lw);
	    borderRectangles.append (nextRectangle);
	    nextRectangle = drawRectangle (0, tlY - lw, tlX - lw, ph + 2*lw);
	    borderRectangles.append (nextRectangle);
	    nextRectangle = drawRectangle (tlX + pw + lw, tlY - lw,
						    cw - tlX -pw - lw, ph + 2*lw);
	    borderRectangles.append (nextRectangle);
	}

	// Draw the inside edges of the border, in the same way.
	colour = QColor (Qt::black);

	nextRectangle = drawRectangle (tlX - lw, tlY - lw, pw + 2*lw, lw);
	borderRectangles.append (nextRectangle);
	nextRectangle = drawRectangle (tlX - lw, ph + tlY, pw + 2*lw, lw);
	borderRectangles.append (nextRectangle);
	nextRectangle = drawRectangle (tlX - lw, tlY, lw, ph);
	borderRectangles.append (nextRectangle);
	nextRectangle = drawRectangle (tlX + pw, tlY, lw, ph);
	borderRectangles.append (nextRectangle);
    }
}

void KGrCanvas::drawSpotLight (qreal value)
{
    static int count = 0;
    if (value > 0.99) {
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
    qreal w = qreal (nCellsW * imgW);
    qreal h = qreal (nCellsH * imgW);
    qreal x = qreal (topLeft.x());
    qreal y = qreal (topLeft.y());
    qreal dh = 0.5 * ::sqrt (w * w + h * h);

    QPainter p;
    p.begin (&picture);
    if (value < 0.01) {
        // Draw a solid black background if the circle would be too small/
        p.fillRect (QRectF (x, y, w, h), QColor (0, 0, 0, 255));
    }
    else {
        static const qreal outerRatio = 1.00;
        static const qreal innerRatio = 0.85;
        static const qreal sqrt1_2 = 0.5 * ::sqrt (2);
        qreal wh = w * 0.5;
        qreal hh = h * 0.5;
        qreal radius = dh * value;
        qreal innerRadius = radius * innerRatio;
        qreal innerDistance = sqrt1_2 * innerRadius;
        qreal side = 2.0 * innerDistance;

        QPointF center (width() * 0.5, height() * 0.5);
        QRadialGradient gradient (center, radius, center);
        gradient.setColorAt (outerRatio, QColor (0, 0, 0, 255));
        gradient.setColorAt (innerRatio, QColor (0, 0, 0, 0));

        QBrush brush (gradient);
        QBrush blackbrush (QColor (0, 0, 0, 255));
        // Draw a transparent circle over the scene.
        p.setCompositionMode (QPainter::CompositionMode_SourceOver);
        if (radius < wh) {
            // If the spotlight radius is smaller than half the scene width,
            // draw black rectangles to its sides, which is faster.
            qreal diameter = radius * 2.0;
            p.fillRect (QRectF (x, y, 1.0 + wh - radius, h), blackbrush);
            p.fillRect (QRectF (x + wh + radius, y,
                            wh - radius, h), blackbrush);
            if (radius < hh) {
                // If the spotlight radius is smaller than half the scene
                // height, draw black rectangles to its top and bottom sides,
                // which is faster.
                p.fillRect (QRectF (x + wh - radius, y,
                            diameter, hh - radius + 1.0), blackbrush);
                p.fillRect (QRectF (x + wh - radius, y + hh + radius,
                            diameter, hh - radius), blackbrush);
        	
                // Draw the spotlight circle, but skip the transparent center.
                p.fillRect (QRectF (x + wh - radius, y + hh - radius,
                            radius - innerDistance, diameter), brush);
                p.fillRect (QRectF (x + wh + innerDistance, y + hh - radius,
                            radius - innerDistance, diameter), brush);
                p.fillRect (QRectF (x + wh - innerDistance, y + hh - radius,
                            side, radius - innerDistance), brush);
                p.fillRect (QRectF (x + wh - innerDistance,
                            y + hh + innerDistance,
                            side, radius - innerDistance), brush);
            }
            else {
                // Else draw the radial gradient
                p.fillRect (QRectF (x + wh - radius, y, diameter, h), brush);
            }
        }
        else {
            // If the spotlight is bigger than the scene, draw only the
            // sections where the gradient is not transparent.
            p.fillRect (QRectF (x, y, wh - innerDistance, h), brush);
            p.fillRect (QRectF (x + wh + innerDistance, y,
                            wh - innerDistance, h), brush);
            if (innerDistance < hh) {
                p.fillRect (QRectF (x + wh - innerDistance, y,
                            side, hh - innerDistance), brush);
                p.fillRect (QRectF (x + wh - innerDistance,
                            y + hh + innerDistance,
                            side, hh - innerDistance), brush);
            }
        }
    }

    p.end();
    m_spotLight->setPicture (picture);
}

KGameCanvasRectangle * KGrCanvas::drawRectangle (int x, int y, int w, int h)
{
    KGameCanvasRectangle * r =
                new KGameCanvasRectangle (colour, QSize (w, h), this);
    r->moveTo (x, y);
    r->show();
    return (r);
}

KGameCanvasPixmap * KGrCanvas::makeBorderElement(QList< QPixmap >frameTiles, 
                                                 int x, int y, int which)
{
    KGameCanvasPixmap *borderElement = new KGameCanvasPixmap (this);
    borderElement->setPixmap (frameTiles.at (which));
    borderElement->moveTo (x, y);
    borderElement->show();
    return borderElement;
}

KGameCanvasPixmap * KGrCanvas::makeDisplay (QList< QPixmap > tiles, int w)
{
    int sections = 1 + numSections(w, imgW); 
    int width = (sections + 2) * imgW;
    QPixmap pix (width, imgH);
    pix.fill(QColor(0, 0, 0, 0));
    QPainter p;
    p.begin (&pix);
    p.drawPixmap (0, 0, tiles.at(0));
    for (int x = imgW; x < sections * imgW; x += imgW) {
        p.drawPixmap (x, 0, tiles.at(1));
    }
    p.drawPixmap (sections * imgW, 0, tiles.at(2));
    
    p.end();
    KGameCanvasPixmap *element = new KGameCanvasPixmap (this);
    element->setPixmap (pix);
    element->show();
    return element;
}

QSize KGrCanvas::sizeHint() const
{
    return QSize ((nCellsW  + border) * imgW, (nCellsH + border) * imgH);
}

#include "kgrcanvas.moc"
// vi: set sw=4 et:
