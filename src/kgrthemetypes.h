/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KGRTHEMETYPES_H
#define KGRTHEMETYPES_H

#include <KGameTheme>

/**
 * Class to locate KGoldrunner's "Actors" SVG files: derived from KGameTheme.
 */
class KGrActorsTheme : public KGameTheme
{
    Q_OBJECT
public:
    Q_INVOKABLE KGrActorsTheme(const QByteArray &identifier, QObject *parent=nullptr);
    ~KGrActorsTheme() override;

    /*
     * Re-defined from KGameTheme. Finds a SVG file with config name "Actors".
     *
     * @param path    The full path of the theme's .desktop file.
     */
    bool readFromDesktopFile(const QString& path) override;
};


/**
 * Class to locate KGoldrunner's "Set" SVG files: derived from KGameTheme.
 */
class KGrSetTheme : public KGameTheme
{
    Q_OBJECT
public:
    Q_INVOKABLE KGrSetTheme(const QByteArray &identifier, QObject *parent=nullptr);
    ~KGrSetTheme() override;

    /*
     * Re-defined from KGameTheme. Finds a SVG file with config name "Set".
     *
     * @param path    The full path of the theme's .desktop file.
     */
    bool readFromDesktopFile(const QString& path) override;
};

#endif // KGRTHEMETYPES_H
