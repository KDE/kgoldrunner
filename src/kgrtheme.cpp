#include "kgrdebug.h"
#include <stdio.h>

/****************************************************************************
 *    Copyright 2007  Luciano Montanaro <mikelima@cirulla.net>              *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
 *                                                                          *
 *    This program is free software; you can redistribute it and/or         *
 *    modify it under the terms of the GNU General Public License as        *
 *    published by the Free Software Foundation; either version 2 of        *
 *    the License, or (at your option) any later version.                   *
 *                                                                          *
 *    This program is distributed in the hope that it will be useful,       *
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *    GNU General Public License for more details.                          *
 *                                                                          *
 *    You should have received a copy of the GNU General Public License     *
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ****************************************************************************/

#include "kgrtheme.h"

#include <QDebug>
#include <QPainter>
#include <QFileInfo>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KPixmapCache>
#include <KDebug>

KGrTheme::KGrTheme()
        : 
        svgLoaded (false),
        m_themeFilepath (""), 
        svgSetFilepath (""),
        svgActorsFilepath (""),
        numBackgrounds (0),
        pixCache (0)
{
    dbgLevel = 0;

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
    pixCache = 0;
    kDebug() << "Theme and pixmap cache DELETED.";
}


bool KGrTheme::load (const QString& themeFilepath)
{
    kDebug() << "New Theme -" << themeFilepath;
    if (!m_themeFilepath.isEmpty() && (themeFilepath == m_themeFilepath)) {
        kDebug() << "NO CHANGE OF THEME ...";
        return true;					// No change of theme.
    }

    KConfig theme (themeFilepath, KConfig::SimpleConfig);
    KConfigGroup group = theme.group ("KGameTheme");	// Get graphics config.

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

    kDebug() << "Border" << themeDrawBorder << "Color" << themeBorderColor;

    m_themeFilepath   = themeFilepath;

    int filePos       = themeFilepath.lastIndexOf ("/") + 1;
    QString dirName   = themeFilepath.left (filePos);
    themeName         = themeFilepath.mid (filePos);
    themeName         = themeName.left (themeName.indexOf ("."));

    QString filepath  = group.readEntry ("Set", "");
    svgSetFilepath    = dirName + filepath;

    filepath          = group.readEntry ("Actors", "default/actors.svg");
    svgActorsFilepath = dirName + filepath;

    // Force the cache to reload SVG when it cannot find a pixmap in the cache.
    svgLoaded = false;

    // If theme NEVER loaded or cache outdated, load SVG (we need tile-counts).
    KConfigGroup themeGroup (KGlobal::config(), "Theme_" + themeName);
    bool loadedBefore = themeGroup.readEntry ("LoadedBefore", false);
    bool cacheOK = createPixCache();
    if (! (loadedBefore && cacheOK)) {
        kDebug() << "Calling loadSvg(), cacheOK:" << cacheOK
                 << "loadedBefore:" << loadedBefore;
	kDebug() << "SVG files" << svgSetFilepath << svgActorsFilepath;
        if (! (loadSvg())) {
            return false;
        }
    }
    numBackgrounds = themeGroup.readEntry ("numBackgrounds", -1);

    // Save the user's selected theme in KDE's config-group data for the game.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("ThemeFilepath", themeFilepath);
    gameGroup.sync();			// Ensure that the entry goes to disk.

    return true;
}


QPixmap KGrTheme::background (unsigned int width, unsigned int height, 
                              unsigned int variant)
{
    QTime t;
    t.restart();
    QPixmap pixmap;
    if ((width != 0) && (height != 0)) {
        if (numBackgrounds <= 0) {	// No variants, maybe not even a b/g.
            loadPixmap (QSize (width, height),
                        QString ("background"), pixmap, Set);
            dbe1 "loadPixmap: \"background\" %dx%d\n", width, height);
        }
        else {
            variant %= numBackgrounds;	// Cycle through the variants of b/g.
            loadPixmap (QSize (width, height),
                          QString ("background%1").arg (variant), pixmap, Set);
            dbe1 "loadPixmap: \"background%d\" %dx%d\n", variant, width, height);
        }
    }
    qDebug() << "Background took" << t.elapsed() << "ms to render";
    return pixmap;
}

