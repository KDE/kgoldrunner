/****************************************************************************
 *    Copyright 2003  Marco Krüger <grisuji@gmx.de>                         *
 *    Copyright 2003  Ian Wadham <iandw.au@gmail.com>                       *
 *    Copyright 2009  Ian Wadham <iandw.au@gmail.com>                       *
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

#include <QDebug>

#include <KAboutData>

#include <klocale.h>
#include <QApplication>
#include <KLocalizedString>
#include <QCommandLineParser>
#include "kgoldrunner.h"
#include <KDBusService>

static const char description[] =
    I18N_NOOP ("KGoldrunner is a game of action and puzzle solving");

// The intention is to keep the KGoldrunner version number in line with KDE.
static const char version[] = "4.10";

static bool gameDataOK();
static void addCredits (KAboutData & about);

int main (int argc, char **argv)
{
    // Check data integrity and find base directories.
    if (! gameDataOK()) {
 	// Error message;
 	return 2;
    }
    QApplication app(argc, argv);

    KAboutData about ("kgoldrunner", i18n ("KGoldrunner"),
                     version, i18n (description),
                     KAboutLicense::GPL,
                     i18n ("(C) 2003 Ian Wadham and Marco Krüger"),
                      "http://games.kde.org/kgoldrunner" );
    addCredits (about);

    QCommandLineParser parser;
    KAboutData::setApplicationData(about);
    parser.addVersionOption();
    parser.addHelpOption();
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);
    KDBusService service;
    // See if we are starting with session management.
    if (app.isSessionRestored()) {
        // New RESTORE (KGrController);
        RESTORE (KGoldrunner);
    }
    else {
        // New KGrController * controller = new KGrController();
        // New KGrView *       view       = new KGrView (controller);
        // New KGrGame *       game       = new KGrGame (view);
	// New controller->makeConnections (game, view);
        KGoldrunner * controller = new KGoldrunner();
        controller->show();
    }
    return app.exec();
}

void addCredits (KAboutData & about)
{
    about.addAuthor (i18n ("Ian Wadham"), i18n ("Current author"),
                            "iandw.au@gmail.com");
    about.addAuthor (i18n ("Marco Krüger"), i18n ("Original author"),
                            "grisuji@gmx.de");
    about.addCredit (i18n ("Mauricio Piacentini"),
                     i18n ("Port to KDE4, Qt4 and KGameCanvas classes"), 
                            "mauricio@tabuleiro.com");
    about.addCredit (i18n ("Maurizio Monge"),
                     i18n ("KGameCanvas classes for KDE4"), 
                            "maurizio.monge@gmail.com");
    about.addCredit (i18n ("Mauricio Piacentini"),
                     i18n ("Artwork for runners and default theme"), 
                            "mauricio@tabuleiro.com");
    about.addCredit (i18n ("Johann Ollivier Lapeyre"),
                     i18n ("Artwork for bars and ladders"), 
                            "johann.ollivierlapeyre@gmail.com");
    about.addCredit (i18n ("Eugene Trounev"),
                     i18n ("Artwork for background of Geek City theme"), 
                            "irs_me@hotmail.com");
    about.addCredit (i18n ("Luciano Montanaro"),
                     i18n ("Nostalgia themes, improvements to runners, "
                            "multiple-backgrounds feature, fade-in/fade-out "
                            "feature and several other ideas"), 
                            "mikelima@cirulla.net");
    about.addCredit (i18n ("Eugene Trounev"),
                     i18n ("Artwork for the Treasure of Egypt theme"), 
                            "irs_me@hotmail.com");
}

bool gameDataOK()
{
    return true;
}
