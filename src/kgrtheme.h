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

#ifndef KGRTHEME_H
#define KGRTHEME_H

#include <QString>
#include <QColor>

#include <KSvgRenderer>

class KPixmapCache;

/**
 * KGrTheme handles KGoldrunner theme management.
 */
class KGrTheme
{
public:
    enum TileType {
        EmptyTile = 0,			// The empty tile must be tile 0.
	HiddenLadderTile,
	FalseBrickTile,
	BrickAnimation1Tile,		// The dug-brick tiles must come
	BrickAnimation2Tile,		// before any tiles that have variants.
	BrickAnimation3Tile,
	BrickAnimation4Tile,
	BrickAnimation5Tile,
	BrickAnimation6Tile,
	BrickAnimation7Tile,
	BrickAnimation8Tile,
	BrickAnimation9Tile,
	HeroTile,			// These two tiles are used in editing.
	EnemyTile,
	GoldTile,			// These five tiles can have variants.
	BarTile,
	LadderTile,
	ConcreteTile,
	BrickTile,
	TileTypeCount
    };

    /**
     * Default constructor.
     * \param[in] systemDataDir The system data dir is used to calculate the
     * theme data directory.
     */
    KGrTheme (const QString &systemDataDir);
    
    /**
     * Default dtor
     */
    ~KGrTheme();

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
     * Given the tile size, returns the list of themed frame tiles.
     * \param[in] size The tile size.
     * An empty list is returned when the theme does not provide a themed frame
     * area.
     */
    QList<QPixmap> frameTiles (unsigned int size);

    /**
     * Given the tile size, returns the list of themed display tiles
     * (Frame/decoration around score/level).
     * \param[in] size The tile size.
     * An empty list is returned when the theme does not provide a themed
     * display area.
     */
    QList<QPixmap> displayTiles (unsigned int size);
    
    /**
     * Is a border required for this theme?
     * \returns true if a border is required for this theme.
     */
    bool isBorderRequired() const { return themeDrawBorder; }
    
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
    // Each KSvgRenderer loads a single .svg file. multiple SVG files make a
    // single theme.
    KSvgRenderer svgSet;                //< Tiles in here
    KSvgRenderer svgActors;             //< Everything else?
    
    QColor m_borderColor, m_textColor;	// Border colours.

    QString themeDataDir;
    QString m_themeFilepath;
    short themeDrawBorder;

    QList< QPixmap > namedTiles (QList< QString > names, unsigned int size);
    
    // Thomi - 31/01/2008
    // This private method will load the given item from SVG, if it exists, and
    // will automatically handle inserting and removing files from the pixmap
    // cache.
    QPixmap loadGraphic(const QSize &size, const QString &strName, KSvgRenderer &Svg, double boundsAdjust=0.5);

    // utility method - create the pixCache pointer below given a theme file
    // path.
    void createPixCache();
    
    int numBackgrounds;

    int counts[TileTypeCount];
    int offsets[TileTypeCount];

    // Thomi Richards - 25/01/2008
    //
    // Use a KPixmapCache to store rendered pixmaps in, so we don't need to
    // render from SVG every time:
    KPixmapCache *pixCache;
};

#endif // KGRTHEME_H
