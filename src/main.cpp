/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Jan 23 15:19:17 EST 2002
    copyright            : (C) 2002 by Marco Krüger and Ian Wadham
    email                : See menu "Help, About KGoldrunner"
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kgoldrunner.h"
#include <kglobalsettings.h>
// #include <qtextcodec.h>

int main (int argc, char *argv[])
{
    QApplication app (argc, argv);
    KGoldrunner widget;

    app.setFont( KGlobalSettings::generalFont() );
    // QTranslator tor( 0 );
    // set the location where your .qm files are in load() below as the last parameter instead of "."
    // for development, use "/" to use the english original as
    // .qm files are stored in the base project directory.
    // tor.load( QString("kgoldrunner.") + QString(QTextCodec::locale()), "." );
    // app.installTranslator( &tor );

    /* uncomment the following line, if you want a Windows 95 look*/
    // app.setStyle(WindowsStyle);

    app.setMainWidget(&widget);

    return app.exec();
}
