/*
 * Copyright (C) 2003 Ian Wadham and Marco Krüger <ianw2@optusnet.com.au>
 */

#include <kapplication.h>
#include <dcopclient.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

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
    		     "ianw2@optusnet.com.au" );
    about.addAuthor( "Marco Krüger", I18N_NOOP("Original author"), 0);

    KCmdLineArgs::init (argc, argv, &about);

    KApplication app;

    // Register as a DCOP client.
    app.dcopClient()->registerAs (app.name(), false);

    // See if we are starting with session management.
    if (app.isRestored())
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
