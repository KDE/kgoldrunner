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
#include <QFileInfo>

KGrTheme::KGrTheme (const QString &systemDataDir) : 
        themeDataDir (systemDataDir + "../theme/"),
        m_themeFilepath (""), 
        numBackgrounds (0),
        pixCache (NULL)
{
    KConfigGroup group (KGlobal::config(), "Debugging");
    
    // Initialize theme lookup table
    for (int i = 0; i < TileTypeCount; ++i) {
	offsets[i] = i;
	counts[i] = 1;
    }
}

KGrTheme::~ KGrTheme()
{
    delete pixCache;
    pixCache = NULL;
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
        if (f.endsWith (".svg") || f.endsWith (".svgz")) 
        {
            QString path = themeFilepath.left (themeFilepath.lastIndexOf ("/") + 1) + f;
            if (!path.isEmpty()) 
            {
                svgActors.load (path);
            }
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
    
    createPixCache();
    
    return true;
}


QPixmap KGrTheme::background (unsigned int width, unsigned int height, 
                              unsigned int variant)
{
    variant %= numBackgrounds;
    QTime t;
    t.restart();
    QPixmap pixmap;
    //for (int i = 0; i < 5; i++) {
    if ((width != 0) && (height != 0) && (numBackgrounds > 0))
    {
        if (svgSet.elementExists(QString("background%1").arg(variant)))
        {
            pixmap = loadGraphic(QSize(width, height), QString("background%1").arg(variant),svgSet);
        }
        else if (svgSet.elementExists("background"))
        {
            pixmap = loadGraphic(QSize(width, height), "background",svgSet);
        }

    }
    //}
    qDebug() << "background took" << t.elapsed() << "ms to render";
    return pixmap;
}

QList<QPixmap> KGrTheme::hero (unsigned int size)
{
    QList<QPixmap> frames;

    for (int i = 1; i <= 36; i++)
    {
        frames << loadGraphic(QSize(size, size), QString("hero_%1").arg(i), svgActors);
    }

    return frames;
}

QList<QPixmap> KGrTheme::enemy (unsigned int size)
{
    QList<QPixmap> frames;
    for (int i = 1; i <= 36; i++)
    {
        frames << loadGraphic(QSize(size, size), QString("enemy_%1").arg(i), svgActors);
    }
    for (int i = 1; i <= 36; i++)
    {
        frames << loadGraphic(QSize(size, size), QString("gold_enemy_%1").arg(i), svgActors);
    }

    return frames;
}

QList<QPixmap> KGrTheme::svgFrames (const QString &elementPattern,
                                    unsigned int size, int nFrames)
{
    QList<QPixmap> frames;

    QTime t;
    t.restart();

    for (int i = 1; i <= nFrames; i++) 
    {
        // This is the pixmap we're loading:
        frames.append (loadGraphic(QSize(size, size), elementPattern.arg(i), svgActors));
    }
    
    qDebug() << "rendering frames took " << t.elapsed() << "ms";
    return frames;
}


QList<QPixmap> KGrTheme::tiles (unsigned int size)
{
    QList<QPixmap> list;

    // Create a list of rendered tiles. The tiles must be appended in the
    // same order they appear in the TileType enum.
    // While creating the tiles, count the variants, and fill the offset and count tables.
    
    QVector< QString > tileNames;
    tileNames << "empty" << "hidden_ladder" << "false_brick" << "hero_1" << "enemy_1";
    int i = 0;
    // These tiles come never have variants
    foreach (QString name, tileNames) {
        list.append (loadGraphic(QSize(size, size), name, svgSet));
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
            list.append (loadGraphic( QSize(size, size), tileNamePattern.arg(tileCount), svgSet));
            tileCount++;
        }
        if (tileCount > 0) {
            counts[i] = tileCount;
        } else {
            list.append (loadGraphic(QSize(size, size), name, svgSet));
            counts[i] = 1;
        } 
        offsets[i] = offsets[i - 1] + counts[i - 1];
        i++;
    }

    // Add SVG versions of blasted bricks.
    QString brickPattern("brick_%1");
    for (int j = 1; j <= 9; ++j) {
        list.append (loadGraphic(QSize(size, size), brickPattern.arg(j), svgSet));
    }
    offsets[i] = offsets[i - 1] + counts[i - 1];
    counts[i] = 9;
    
    return list;
}

QList< QPixmap > KGrTheme::frameTiles (unsigned int size)
{
    QList< QPixmap > list;
    QVector< QString > tileNames;
    
    tileNames << "frame-topleft" << "frame-top" << "frame-topright" << 
	         "frame-left" << "frame-fill" << "frame-right" <<
	         "frame-bottomleft" << "frame-bottom" << "frame-bottomright";

    foreach (QString name, tileNames) {
	if (svgSet.elementExists (name)) {
            list.append (loadGraphic(QSize(size, size), name, svgSet));
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

QPixmap KGrTheme::loadGraphic(const QSize & size, const QString & strName, KSvgRenderer &Svg, double boundsAdjust)
{
    if (! pixCache)
    {
        kWarning() << "Cannot load graphics until pixmap cache object has been created!";
        return QPixmap();
    }
    QPixmap pix(size);
    
    // create tag name:
    QString strTagName = QString("%1|%2|%3x%4").arg(m_themeFilepath).arg(strName).arg(size.width()).arg(size.height());
    
    if (! pixCache->find(strTagName, pix))
    {
//        kWarning() << "Element" << strName << "Not in cache, rendering from SVG";
        if (! Svg.elementExists(strName))
        {
            kWarning() << "Element" << strName << "Not found in SVG document - unable to load!";
            return pix;
        }
        else
        {
            pix.fill(QColor(0,0,0,0));
            QPainter p;
            p.begin(&pix);
            QRectF bounds(0,0, size.width(), size.height());
            bounds.adjust(-boundsAdjust, -boundsAdjust, boundsAdjust, boundsAdjust);
            Svg.render(&p, strName, bounds);
            p.end();
            pixCache->insert(strTagName, pix);
        }
        
    }
    return pix;
}

void KGrTheme::createPixCache()
{
    delete pixCache;
    pixCache = NULL;
    
    QString strCacheName = m_themeFilepath.mid(m_themeFilepath.lastIndexOf('/') + 1);
    strCacheName = strCacheName.left(strCacheName.indexOf('.'));
    kWarning() << strCacheName;
    
    pixCache = new KPixmapCache(QString("kgoldrunner-pixmap-cache-") + strCacheName);
    pixCache->setRemoveEntryStrategy(KPixmapCache::RemoveLeastRecentlyUsed);
    pixCache->setCacheLimit(1024 * 3);  // set cache size to 3 MB PER THEME
    
    // Check the file modification time of the theme. If it is newer than the pixmap cache
    // timestamp then we invalidate the entire cache for this theme only.
    QFileInfo fi(m_themeFilepath);
    if (fi.lastModified().toTime_t() != pixCache->timestamp())
    {
        kWarning() << "Pixmap cache for theme '" << strCacheName << "' is outdated; invalidating cache.";
        pixCache->discard();
        pixCache->setTimestamp(fi.lastModified().toTime_t());
    }
}

