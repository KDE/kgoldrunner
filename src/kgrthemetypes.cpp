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

#include <QString>
#include <QFileInfo>
#include <QDir>

#include <QDebug>

#include "kgrthemetypes.h"

// Helper: finds the absolute path of a file pointed to by the .desktop file.
QString absolutePath (const QString & path, const QString & relativePath)
{
    const QFileInfo file (path);
    const QDir dir = file.dir();
    QString result = "";
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
    qDebug() << "KGrActorsTheme constructor";
}

KGrActorsTheme::~KGrActorsTheme()
{
    qDebug() << "KGrActorsTheme destructor";
}

bool KGrActorsTheme::readFromDesktopFile(const QString& path)
{
    qDebug() << "KGrActorsTheme::readFromDesktopFile" << path;
    // Base class call.
    if (!KgTheme::readFromDesktopFile(path))
        return false;

    // Customised behaviour: interprete "Actors" key as "FileName" for SVG file.
    qDebug() << "Calling setGraphicsPath" << absolutePath (path, customData("Actors"));
    setGraphicsPath (absolutePath (path, customData("Actors")));
    return true;
}

KGrSetTheme::KGrSetTheme(const QByteArray &identifier, QObject *parent)
    :
    KgTheme(identifier, parent)
{
    qDebug() << "KGrSetTheme constructor";
}

KGrSetTheme::~KGrSetTheme()
{
    qDebug() << "KGrSetTheme destructor";
}

bool KGrSetTheme::readFromDesktopFile(const QString& path)
{
    qDebug() << "KGrSetTheme::readFromDesktopFile" << path;
    // Base class call.
    if (!KgTheme::readFromDesktopFile(path))
        return false;

    // Customised behaviour: interprete "Set" key as "FileName" for SVG file.
    qDebug() << "Calling setGraphicsPath" << absolutePath (path, customData("Set"));
    setGraphicsPath (absolutePath (path, customData("Set")));
    return true;
}

#include "kgrthemetypes.moc"