QList<QPixmap> KGrTheme::hero (unsigned int size)
{
    QList<QPixmap> frames;

    for (int i = 1; i <= 36; i++) {
        QPixmap pix;
        loadPixmap (QSize (size, size), QString ("hero_%1").arg (i), pix,
                    Actors);
        frames << pix;
    }

    return frames;
}

QList<QPixmap> KGrTheme::enemy (unsigned int size)
{
    QList<QPixmap> frames;

    for (int i = 1; i <= 36; i++) {
        QPixmap pix;
        loadPixmap (QSize (size, size), QString ("enemy_%1").arg (i), pix,
                    Actors);
        frames << pix;
    }
    for (int i = 1; i <= 36; i++) {
        QPixmap pix;
        loadPixmap (QSize (size, size), QString ("gold_enemy_%1").arg (i), pix,
                    Actors);
        frames << pix;
    }

    return frames;
}

QList<QPixmap> KGrTheme::tiles (unsigned int size)
{
    QList<QPixmap> list;

    // Create a list of rendered tiles. The tiles must be appended in the same
    // order as they appear in the TileType enum. The empty tile must be first
    // (tile 0) and the dug bricks must appear before any tiles that can have
    // several variants, to avoid a crash when digging bricks then changing
    // to a theme that has different numbers of variants.

    // While creating the tiles, count the variants and fill the offset[]
    // and count[] tables.
    
    QVector< QString > tileNames;
    int i = 0;

    // These tiles can never have variants
    tileNames << "empty" << "hidden_ladder" << "false_brick";
    foreach (const QString &name, tileNames) {
        QPixmap pix;
        loadPixmap (QSize (size, size), name, pix, Set);
        list.append (pix);
        offsets[i] = i;
        counts[i] = 1;
        i++;
    }

    // Add SVG versions of dug bricks.
    QString brickPattern("brick_%1");
    for (int j = 1; j <= 9; ++j) {
        QPixmap pix;
        loadPixmap (QSize (size, size), brickPattern.arg (j), pix, Set);
        list.append (pix);
        offsets[i] = i;
        counts[i] = 1;
        i++;
    }
    
    // These tiles, used in the game-editor, come from the Actors SVG file
    tileNames.clear();
    tileNames << "hero_1" << "enemy_1";
    foreach (const QString &name, tileNames) {
        QPixmap pix;
        loadPixmap (QSize (size, size), name, pix, Actors);
        list.append (pix);
        offsets[i] = i;
        counts[i] = 1;
        i++;
    }

    // These tiles can have variants
    KConfigGroup themeGroup (KGlobal::config(), "Theme_" + themeName);
    tileNames.clear();
    tileNames << "gold" << "bar" << "ladder" << "concrete" << "brick";
    foreach (const QString & name, tileNames) {
        int tileCount = themeGroup.readEntry (name + "Types", -1);
        kDebug() << name << "tileCount" << tileCount;
        QString pattern = name + "-%1";
        QPixmap pix;
        if (tileCount <= 0) {		// No variants, maybe not even a tile.
            loadPixmap (QSize (size, size), name, pix, Set);
            list.append (pix);
            tileCount = 1;
        }
        else {				// Load variants 0 to (tileCount -1).
            for (int v = 0; v < tileCount; v++) {
                loadPixmap (QSize (size, size), pattern.arg (v), pix, Set);
                list.append (pix);
            }
        }
        counts[i]  = tileCount;
        offsets[i] = offsets[i - 1] + counts[i - 1];
        i++;
    }

    kDebug() << "Total tiles:" << list.size();
    return list;
}

