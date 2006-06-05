/*
 * Copyright (C) 2003 Ian Wadham and Marco Krüger <ianw@netspace.net.au>
 */

#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <dbus/qdbus.h>
#include "kgrconsts.h"
#include "kgoldrunner.h"

static const char description[] =
    I18N_NOOP("KGoldrunner is a game of action and puzzle solving");

static const char version[] = "2.0";

int main (int argc, char **argv)
{
    KAboutData about("kgoldrunner", I18N_NOOP("KGoldrunner" ),
    		     version, description,
                     KAboutData::License_GPL,
		     "(C) 2003 Ian Wadham and Marco Krüger");
    about.addAuthor( "Ian Wadham", I18N_NOOP("Current author"),
    		     "ianw@netspace.net.au" );
    about.addAuthor( "Marco Krüger", I18N_NOOP("Original author"), 0);

    KCmdLineArgs::init (argc, argv, &about);

    KApplication app;
	QString nameapp = QString("org.kde.%1").arg(app.name());
	QDBus::sessionBus().busService()->requestName(nameapp, /*flags=*/0);
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
