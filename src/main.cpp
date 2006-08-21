/*
    Copyright 2003 Marco Krüger
    Copyright 2003 Ian Wadham <ianw@netspace.net.au>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <QtDBus/QtDBus>
#include "kgrconsts.h"
#include "kgoldrunner.h"

static const char description[] =
    I18N_NOOP("KGoldrunner is a game of action and puzzle solving");

static const char version[] = "2.1";

int main (int argc, char **argv)
{
    KAboutData about("kgoldrunner", I18N_NOOP("KGoldrunner" ),
    		     version, description,
                     KAboutData::License_GPL,
		     "(C) 2003 Ian Wadham and Marco Krüger");
    about.addAuthor( "Ian Wadham", I18N_NOOP("Current author"),
    		     "ianw@netspace.net.au" );
    about.addAuthor( "Marco Krüger", I18N_NOOP("Original author"), 0);
    about.addCredit("Mauricio Piacentini", I18N_NOOP("Port to KDE4"), 
		      "mauricio@tabuleiro.com");

    KCmdLineArgs::init (argc, argv, &about);

    KApplication app;
    QString nameapp = QString("org.kde.%1").arg(app.objectName());
    QDBusConnection::sessionBus().interface()->registerService(nameapp);
    // See if we are starting with session management.
    if (app.isSessionRestored())
    {
        RESTORE(KGoldrunner);
	return app.exec();
    }
    else
    {
	KGoldrunner * widget = new KGoldrunner;
	if (widget->startedOK()) {
	    widget->show();
	    return app.exec();
	}
    }
}
