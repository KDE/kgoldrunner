/***************************************************************************
                         kgrtheme.cpp  -  description
                             -------------------
    begin                : Wed Jul 7 2007
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>
    Copyright 2007 Luciano Montanaro <mikelima@cirulla.net>
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
#include <QPainter>

KGrTheme::KGrTheme(const QString &systemDataDir) : 
	themeDataDir(systemDataDir + "../theme/"),
	m_themeFilepath(""), 
	tileGraphics(NONE),
	backgroundGraphics(NONE),
	runnerGraphics(NONE),
	numBackgrounds(0),
	hasPanelTiles(false)
{
}

bool KGrTheme::load(const QString& themeFilepath)
{
    kDebug() << "New Theme -" << themeFilepath;
    if (!m_themeFilepath.isEmpty() && (themeFilepath == m_themeFilepath)) {
	kDebug() << "NO CHANGE OF THEME ...";
	return true;					// No change of theme.
    }

    KConfig theme (themeFilepath, KConfig::OnlyLocal);	// Read graphics config.
    KConfigGroup group = theme.group ("KDEGameTheme");

    QString f = group.readEntry ("Set", "");
    if (f.endsWith (".svg") || f.endsWith (".svgz")) {
	// Load a SVG theme (KGoldrunner 3+ and KDE 4+).
	QString path = themeFilepath.left(themeFilepath.lastIndexOf("/") + 1) + f;
	if (!path.isEmpty()) {
	    svgSet.load(path);
	    
	    // The theme may have multiple backgrounds, called
	    // background0...backgroundN or just one background, called
	    // background0 or simply background.
	    QString backgroundPattern("background%1");
	    numBackgrounds = 0;
	    while (svgSet.elementExists(backgroundPattern.arg(numBackgrounds))) {
		++numBackgrounds;
	    }
	    if (numBackgrounds == 0) {
		if (svgSet.elementExists("background")) {
		    numBackgrounds = 1;
		}
	    }
	    if (svgSet.elementExists("panel_1")) {
		hasPanelTiles = true;
	    } else {
		hasPanelTiles = false;
	    }
	}
	f = group.readEntry ("Actors", "default/actors.svg");
	if (f.endsWith (".svg") || f.endsWith (".svgz")) {
	    QString path = themeFilepath.left(themeFilepath.lastIndexOf("/") + 1) + f;
	    if (!path.isEmpty()) {
		svgActors.load(path);
	    }
	    tileGraphics = SVG;
	    backgroundGraphics = SVG;
	    runnerGraphics = SVG;
	}
    } else {
	return false;		// Not SVG: old XPM themes no longer supported.
    }

    // Check if the theme asks us to draw a border and set the specified color.
    themeDrawBorder = group.readEntry ("DrawCanvasBorder", 0);
    if (themeDrawBorder) {
      QString themeBorderColor = group.readEntry ("BorderColor", "");
      if (!themeBorderColor.isEmpty()) {
        m_borderColor.setNamedColor(themeBorderColor);
      }
    }
    // If specified, also set the title color.
    QString themeTextColor = group.readEntry ("TextColor", "");
    if (!themeTextColor.isEmpty()) {
      m_textColor.setNamedColor(themeTextColor);
    }

    // Save the user's selected theme in KDE's config-group data for the game.
    KConfigGroup gameGroup (KGlobal::config(), "KDEGame");
    gameGroup.writeEntry ("ThemeFilepath", themeFilepath);
    gameGroup.sync();			// Ensure that the entry goes to disk.
    m_themeFilepath = themeFilepath;
    return true;
}

QImage KGrTheme::background(unsigned int width, unsigned int height,
				unsigned int variant)
{
    if ((width != 0) && (height != 0) && 
	    (backgroundGraphics == SVG) && numBackgrounds > 0) {
	QImage background(width, height, QImage::Format_ARGB32_Premultiplied);
	background.fill(0);
	QPainter painter(&background);
	variant %= numBackgrounds;
	QString backgroundName = "background%1";
	kDebug() << "Trying to load background" << backgroundName.arg(variant);
	if (svgSet.elementExists(backgroundName.arg(variant))) 
	    svgSet.render(&painter, backgroundName.arg(variant));
	else if (svgSet.elementExists("background")) 
	    svgSet.render(&painter, "background");
	return background;
    }
    return QImage();
}

QList<QPixmap> KGrTheme::hero(unsigned int size)
{
    QList<QPixmap> frames;
    if (runnerGraphics == SVG) {
	frames << svgFrames("hero_%1", size, 36);
    }
    return frames;
}

QList<QPixmap> KGrTheme::enemy(unsigned int size)
{
    QList<QPixmap> frames;
    if (runnerGraphics == SVG) {
	frames << svgFrames("enemy_%1", size, 36);
	frames << svgFrames("gold_enemy_%1", size, 36);
    }
    return frames;
}

QList<QPixmap> KGrTheme::svgFrames (const QString &elementPattern,
					unsigned int size, int nFrames)
{
    QImage img (size, size, QImage::Format_ARGB32_Premultiplied);
    QRectF bounds = img.rect();
    QPainter q (&img);
    bounds.adjust(-0.5, -0.5, 0.5, 0.5);
    QList<QPixmap> frames;
    for (int i = 1; i <= nFrames; i++) {
	QString s = elementPattern.arg(i);	// e.g. "hero_1", "hero_2", etc.
	img.fill (0);
	    if (svgActors.elementExists(s)) {
	    svgActors.render (&q, s, bounds);
	}else {
	    // The theme does not contain the needed element.
	    kWarning() << "The needed element" << s << "is not in the theme.";
	}
	frames.append (QPixmap::fromImage (img));
    }
    return frames;
}

QPixmap KGrTheme::svgTile (QImage & img, QPainter & q, const QString & name)
{
    img.fill (0);
    
    QRectF bounds = img.rect();
    bounds.adjust(-0.5, -0.5, 0.5, 0.5);
    if (svgSet.elementExists(name)) {
	svgSet.render (&q, name, bounds);
    } else if (svgActors.elementExists(name)) {
	svgActors.render(&q, name, bounds);
    } else {
	// The theme does not contain the needed element.
	kWarning() << "The needed element" << name << "is not in the theme.";
    }
    return QPixmap::fromImage (img);
}

QList<QPixmap> KGrTheme::tiles(unsigned int size)
{
    QList<QPixmap> list;
    if (tileGraphics == SVG) {
	// Draw SVG versions of nugget, bar, ladder, concrete and brick.
	QImage img (size, size, QImage::Format_ARGB32_Premultiplied);
	QPainter painter (&img);

	list.append(svgTile(img, painter, "empty"));
	list.append(svgTile(img, painter, "gold"));
	list.append(svgTile(img, painter, "bar"));
	list.append(svgTile(img, painter, "ladder"));
	list.append(svgTile(img, painter, "hidden_ladder"));
	list.append(svgTile(img, painter, "hero_1"));	// For use in edit mode.
	list.append(svgTile(img, painter, "enemy_1"));	// For use in edit mode.
	list.append(svgTile(img, painter, "concrete"));
	list.append(svgTile(img, painter, "false_brick"));
	list.append(svgTile(img, painter, "brick"));

	// Add SVG versions of blasted bricks.
	QString brickPattern("brick_%1");
	for (int i = 1; i <= 9; ++i) {
	    list.append(svgTile(img, painter, brickPattern.arg(i)));
	}
    }
    return list;
}

// vi: sw=4
