/***************************************************************************
                         kgrtheme.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
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

#ifndef KGRTHEME_H
#define KGRTHEME_H

#include <QString>
#include <QColor>
#include <QImage>
#include <KSvgRenderer>

/**
 * KGrTheme handles KGoldrunner theme management.
 */
class KGrTheme
{
public:
    enum TileType {
        EmptyTile = 0,
	HiddenLadderTile,
	FalseBrickTile,
	HeroTile,
	EnemyTile,
	GoldTile,
	BarTile,
	LadderTile,
	ConcreteTile,
	BrickTile,
	BrickAnimationTile,
	TileTypeCount
    };
    /**
     * Default constructor.
     * \param[in] systemDataDir The system data dir is used to calculate the theme data directory.
     */
    KGrTheme (const QString &systemDataDir);

    /** 
     * Load a theme given the name of its .desktop file.
     */
    bool load (const QString & themeFilepath);
    
    /**
     * Given the tile size, returns the list of themed tiles.
     * \param[in] size The tile size
     */
    QList<QPixmap> tiles (unsigned int size);
    
    /**
     * Given the tile size, returns the list of hero frames.
     * \param[in] size The tile size
     */
    QList<QPixmap> hero (unsigned int size);
    
    /**
     * Given the tile size, returns the list of enemy frames.
     * \param[in] size The tile size.
     */
    QList<QPixmap> enemy (unsigned int size);
    
    /**
     * Prepares an image with the background, given the desired width and
     * height and background variant.
     * \param[in] variant The background variant you wish to use. Valid values
     * are between 0 and backgroundCount().
     * \param[in] width The desired width of the background.
     * \param[in] height The desired height of the background.
     */
    QPixmap background (unsigned int width, unsigned int height,
                        unsigned int variant);
    
    /**
     * Is a border required for this theme?
     * \returns true if a border is required for this theme.
     */
    bool isBorderRequired() const { return themeDrawBorder; }
    
    /**
     * Find out if the theme has a background picture.
     */
    bool isWithBackground() const { return numBackgrounds > 0; }

    /**
     * Find out if the theme has a multiple background pictures.
     */
    bool multipleBackgrounds() const { return numBackgrounds > 1; }
    
    /**
     * Return the number of background variants in the theme.
     */
    int backgroundCount() { return numBackgrounds; }

    int tileCount(TileType t) { return counts[t]; }
    int firstTile(TileType t) { return offsets[t]; }

    /**
     * Obtain the theme defined border color.
     */
    QColor borderColor() { return m_borderColor; }
    
    /**
     * Obtain the theme defined text color.
     */
    QColor textColor() { return m_textColor; }

private:
    KSvgRenderer svgSet;
    KSvgRenderer svgActors;
    QColor m_borderColor, m_textColor;	// Border colours.

    QString themeDataDir;
    QString m_themeFilepath;
    short themeDrawBorder;
    QPixmap svgTile (QImage &image, QPainter &painter, const QString &name);
    QList<QPixmap> svgFrames (const QString & elementPattern,
                                unsigned int size, int nFrames);

    enum GraphicsType { NONE, SVG, PNG };
    GraphicsType tileGraphics;
    GraphicsType backgroundGraphics;
    GraphicsType runnerGraphics;
    int numBackgrounds;
    bool hasPanelTiles;
    bool useDirectPixmaps;

    int counts[TileTypeCount];
    int offsets[TileTypeCount];
};

#endif // KGRTHEME_H
