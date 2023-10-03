/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2012 Roney Gomes <roney477@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRRENDERER_H
#define KGRRENDERER_H

#include <QObject>
#include <QString>
#include <KGameRenderer>

#include "kgrsprite.h"

class KGrScene;
class KGameThemeProvider;
class KGameThemeSelector;
class KGameRenderedItem;

/* @short A class to assist theme-handling and rendering in KGoldrunner.
 *
 * KGoldrunner has two SVG files for each theme: one to hold the Actors (hero
 * and enemies) and one to hold the Set (bricks, ladders, background, etc.).
 *
 * The files are marked with the keywords "Actors" and "Set" in each theme's
 * .desktop file, rather than the usual "Filename" keyword. There are two
 * KGameThemeProvider objects and two KGameRenderer objects, each with its own
 * set of SVG files and KGameTheme objects.
 *
 * There is one KGameThemeSelector object, which selects the "Set" theme and uses
 * the "Set" KGameThemeProvider. Its currentThemeChanged signal is connected to a
 * currentThemeChanged slot in KGrRenderer, which finds the KGameTheme for the
 * corresponding "Actors" theme and SVG file.
 *
 * KGoldrunner also has several different usages of the KGameRenderer concepts
 * of "frameSuffix" and "frameBaseIndex". For animation frames (hero, enemies
 * and dug bricks), it always has frameSuffix = "_%1" and frameBaseIndex = 1, so
 * animation will be handled normally by KGameRenderer. 
 *
 * Depending on the theme and the artist's choices, backgrounds and tiles can
 * have one or more variants, to add variety to the look of brick walls, etc.
 * The suffixes used can be "-%1" or just "%1" and the frameBaseIndex = 0, or
 * there can be just one variant, with no suffix. The keyTable structure and the
 * getPixmapKey() and getBackgroundKey() methods of KGrRenderer provide ways to
 * go from KGoldrunner's internal tile-types to SVG element names that can be
 * used as pixmap keys in KGameRenderer.
 */
class KGrRenderer : public QObject
{
    Q_OBJECT
public:
    explicit KGrRenderer (KGrScene * scene);
    ~KGrRenderer() override;

    /*
     * Get a pointer to the KGameRenderer for "Set" graphics (bricks, etc.).
     */
    KGameRenderer * getSetRenderer()    { return m_setRenderer; }

    /*
     * Get a pointer to the KGameRenderer for "Actors" graphics (hero, etc.).
     */
    KGameRenderer * getActorsRenderer() { return m_actorsRenderer; }

    /*
     * Create the QGraphicsScene item for a tile of a particular type (e.g. bar,
     * gold, concrete, etc.) at a place in the on-screen KGoldrunner grid.
     *
     * @param picType     The internal KGoldrunner type of the required tile. If
     *                    FREE, just delete the previous tile (if any).
     * @param currentTile The pre-existing tile that is to be replaced or
     *                    deleted, or zero if the place is empty.
     */
    KGameRenderedItem * getTileItem (const char picType,
                                     KGameRenderedItem * currentTile);

    /*
     * Create the QGraphicsScene item for the background corresponding to the
     * current level.
     *
     * @param level             The current level in a KGoldrunner game.
     * @param currentBackground The pre-existing background that is to be
     *                          replaced, or zero if there's no background yet.
     */
    KGameRenderedItem * getBackground (const int level,
                                       KGameRenderedItem * currentBackground);

    /*
     * Create the QGraphicsScene item for a border tile.
     *
     * @param spriteKey     The name of the sprite which will be rendered. 
     * @param currentItem   The pre-existing item that is to be replaced, or
     *                      zero if the previous theme had no border.
     */
    KGameRenderedItem * getBorderItem (const QString &spriteKey,
                                       KGameRenderedItem * currentItem);

    /*
     * TODO - Document this.
     */
    KGrSprite * getSpriteItem (const char picType, const int tickTime);

    /*
     * Returns true case the current theme has a border around its background
     * and false otherwise.
     */
    bool hasBorder      () const;

    /*
     * Get the color of the scene's background brush requested for the current
     * theme.
     */
    QColor borderColor  () const;

    /*
     * Get the color of the on-screen text which appears in certain game stages
     * (the demo stage for instance) and in the score box.
     */
    QColor textColor    () const;

    /*
     * Get a pixmap of a particular tile type (e.g. brick, ladder, gold etc.) 
     *
     * @param picType The internal KGoldRunner type of the required tile.
     */
    QPixmap getPixmap   (const char picType);

    /*
     * Show the theme-selector dialog. When the theme changes, KGrRenderer uses
     * a signal and slot to keep the "Set" and "Actors" parts of the theme and
     * SVG files in synch.
     */
    void selectTheme();

private Q_SLOTS:
     // Keep the "Set" and "Actors" parts of a KGoldrunner theme in synch as
     // the theme-selection changes.
    void currentThemeChanged(const KGameTheme * currentSetTheme);

private:
    enum   PicSrc     {Actors, Set};

    // Structure of table-row to specify a tile or pixmap type in a theme.
    struct PixmapSpec {
        const char    picType;		// KGoldrunner's internal type.
	const PicSrc  picSource;	// Actors or Set?
	const char *  picKey;		// Prefix of SVG element name.
	const char *  frameSuffix;	// Format of suffix or "" if none.
	const int     frameBaseIndex;	// Lowest value of suffix or -1 if none.
	      int     frameCount;	// Number of variants available.
					// -2 = not yet counted, -1 = element
					// not found, 0 = only one variant with
					// no suffix, >0 = number of variants.
    };

    KGrScene        * m_scene;		// The scene to be rendered.

    KGameThemeProvider * m_setProvider;	// Provider for Set themes.
    KGameThemeProvider * m_actorsProvider;	// Provider for Actors themes.

    KGameThemeSelector * m_themeSelector;	// Selector (dialog) for themes.

    KGameRenderer   * m_setRenderer;	// Renderer for Set SVG files.
    KGameRenderer   * m_actorsRenderer;	// Renderer for Actors SVG files.

    static PixmapSpec keyTable [];	// Table of tile/background specs.

    // Set the frame counts to -2 at startup and when the theme changes.
    void initPixmapKeys();

    // Make the Actors theme (hero, etc.) match the Set theme (bricks, etc.).
    void matchThemes (const KGameTheme * currentSetTheme);

    // Find a tile type or background in the table of tiles and backgrounds.
    int findKeyTableIndex (const char picType);

    // Count the number of variants of a tile or background.
    int countFrames (const int index);

    /*
     * Get the SVG element name for a KGoldrunner tile type. If the theme has
     * more than one tile of that type (e.g. BRICK), make a random selection.
     *
     * @param picType The internal KGoldrunner type of a tile or background.
     */
    QString getPixmapKey (const int index);

    /*
     * Get the SVG element name for a KGoldrunner background. If the theme has
     * more than one background, cycle though the choices as the KGoldrunner
     * game's level changes.
     *
     * @param level The current level in a KGoldrunner game.
     */
    QString getBackgroundKey (const int level);
};

#endif // KGRRENDERER_H
