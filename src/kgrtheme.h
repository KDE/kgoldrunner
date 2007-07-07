/***************************************************************************
                         kgrtheme.h  -  description
                             -------------------
    begin                : Wed Jan 23 2002
    Copyright 2002 Marco Krüger <grisuji@gmx.de>
    Copyright 2002 Ian Wadham <ianw2@optusnet.com.au>
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

class KGrTheme
{
public:
    KGrTheme(const QString &systemDataDir);
    void load (const QString & themeFilepath);
    QList<QPixmap> tiles(int size);
    QList<QPixmap> hero(int size);
    QList<QPixmap> enemy(int size);
    QImage background(int width, int height);
    bool isBorderRequired() { return themeDrawBorder; }
    bool isWithBackground() { return hasBackground; }
    QColor borderColor() { return m_borderColor; }
    QColor textColor() { return m_textColor; }
private:
    KSvgRenderer svg;
    QColor m_borderColor, m_textColor;	// Border colours.

    QString picsDataDir;
    QString filepathSVG;
    QString m_themeFilepath;
    short themeDrawBorder;
    void changeColors (const char * colours []);
    void recolorObject (const char * object [], const char * colours []);
    QList<QPixmap> xpmFrames (const QImage & image, int size, const int nFrames);
    QPixmap svgTile (QImage &image, QPainter &painter, const QString &name);
    QList<QPixmap> svgFrames (const QString & elementPattern, int size, const int nFrames);

    enum GraphicsType { NONE, SVG, XPM, PNG };
    GraphicsType tileGraphics;
    GraphicsType backgroundGraphics;
    GraphicsType runnerGraphics;
    bool hasBackground;
};

#endif // KGRTHEME_H
