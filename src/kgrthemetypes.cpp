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

#include <QDir>
#include <QFileInfo>
#include <QString>

#include "kgrthemetypes.h"

// Helper: Find the absolute path of a file pointed to by the .desktop file.
QString absolutePath (const QString & path, const QString & relativePath)
{
    const QFileInfo file   (path);
    const QDir      dir    = file.dir();
    QString         result;
    // Add the directory part of "path" to "relativePath".
    if (!relativePath.isEmpty() && QFileInfo(relativePath).isRelative()) {
        result = dir.absoluteFilePath(relativePath);
    }
    return result;
}

KGrActorsTheme::KGrActorsTheme(const QByteArray &identifier, QObject *parent)
    :
    KgTheme(identifier, parent)
{
}

KGrActorsTheme::~KGrActorsTheme()
{
}

bool KGrActorsTheme::readFromDesktopFile(const QString& path)
{
    // Base-class call.
    if (!KgTheme::readFromDesktopFile(path))
        return false;

    // Customized behaviour: interprete "Actors" key as "FileName" for SVG file.
    setGraphicsPath (absolutePath (path, customData(QStringLiteral("Actors"))));
    return true;
}

KGrSetTheme::KGrSetTheme(const QByteArray &identifier, QObject *parent)
    :
    KgTheme(identifier, parent)
{
}

KGrSetTheme::~KGrSetTheme()
{
}

bool KGrSetTheme::readFromDesktopFile(const QString& path)
{
    // Base-class call.
    if (!KgTheme::readFromDesktopFile(path))
        return false;

    // Customized behaviour: interprete "Set" key as "FileName" for SVG file.
    setGraphicsPath (absolutePath (path, customData(QStringLiteral("Set"))));
    return true;
}


