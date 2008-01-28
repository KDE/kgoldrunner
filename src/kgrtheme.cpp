/***************************************************************************
 *                       kgrtheme.cpp  -  description                      *
 *                           -------------------                           *
 *  begin                : Wed Jul 7 2007                                  *
 *  Copyright 2002 Marco Krüger <grisuji@gmx.de>                           *
 *  Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>                      *
 *  Copyright 2007 Luciano Montanaro <mikelima@cirulla.net>                *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgrtheme.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KDebug>
#include <QPainter>

KGrTheme::KGrTheme (const QString &systemDataDir) : 
        themeDataDir (systemDataDir + "../theme/"),
        m_themeFilepath (""), 
        tileGraphics (NONE),
        backgroundGraphics (NONE),
        runnerGraphics (NONE),
        numBackgrounds (0),
        useDirectPixmaps (false),
        pixCache ("kgoldrunner-pixmap-cache")
{
    KConfigGroup group (KGlobal::config(), "Debugging");
    char *val = getenv ("KGOLDRUNNER_USE_PIXMAPS");
    if (val) useDirectPixmaps = (atoi(val) > 0);
    
    // Initialize theme lookup table
    for (int i = 0; i < TileTypeCount; ++i) {
	offsets[i] = i;
	counts[i] = 1;
    }
    
    pixCache.setRemoveEntryStrategy(KPixmapCache::RemoveLeastRecentlyUsed);
}

bool KGrTheme::load (const QString& themeFilepath)
{
    kDebug() << "New Theme -" << themeFilepath;
    if (!m_themeFilepath.isEmpty() && (themeFilepath == m_themeFilepath)) {
        kDebug() << "NO CHANGE OF THEME ...";
        return true;					// No change of theme.
    }

    KConfig theme (themeFilepath, KConfig::SimpleConfig);	// Read graphics config.
    KConfigGroup group = theme.group ("KDEGameTheme");

    QString f = group.readEntry ("Set", "");
    if (f.endsWith (".svg") || f.endsWith (".svgz")) {
        // Load a SVG theme (KGoldrunner 3+ and KDE 4+).
        QString path = themeFilepath.left (themeFilepath.lastIndexOf ("/") + 1) + f;
        if (! path.isEmpty()) {
            svgSet.load (path);
            
            // The theme may have multiple backgrounds, called
            // background0...backgroundN or just one background, called
            // background0 or simply background.
            QString backgroundPattern ("background%1");
            numBackgrounds = 0;
            while (svgSet.elementExists (backgroundPattern.arg (numBackgrounds))) {
                ++numBackgrounds;
            }
            if (numBackgrounds == 0) {
                if (svgSet.elementExists ("background")) {
                    numBackgrounds = 1;
                }
            }
        }
        f = group.readEntry ("Actors", "default/actors.svg");
        if (f.endsWith (".svg") || f.endsWith (".svgz")) {
            QString path = themeFilepath.left (themeFilepath.lastIndexOf ("/") + 1) + f;
            if (!path.isEmpty()) {
                svgActors.load (path);
            }
            tileGraphics = SVG;
            backgroundGraphics = SVG;
            runnerGraphics = SVG;
        }
    }
    else {
        return false;		// Not SVG: old XPM themes no longer supported.
    }

    // Check if the theme asks us to draw a border and set the specified color.
    themeDrawBorder = group.readEntry ("DrawCanvasBorder", 0);

    // The border color (default black) is also used as the view's background
    // color, to soften the ugly look of empty rectangles during repainting.
    QString themeBorderColor = group.readEntry ("BorderColor", "#000000");
    if (! themeBorderColor.isEmpty()) {
        m_borderColor.setNamedColor (themeBorderColor);
    }

    // If specified, also set the title color.
    QString themeTextColor = group.readEntry ("TextColor", "");
    if (! themeTextColor.isEmpty()) {
        m_textColor.setNamedColor (themeTextColor);
    }

    // Save the user's selected theme in KDE's config-group data for the game.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("ThemeFilepath", themeFilepath);
    gameGroup.sync();			// Ensure that the entry goes to disk.
    m_themeFilepath = themeFilepath;
    return true;
}

// Helper function 
void renderBackground (QPainter &painter, QSvgRenderer &svgSet, int variant, int numBackgrounds)
{
    variant %= numBackgrounds;
    QString backgroundName = "background%1";
    kDebug() << "Trying to load background" << backgroundName.arg (variant);
    if (svgSet.elementExists (backgroundName.arg (variant))) 
        svgSet.render (&painter, backgroundName.arg (variant));
    else if (svgSet.elementExists ("background")) 
        svgSet.render (&painter, "background");
}

QPixmap KGrTheme::background (unsigned int width, unsigned int height, 
                              unsigned int variant)
{
    QTime t;
    t.restart();
    QPixmap pixmap;
    //for (int i = 0; i < 5; i++) {
    if ((width != 0) && (height != 0) && 
        (backgroundGraphics == SVG) && numBackgrounds > 0) {
        QPainter painter;
        if (useDirectPixmaps) {
            QPixmap backgroundPixmap (width, height);
            painter.begin (&backgroundPixmap);
            backgroundPixmap.fill (Qt::black);
            renderBackground (painter, svgSet, variant, numBackgrounds);
            painter.end();
            pixmap = backgroundPixmap;
        }
        else {
            QImage backgroundImage (width, height,
                                    QImage::Format_ARGB32_Premultiplied);
            backgroundImage.fill (0);
            painter.begin (&backgroundImage);
            renderBackground (painter, svgSet, variant, numBackgrounds);
            painter.end();
            pixmap = QPixmap::fromImage (backgroundImage);
        }
    }
    //}
    qDebug() << "background took" << t.elapsed() << "ms to render";
    return pixmap;
}

QList<QPixmap> KGrTheme::hero (unsigned int size)
{
    QList<QPixmap> frames;
    if (runnerGraphics == SVG) {
        frames << svgFrames ("hero_%1", size, 36);
    }
    return frames;
}

QList<QPixmap> KGrTheme::enemy (unsigned int size)
{
    QList<QPixmap> frames;
    if (runnerGraphics == SVG) {
        frames << svgFrames ("enemy_%1", size, 36);
        frames << svgFrames ("gold_enemy_%1", size, 36);
    }
    return frames;
}

QList<QPixmap> KGrTheme::svgFrames (const QString &elementPattern,
                                    unsigned int size, int nFrames)
{
    QPainter q;
    QList<QPixmap> frames;
    QRectF bounds (0, 0, size, size);
    bounds.adjust (-0.5, -0.5, 0.5, 0.5);

    QTime t;
    t.restart();

    for (int i = 1; i <= nFrames; i++) 
    {
        // This is the pixmap we're loading:
        QPixmap pix (size, size);
        // name of the image we're looking for e.g. "hero_1", "hero_2", etc.
        QString s = elementPattern.arg (i);
    
        // First, is it in the pixmap cache?
        // We need a string to store the pixmaps as. It must include the theme name,
        // graphics name, and size:
        QString strTagName = QString("%1|%2|%3").arg(m_themeFilepath).arg(s).arg(QString::number(size));
        
        kWarning() << "Tagname for this graphic is:" << strTagName;
        
        if (! pixCache.find (strTagName, pix))
        {
            kWarning() << "Element" << s << "not in cache, rendering from SVG";
            // no, it's not int he cache. Render it from SVG:
            // does it exist in the SVG set?
            if (svgActors.elementExists (s))
            {
                // are we using pixmaps?
                if (useDirectPixmaps)
                {
                    // yes - we're using pixmaps
                    pix.fill(QColor (0, 0, 0, 0));
                    q.begin (&pix);
                    svgActors.render (&q, s, bounds);
                    q.end();
                }
                else
                {
                    // no, we're using images
                    QImage img (size, size, QImage::Format_ARGB32_Premultiplied);
                    img.fill (0);
                    q.begin (&img);
                    svgActors.render (&q, s, bounds);
                    q.end();
                    pix = QPixmap::fromImage(img);
                }
                pixCache.insert(strTagName, pix);
            }
            else
            {
                kWarning() << "The element" << s << "Wasn't found in the current theme!";
            }
        }
        else
        {
            kWarning() << "Element" << s << "taken from pixmap Cache";
        }
        // at this point we have a pixmap:
        frames.append (pix);
    }
    
    qDebug() << "rendering frames took " << t.elapsed() << "ms";
    return frames;
}

QPixmap KGrTheme::svgTile (QImage & img, QPainter & q, const QString & name)
{
    q.begin (&img);
    img.fill (0);
    
    QRectF bounds = img.rect();
    bounds.adjust (-0.5, -0.5, 0.5, 0.5);
    if (svgSet.elementExists (name)) {
        svgSet.render (&q, name, bounds);
    }
    else if (svgActors.elementExists (name)) {
        svgActors.render (&q, name, bounds);
    }
    else {
        // The theme does not contain the needed element.
        kWarning() << "The needed element" << name << "is not in the theme.";
    }
    q.end();
    return QPixmap::fromImage (img);
}

QList<QPixmap> KGrTheme::tiles (unsigned int size)
{
    QList<QPixmap> list;
    if (tileGraphics == SVG) {
        // Draw SVG versions of nugget, bar, ladder, concrete and brick.
        QImage img (size, size, QImage::Format_ARGB32_Premultiplied);
        QPainter painter;

	// Create a list of rendered tiles. The tiles must be appended in the
	// same order they appear in the TileType enum.
	// While creating the tiles, count the variants, and fill the offset and count tables.
	
	QVector< QString > tileNames;
	tileNames << "empty" << "hidden_ladder" << "false_brick" << "hero_1" << "enemy_1";
	int i = 0;
	// These tiles come never have variants
	foreach (QString name, tileNames) {
	    list.append (svgTile (img, painter, name));
	    offsets[i] = i;
	    counts[i] = 1;
	    i++;
	}

	// These tiles can have variants
	tileNames.clear();
	tileNames << "gold" << "bar" << "ladder" << "concrete" << "brick";
	foreach (QString name, tileNames) {
	    int tileCount = 0;
	    QString tileNamePattern = name + "-%1";
	    while (svgSet.elementExists (tileNamePattern.arg (tileCount))) {
		kDebug() << tileNamePattern.arg(tileCount);
		list.append (svgTile (img, painter, tileNamePattern.arg(tileCount)));
		tileCount++;
	    }
	    if (tileCount > 0) {
		counts[i] = tileCount;
	    } else {
		list.append (svgTile (img, painter, name));
		counts[i] = 1;
	    } 
	    offsets[i] = offsets[i - 1] + counts[i - 1];
	    i++;
	}

	// Add SVG versions of blasted bricks.
	QString brickPattern("brick_%1");
	for (int j = 1; j <= 9; ++j) {
	    list.append (svgTile (img, painter, brickPattern.arg(j)));
	}
	offsets[i] = offsets[i - 1] + counts[i - 1];
	counts[i] = 9;
    }
    return list;
}

QList< QPixmap > KGrTheme::frameTiles (unsigned int size)
{
    QList< QPixmap > list;

    QImage img (size, size, QImage::Format_ARGB32_Premultiplied);
    QPainter painter;

    QVector< QString > tileNames;
    tileNames << "frame-topleft" << "frame-top" << "frame-topright" << 
	         "frame-left" << "frame-fill" << "frame-right" <<
	         "frame-bottomleft" << "frame-bottom" << "frame-bottomright";

    foreach (QString name, tileNames) {
	if (svgSet.elementExists (name)) {
            list.append (svgTile (img, painter, name));
            kDebug() << name << "found";
        }
        else {
            list.clear();
            kDebug() << name << "NOT found, exiting";
            break;
        }
    }

    kDebug() << "frameTiles() tiles:" << list.size();

    return list;
}

// vi: sw=4 et
