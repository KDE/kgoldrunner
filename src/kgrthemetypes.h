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

#ifndef KGRTHEMETYPES_H
#define KGRTHEMETYPES_H

#include <KgTheme>

/**
 * Class to locate KGoldrunner's "Actors" SVG files: derived from KgTheme.
 */
class KGrActorsTheme : public KgTheme
{
    Q_OBJECT
public:
    Q_INVOKABLE KGrActorsTheme(const QByteArray &identifier, QObject *parent=0);
    virtual ~KGrActorsTheme();

    /*
     * Re-defined from KgTheme. Finds a SVG file with config name "Actors".
     *
     * @param path    The full path of the theme's .desktop file.
     */
    bool readFromDesktopFile(const QString& path);
};


/**
 * Class to locate KGoldrunner's "Set" SVG files: derived from KgTheme.
 */
class KGrSetTheme : public KgTheme
{
    Q_OBJECT
public:
    Q_INVOKABLE KGrSetTheme(const QByteArray &identifier, QObject *parent=0);
    virtual ~KGrSetTheme();

    /*
     * Re-defined from KgTheme. Finds a SVG file with config name "Set".
     *
     * @param path    The full path of the theme's .desktop file.
     */
    bool readFromDesktopFile(const QString& path);
};

#endif // KGRTHEMETYPES_H
