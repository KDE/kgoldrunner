/****************************************************************************
 *    Copyright 2012  Ian Wadham <iandw.au@gmail.com>                       *
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

#include <KGameRenderer>
#include <KgThemeProvider>
#include <KgThemeSelector>
#include <KLocalizedString>

#include <QDebug>
#include <QString>

#include "kgrthemetypes.h"
#include "kgrrenderer.h"
#include "kgrglobals.h"

KGrRenderer::KGrRenderer (QWidget * view)
    :
    QObject (view)
{
    qDebug() << "KGrRenderer called";
    m_setProvider    = new KgThemeProvider("Theme", this);	// Saved config.
    m_actorsProvider = new KgThemeProvider("",      this);	// Do not save.

    const QMetaObject * setThemeClass = & KGrSetTheme::staticMetaObject;
    m_setProvider->discoverThemes ("appdata", QLatin1String ("themes"),
                                   QLatin1String ("egypt"), setThemeClass);

    m_themeSelector = new KgThemeSelector (m_setProvider,
                                           KgThemeSelector::DefaultBehavior,
                                           0);	// No parent: modeless dialog.

    m_setRenderer   = new KGameRenderer (m_setProvider);
    m_setRenderer->setParent (this);

    initPixmapKeys();
}

KGrRenderer::~KGrRenderer()
{
    delete m_themeSelector;
}

void KGrRenderer::selectTheme()
{
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
    {FREE,     Set,    "",              "",   -1, -2}	// Must be last entry.
};

void KGrRenderer::initPixmapKeys()
{
    int index = 0;
    while (true) {
	// Start of game or change of theme: pixmap keys not counted yet.
	keyTable[index].frameCount = -2;
	if (keyTable[index].picType == FREE) {
	    break;			// End of table: all done.
	}
	index++;
    }
}

QString KGrRenderer::getPixmapKey (const char picType)
{
    // TODO - Backgrounds, border tiles, display tiles.
    // TODO - Add attributes to theme: HasBorderTiles, HasDisplayTiles.

    QString pixmapKey = "";
    int index = 0;
    while (true) {
	if (keyTable[index].picType == FREE) {
	    break;			// Pixmap key not found.
	}
	else if (keyTable[index].picType == picType) {
	    if (keyTable[index].frameCount == -2) {
		// TODO - Find and count the SVG elements.
		// TODO - Must check for key with no suffix first.
		// Use picKey + frameSuffix and start at frameBaseIndex.
	    }
	    else if (keyTable[index].frameCount == -1) {
		break;			// Element missing from theme.
	    }
	    else if (keyTable[index].frameCount == 0) {
		pixmapKey = keyTable[index].picKey;	// No suffix.
	    }
	    else {
		// TODO Pick a random frame number and add a suffix.
	    }
	    break;
	}
	index++;
    }
    return pixmapKey;
}

#include "kgrrenderer.moc"