QList< QPixmap > KGrTheme::namedTiles (QStringList names, unsigned int size)
{
    QList< QPixmap > list;
    QPixmap pix;

    foreach (const QString &name, names) {
        if (loadPixmap (QSize (size, size), name, pix, Set)) {
            qDebug() << name;
            list.append (pix);
        }
        else {
            list.clear();
            kDebug() << name << "NOT found, exiting";
            break;
        }
    }

    kDebug() << "namedTiles() tiles:" << list.size();

    return list;
}

QList< QPixmap > KGrTheme::displayTiles (unsigned int size)
{
    QStringList tileNames;
    KConfigGroup themeGroup (KGlobal::config(), "Theme_" + themeName);
    tileNames = themeGroup.readEntry ("DisplayTiles", QStringList());

    return namedTiles (tileNames, size);
}

QList< QPixmap > KGrTheme::frameTiles (unsigned int size)
{
    QStringList tileNames;
    KConfigGroup themeGroup (KGlobal::config(), "Theme_" + themeName);
    qDebug() << themeName;
    tileNames = themeGroup.readEntry ("BorderTiles", QStringList());

    return namedTiles (tileNames, size);
}

bool KGrTheme::loadSvg()
{
    QTime t;
    t.restart();
    // Load a SVG theme.
    if (svgSetFilepath.isEmpty() || svgActorsFilepath.isEmpty()) {
        return false;
    }
    if (! (svgSetFilepath.endsWith (".svg") ||
           svgSetFilepath.endsWith (".svgz"))) {
        return false;
    }
    if (! (svgActorsFilepath.endsWith (".svg") ||
           svgActorsFilepath.endsWith (".svgz"))) {
        return false;
    }

    svgActors.load (svgActorsFilepath);

    svgSet.load (svgSetFilepath);

    // Use local KConfig to record the tiles and backgrounds this theme has.
    KConfigGroup themeGroup (KGlobal::config(), "Theme_" + themeName);

    // The theme may have multiple backgrounds, called "background0" to
    // "backgroundN" or just one background, called "background0" or
    // simply "background".

    QString bgPattern ("background%1");
    numBackgrounds = 0;
    while (svgSet.elementExists (bgPattern.arg (numBackgrounds))) {
        ++numBackgrounds;
    }
    if (numBackgrounds == 0) {
        if (! (svgSet.elementExists ("background"))) {
            numBackgrounds = -1;	// There is no background at all.
        }
    }
    // Save the number of backgrounds, for use in fast loading from the cache.
    themeGroup.writeEntry ("numBackgrounds", numBackgrounds);

    // Some tiles can have variants called "name-0" to "name-N" or they may have
    // just one version named "name".
    QStringList tileNames;
    tileNames << "gold" << "bar" << "ladder" << "concrete" << "brick";
    foreach (const QString & name, tileNames) {
        int tileCount = 0;
        QString tileNamePattern = name + "-%1";
        while (svgSet.elementExists (tileNamePattern.arg (tileCount))) {
            tileCount++;
        }
        if (tileCount == 0) {
            if (! (svgSet.elementExists (name))) {
                tileCount = -1;
            }
        }
        // Save the number of types, for use in fast loading from the cache.
        themeGroup.writeEntry (name + "Types", tileCount);
    }

    QStringList tilesFound;
    tileNames.clear();
    tileNames << "frame-topleft" << "frame-top" << "frame-topright" << 
	         "frame-left" << "frame-fill" << "frame-right" <<
	         "frame-bottomleft" << "frame-bottom" << "frame-bottomright";
    foreach (const QString & name, tileNames) {
	if (svgSet.elementExists (name)) {
            tilesFound.append (name);
        }
    }
    // Saves looking for missing tiles in the cache and then redoing loadSvg().
    themeGroup.writeEntry ("BorderTiles", tilesFound);

    tilesFound.clear();
    tileNames.clear();
    tileNames << "display-left" << "display-centre" << "display-right";
    foreach (const QString & name, tileNames) {
	if (svgSet.elementExists (name)) {
            tilesFound.append (name);
        }
    }
    // Saves looking for missing tiles in the cache and then redoing loadSvg().
    themeGroup.writeEntry ("DisplayTiles", tilesFound);

    svgLoaded = true;
    themeGroup.writeEntry ("LoadedBefore", true);
    themeGroup.sync();			// Ensure that the entries go to disk.
    qDebug() << "SVG took" << t.elapsed() << "ms to load";
    return true;
}

