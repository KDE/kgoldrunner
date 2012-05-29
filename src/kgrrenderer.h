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

#ifndef KGRRENDERER_H
#define KGRRENDERER_H

class KgThemeProvider;
class KgThemeSelector;
class KGameRenderer;
class QWidget;

class KGrRenderer : public QObject
{
    Q_OBJECT
public:
    KGrRenderer (QWidget * view);
    virtual ~KGrRenderer();

    QString getPixmapKey (const char picType);

public slots:
    void selectTheme();

private:
    enum PicSrc {Actors, Set};
    struct PixmapSpec {
        const char    picType;
	const PicSrc  picSource;
	const char *  picKey;
	const char *  frameSuffix;
	const int     frameBaseIndex;
	      int     frameCount;
    };

    static PixmapSpec keyTable [];

    void              initPixmapKeys();
    KgThemeProvider * m_setProvider;
    KgThemeProvider * m_actorsProvider;
    KgThemeSelector * m_themeSelector;
    KGameRenderer   * m_setRenderer;
    KGameRenderer   * m_actorsRenderer;
};

#endif // KGRRENDERER_H
