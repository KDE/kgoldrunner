/*
    SPDX-FileCopyrightText: 2003 Marco Krüger <grisuji@gmx.de>
    SPDX-FileCopyrightText: 2003 Ian Wadham <iandw.au@gmail.com>
    SPDX-FileCopyrightText: 2009 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>

#include "kgoldrunner_debug.h"
#include "kgoldrunner_version.h"
#include "kgoldrunner.h"

static void addCredits (KAboutData & about);

int main (int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kgoldrunner");

    KAboutData about (QStringLiteral("kgoldrunner"), i18n ("KGoldrunner"),
                     QStringLiteral(KGOLDRUNNER_VERSION_STRING),
                     i18n("A game of action and puzzle solving"),
                     KAboutLicense::GPL,
                     i18n ("(C) 2003 Ian Wadham and Marco Krüger"),
                     QString(),
                     QStringLiteral("https://apps.kde.org/kgoldrunner") );
    addCredits (about);

    KAboutData::setApplicationData(about);

    KCrash::initialize();

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    KDBusService service;

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kgoldrunner")));

    // See if we are starting with session management.
    if (app.isSessionRestored()) {
        // New RESTORE (KGrController);
        kRestoreMainWindows<KGoldrunner>();
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
                            QStringLiteral("iandw.au@gmail.com"));
    about.addAuthor (i18n ("Marco Krüger"), i18n ("Original author"),
                            QStringLiteral("grisuji@gmx.de"));
    about.addCredit (i18n ("Mauricio Piacentini"),
                     i18n ("Port to KDE4, Qt4 and KGameCanvas classes"), 
                            QStringLiteral("mauricio@tabuleiro.com"));
    about.addCredit (i18n ("Maurizio Monge"),
                     i18n ("KGameCanvas classes for KDE4"), 
                            QStringLiteral("maurizio.monge@gmail.com"));
    about.addCredit (i18n ("Mauricio Piacentini"),
                     i18n ("Artwork for runners and default theme"), 
                            QStringLiteral("mauricio@tabuleiro.com"));
    about.addCredit (i18n ("Johann Ollivier Lapeyre"),
                     i18n ("Artwork for bars and ladders"), 
                            QStringLiteral("johann.ollivierlapeyre@gmail.com"));
    about.addCredit (i18n ("Eugene Trounev"),
                     i18n ("Artwork for background of Geek City theme"), 
                            QStringLiteral("irs_me@hotmail.com"));
    about.addCredit (i18n ("Luciano Montanaro"),
                     i18n ("Nostalgia themes, improvements to runners, "
                            "multiple-backgrounds feature, fade-in/fade-out "
                            "feature and several other ideas"), 
                            QStringLiteral("mikelima@cirulla.net"));
    about.addCredit (i18n ("Eugene Trounev"),
                     i18n ("Artwork for the Treasure of Egypt theme"), 
                            QStringLiteral("irs_me@hotmail.com"));
}