bool KGrTheme::loadPixmap (const QSize & size, const QString & strName,
                           QPixmap & pix, SvgSource source, double boundsAdjust)
{
    if (! pixCache) {
        kWarning() << "Cannot load pixmaps until the pixmap cache object has been created!";
        pix = QPixmap();
        return false;
    }

    // Calculate the key for this pixmap in the cache.
    QString key = QString("%1|%2|%3x%4")
                          .arg (m_themeFilepath).arg (strName)
                          .arg (size.width()).arg (size.height());
    
    if (! pixCache->find (key, pix)) {
        // kWarning() << "Element" << strName
                   // << "Not in cache, rendering from SVG";

        // If theme not loaded, go and load it.
        if (! svgLoaded) {
            kDebug() << "Calling loadSvg(), theme:" << themeName
                     << "key" << key;
            if (! (loadSvg())) {
                return false;
            }
        }

        // If required pixmap not in cache at right size, render and cache it.
        bool exists = false;
        if (source == Set) {
            exists = svgSet.elementExists (strName);
        }
        else if (source == Actors) {
            exists = svgActors.elementExists (strName);
        }
        if (! exists) {
            kWarning() << "Element" << strName
                       << "Not found in SVG document - unable to load!";
            return false;
        }
        else {
            pix = QPixmap (size);
            pix.fill (QColor (0, 0, 0, 0));
            QPainter p;
            p.begin(&pix);
            QRectF bounds (0, 0, size.width(), size.height());
            bounds.adjust (-boundsAdjust, -boundsAdjust,
                           +boundsAdjust, +boundsAdjust);
            if (source == Set) {
                svgSet.render (&p, strName, bounds);
            }
            else if (source == Actors) {
                svgActors.render (&p, strName, bounds);
            }
            p.end();
            pixCache->insert (key, pix);
            dbe1 "Set cache %s, %s size %dx%d\n", themeName.toUtf8().constData(),
                strName.toUtf8().constData(), size.width(), size.height());
        }
    }
    return true;
}

bool KGrTheme::createPixCache()
{
    delete pixCache;
    pixCache = 0;

    kDebug() << "Create cache:"
             << (QString("kgoldrunner-pixmap-cache-") + themeName);
    pixCache = new KPixmapCache
                    (QString("kgoldrunner-pixmap-cache-") + themeName);
    pixCache->setRemoveEntryStrategy (KPixmapCache::RemoveLeastRecentlyUsed);
    pixCache->setCacheLimit (1024 * 3);  // Set cache size to 3 MB per theme.
    
    // Check the file modification times of the theme's SVG files.  If the most
    // recent does not agree with the pixmap cache's timestamp, invalidate the
    // entire cache (for this theme only), thus forcing load and render of SVG.

    QFileInfo fInfo   (svgSetFilepath);
    uint setTime    = fInfo.lastModified().toTime_t();
    fInfo.setFile     (svgActorsFilepath);
    uint actorsTime = fInfo.lastModified().toTime_t();

    uint lastChange = qMax(setTime, actorsTime);
    dbe1 "Timestamps for Set %d, Actors %d,\n"
        "       Last Change %d,  Cache %d\n",
        setTime, actorsTime, lastChange, pixCache->timestamp());
    if (lastChange != pixCache->timestamp()) {
        kWarning() << "Pixmap cache for theme '" << themeName
                   << "' is outdated; invalidating cache.";
        kDebug() << "Theme:" << themeName
                 << "files:" << svgSetFilepath << svgActorsFilepath;
        pixCache->discard();
        pixCache->setTimestamp (lastChange);
        return false;
    }
    return true;
}
