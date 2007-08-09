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
    KGrTheme(const QString &systemDataDir);

    /** 
     * load a theme given the name of its .desktop file 
     */
    void load(const QString & themeFilepath);
    
    QList<QPixmap> tiles(unsigned int size);
    
    QList<QPixmap> hero(unsigned int size);
    
    QList<QPixmap> enemy(unsigned int size);
    
    /**
     * prepares an image with the background variant \param variant with size
     * \param width and \param height.
     */
    QImage background(unsigned int width, unsigned int height, unsigned int variant);
    
    bool isBorderRequired() const { return themeDrawBorder; }
    
    bool isWithBackground() const { return numBackgrounds > 0; }
    
    QColor borderColor() { return m_borderColor; }
    
    QColor textColor() { return m_textColor; }
private:
    KSvgRenderer svgSet;
    KSvgRenderer svgActors;
    QColor m_borderColor, m_textColor;	// Border colours.

    QString themeDataDir;
    QString m_themeFilepath;
    short themeDrawBorder;
    void changeColors (const char * colours []);
    void recolorObject (const char * object [], const char * colours []);
    QList<QPixmap> xpmFrames (const QImage & image, int size, const int nFrames);
    QPixmap svgTile (QImage &image, QPainter &painter, const QString &name);
    QList<QPixmap> svgFrames (const QString & elementPattern, unsigned int size, int nFrames);

    enum GraphicsType { NONE, SVG, XPM, PNG };
    GraphicsType tileGraphics;
    GraphicsType backgroundGraphics;
    GraphicsType runnerGraphics;
    int numBackgrounds;
};

#endif // KGRTHEME_H
