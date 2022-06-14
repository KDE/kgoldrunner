/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

    // TODO - Border tiles, display tiles.
    // TODO - Add attributes to theme: HasBorderTiles, HasDisplayTiles.

// KDEGames
#include <kdegames_version.h>
#include <KGameRenderedItem>
#include <KgThemeProvider>
#include <KgThemeSelector>
// KF
#include <KLocalizedString>


#include "kgoldrunner_debug.h"
#include "kgrglobals.h"
#include "kgrthemetypes.h"
#include "kgrrenderer.h"
#include "kgrscene.h"

#include <cmath>

KGrRenderer::KGrRenderer (KGrScene * scene)
    :
    QObject (scene),
    m_scene (scene)
{
    // Set up two theme providers: for the Set and the Actors.
    m_setProvider     = new KgThemeProvider("Theme", this);	// Save config.
    m_actorsProvider  = new KgThemeProvider("",      this);	// Do not save.

    // Find SVG files for the Set, i.e. tiles and backgrounds.
    const QMetaObject * setThemeClass = & KGrSetTheme::staticMetaObject;
#if KDEGAMES_VERSION >= QT_VERSION_CHECK(7, 4, 0)
    m_setProvider->discoverThemes (QStringLiteral ("themes"),
#else
    m_setProvider->discoverThemes ("appdata", QStringLiteral ("themes"),
#endif
                                   QStringLiteral ("egypt"), setThemeClass);

    // Find SVG files for the Actors, i.e. hero and enemies.
    const QMetaObject * actorsThemeClass = & KGrActorsTheme::staticMetaObject;
#if KDEGAMES_VERSION >= QT_VERSION_CHECK(7, 4, 0)
    m_actorsProvider->discoverThemes (QStringLiteral ("themes"),
#else
    m_actorsProvider->discoverThemes ("appdata", QStringLiteral ("themes"),
#endif
                                   QStringLiteral ("egypt"), actorsThemeClass);

    // Set up a dialog for selecting themes.
    m_themeSelector  = new KgThemeSelector (m_setProvider,
                                            KgThemeSelector::DefaultBehavior,
                                            nullptr);	// No parent: modeless dialog.

    // Set up the renderer for the Set, i.e. tiles and backgrounds.
    m_setRenderer    = new KGameRenderer (m_setProvider);
    m_setRenderer->setParent (this);
    m_setRenderer->setFrameSuffix (QStringLiteral("_%1"));
    m_setRenderer->setFrameBaseIndex (1);

    // Set up the renderer for the Actors, i.e. hero and enemies.
    m_actorsRenderer = new KGameRenderer (m_actorsProvider);
    m_actorsRenderer->setParent (this);
    m_actorsRenderer->setFrameSuffix (QStringLiteral("_%1"));
    m_actorsRenderer->setFrameBaseIndex (1);

    // Match the Actors SVG theme to the Set theme, whenever the theme changes.
    connect(m_setProvider, &KgThemeProvider::currentThemeChanged, this, &KGrRenderer::currentThemeChanged);

    // Match the starting SVG theme for the Actors to the one for the Set.
    matchThemes (m_setProvider->currentTheme());
}

KGrRenderer::~KGrRenderer()
{
    delete m_themeSelector;
}

void KGrRenderer::matchThemes (const KgTheme * currentSetTheme)
{
    // Start of game or change of theme: initialise the counts of pixmap keys.
    initPixmapKeys();

    const auto themes = m_actorsProvider->themes();
    for (const KgTheme * actorsTheme : themes) {
    if (actorsTheme->customData(QStringLiteral("Set")) ==
            currentSetTheme->customData(QStringLiteral("Set"))) {
	    m_actorsProvider->setCurrentTheme (actorsTheme);
	    break;
	}
    }
}

void KGrRenderer::currentThemeChanged (const KgTheme* currentSetTheme)
{
    //qCDebug(KGOLDRUNNER_LOG) << "KGrRenderer::currentThemeChanged()" << currentSetTheme->name();

    matchThemes (currentSetTheme);
    m_scene->changeTheme();
}

void KGrRenderer::selectTheme()
{
    // Show the theme-selection dialog.
    m_themeSelector->showAsDialog (i18n("Theme Selector"));
}

KGrRenderer::PixmapSpec KGrRenderer::keyTable [] = {
    {ENEMY,    Actors, "enemy_1",       "",   -1, -2},	// For editor only.
    {HERO,     Actors, "hero_1",        "",   -1, -2},	// For editor only.
    {CONCRETE, Set,    "concrete",      "-%1", 0, -2},
    {BRICK,    Set,    "brick",         "-%1", 0, -2},
    {FBRICK,   Set,    "false_brick",   "",   -1, -2},	// For editor only.
    {HLADDER,  Set,    "hidden_ladder", "",   -1, -2},	// For editor only.
    {LADDER,   Set,    "ladder",        "-%1", 0, -2},
    {NUGGET,   Set,    "gold",          "-%1", 0, -2},
    {BAR,      Set,    "bar",           "-%1", 0, -2},
    {BACKDROP, Set,    "background",    "%1",  0, -2},
    {FREE,     Set,    "empty",         "",   -1, -2}	// Must be last entry.
};

void KGrRenderer::initPixmapKeys()
{
    // Set all pixmaps in keyTable[] as "not counted yet" (frameCount -2).
    int index = 0;
    do {
	keyTable[index].frameCount = -2;
	index++;
    } while (keyTable[index].picType != FREE);
}

KGameRenderedItem * KGrRenderer::getTileItem
                    (const char picType, KGameRenderedItem * currentTile)
{
    if (currentTile) {
	// Remove the tile that was here before.
        m_scene->removeItem (currentTile);
        delete currentTile;
    }

    int index;
    if ((picType == FREE) || ((index = findKeyTableIndex (picType)) < 0)) {
        return nullptr;	// Empty place or missing type, so no KGameRenderedItem.
    }

    // Get the pixmap key and use one of the two renderers to create the tile.
    QString key = getPixmapKey (index);
    KGameRenderedItem * tile =
                new KGameRenderedItem ((keyTable[index].picSource == Set) ?
                                       m_setRenderer : m_actorsRenderer, key);
    tile->setAcceptedMouseButtons (Qt::NoButton);
    m_scene->addItem (tile);
    return tile;
}

KGrSprite * KGrRenderer::getSpriteItem (const char picType, const int tickTime)
{
    int index = findKeyTableIndex (picType);
    if (index < 0) {
        return nullptr;	// Missing type, so no KGrSprite item.
    }
    QString key = (picType == HERO) ? QStringLiteral("hero") :
                  ((picType == ENEMY) ? QStringLiteral("enemy") : QStringLiteral("brick"));
    KGrSprite * sprite = new KGrSprite ((keyTable[index].picSource == Set) ?
                                        m_setRenderer : m_actorsRenderer,
                                        key, picType, tickTime);
    sprite->setAcceptedMouseButtons (Qt::NoButton);
    // We cannot add the sprite to the scene yet: it needs a frame and size.
    return sprite;
}

KGameRenderedItem * KGrRenderer::getBackground
                    (const int level, KGameRenderedItem * currentBackground)
{
    if (currentBackground) {
        m_scene->removeItem (currentBackground);
        delete currentBackground;
    }

    QString key = getBackgroundKey (level);
    KGameRenderedItem * background = new KGameRenderedItem (m_setRenderer, key);
    background->setAcceptedMouseButtons (Qt::NoButton);
    m_scene->addItem (background);

    return background;
}

KGameRenderedItem * KGrRenderer::getBorderItem
                    (const QString &spriteKey, KGameRenderedItem * currentItem)
{
    if (currentItem) {
        m_scene->removeItem (currentItem);
        delete currentItem;
    }

    if (!hasBorder()) {
        return nullptr;
    }

    KGameRenderedItem * item = new KGameRenderedItem (m_setRenderer, spriteKey);
    item->setAcceptedMouseButtons (Qt::NoButton);
    m_scene->addItem (item);
    return item;
}

bool KGrRenderer::hasBorder() const
{
    QString s = m_setRenderer->theme()->customData(QStringLiteral("DrawCanvasBorder"), QStringLiteral("0"));

    if (s == QLatin1Char('1'))
        return true;
    else
        return false;
}

QColor KGrRenderer::borderColor() const
{
    QString s = m_setRenderer->theme()->customData(QStringLiteral("BorderColor"), QStringLiteral("#000000"));
    return QColor (s);
}

QColor KGrRenderer::textColor() const
{
    QString s = m_setRenderer->theme()->customData(QStringLiteral("TextColor"), QStringLiteral("#FFFFFF"));
    return QColor (s);
}

QPixmap KGrRenderer::getPixmap (const char picType)
{
    // Get the pixmap key and use one of the two renderers to create the tile.
    int index   = findKeyTableIndex (picType);
    QString key = getPixmapKey      (index);

    if (keyTable[index].picSource == Set)
        return m_setRenderer->spritePixmap (key, m_scene->tileSize ());
    else
        return m_actorsRenderer->spritePixmap (key, m_scene->tileSize ());
}

QString KGrRenderer::getPixmapKey (const int index)
{
    QString pixmapKey;
    // int index = findKeyTableIndex (picType);
    int frameCount = (index < 0) ? -1 : keyTable[index].frameCount;
    if (frameCount > -1) {
    pixmapKey = QLatin1String(keyTable[index].picKey);	// No suffix.
	if (frameCount > 0) {
	    // Pick a random frame number and add it as a suffix.
	    // Note: We are not worried about having a good seed for this.
        pixmapKey = pixmapKey + QLatin1String(keyTable[index].frameSuffix);
	    pixmapKey = pixmapKey.arg (keyTable[index].frameBaseIndex +
				       (rand() % frameCount));
	}
    }
    return pixmapKey;
}

QString KGrRenderer::getBackgroundKey (const int level)
{
    QString pixmapKey;
    int index = findKeyTableIndex (BACKDROP);
    int frameCount = (index < 0) ? -1 : keyTable[index].frameCount;
    if (frameCount > -1) {
    pixmapKey = QLatin1String(keyTable[index].picKey);
	if (frameCount > 0) {
	    // Cycle through available backgrounds as the game-level increases.
        pixmapKey = pixmapKey + QLatin1String(keyTable[index].frameSuffix);
	    pixmapKey = pixmapKey.arg (level % frameCount);
	}
    }

    //qCDebug(KGOLDRUNNER_LOG) << "BACKGROUND pixmap key" << pixmapKey;
    return pixmapKey;
}

int KGrRenderer::findKeyTableIndex (const char picType)
{
    int index = 0;
    while (true) {		// Find ANY picType, including FREE.
	if (keyTable[index].picType == picType) {
	    if (keyTable[index].frameCount == -2) {
		keyTable[index].frameCount = countFrames (index);
	    }
	    break;
	}
	else if (keyTable[index].picType == FREE) {
	    index = -1;		// Not found.
	    break;
	}
	index++;
    }
    return index;
}

int KGrRenderer::countFrames (const int index)
{
    int count = -1;
    int frame = keyTable[index].frameBaseIndex;
    KGameRenderer * r = (keyTable[index].picSource == Set) ? m_setRenderer :
                                                             m_actorsRenderer;
    if (r->spriteExists (QLatin1String(keyTable[index].picKey))) {
        count++;
    }

    if ((count == 0) && (QLatin1String(keyTable[index].picKey) != QLatin1String("brick"))) {
	return count;
    }

    if (frame < 0) {
	return count;		// This element cannot have more than one frame.
    }

    count = 0;
    QString pixmapKey = QLatin1String(keyTable[index].picKey) +
                        QLatin1String(keyTable[index].frameSuffix);
    while (r->spriteExists (pixmapKey.arg (frame))) {
	count++;
	frame++;
    }

    return count;
}


