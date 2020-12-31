/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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


