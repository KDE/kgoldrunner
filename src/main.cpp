/*
    Copyright 2003 Marco Krüger <grisuji@gmx.de>
    Copyright 2003 Ian Wadham <ianw2@optusnet.com.au>

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
#include "kgrconsts.h"
#include "kgoldrunner.h"

static const char description[] =
    I18N_NOOP ("KGoldrunner is a game of action and puzzle solving");

static const char version[] = "3.0";

int main (int argc, char **argv)
{
    KAboutData about ("kgoldrunner", 0, ki18n ("KGoldrunner"),
                     version, ki18n (description),
                     KAboutData::License_GPL,
                     ki18n ("(C) 2003 Ian Wadham and Marco Krüger"),
                     KLocalizedString(), "http://games.kde.org/kgoldrunner" );
    about.addAuthor (ki18n ("Ian Wadham"), ki18n ("Current author"),
                            "ianw2@optusnet.com.au");
    about.addAuthor (ki18n ("Marco Krüger"), ki18n ("Original author"),
                            "grisuji@gmx.de");
    about.addCredit (ki18n ("Mauricio Piacentini"),
                     ki18n ("Port to KDE4, Qt4 and KGameCanvas classes"), 
                            "mauricio@tabuleiro.com");
    about.addCredit (ki18n ("Maurizio Monge"),
                     ki18n ("KGameCanvas classes for KDE4"), 
                            "maurizio.monge@gmail.com");
    about.addCredit (ki18n ("Mauricio Piacentini"),
                     ki18n ("Artwork for runners and default theme"), 
                            "mauricio@tabuleiro.com");
    about.addCredit (ki18n ("Johann Ollivier Lapeyre"),
                     ki18n ("Artwork for bars and ladders"), 
                            "johann.ollivierlapeyre@gmail.com");
    about.addCredit (ki18n ("Eugene Trounev"),
                     ki18n ("Artwork for background of Geek City theme"), 
                            "irs_me@hotmail.com");
    about.addCredit (ki18n ("Luciano Montanaro"),
                     ki18n ("Nostalgia themes, improvements to runners, "
                            "multiple-backgrounds feature, fade-in/fade-out "
                            "feature and several other ideas"), 
                            "mikelima@cirulla.net");
    about.addCredit (ki18n ("Eugene Trounev"),
                     ki18n ("Artwork for the Treasure of Egypt theme"), 
                            "irs_me@hotmail.com");

    KCmdLineArgs::init (argc, argv, &about);

    KApplication app;
    // See if we are starting with session management.
    if (app.isSessionRestored()) {
        RESTORE (KGoldrunner);
        return app.exec();
    }
    else {
        KGoldrunner * widget = new KGoldrunner;
        if (widget->startedOK()) {
            widget->show();
            return app.exec();
        }
    }
}
