# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = kgoldrunner.h kgoldrunner.h kgoldrunnerwidget.h kgrobj.h back.xpm beton.xpm brick.xpm editenemy.xpm edithero.xpm enemy1.xpm enemy2.xpm filenew.xpm fileopen.xpm filesave.xpm forward.xpm hero.xpm hgbrick.xpm hladder.xpm home.xpm kgrobj.h ladder.xpm nugget.xpm pole.xpm

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kgoldrunner.pot

KDE_ICON = kgoldrunner

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kgoldrunner

# the application source, library search path, and link libraries
kgoldrunner_SOURCES = main.cpp kgoldrunner.cpp kgoldrunnerwidget.cpp kgrobj.cpp
kgoldrunner_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kgoldrunner_LDADD = $(LIB_KDEUI)

# this is where the desktop file will go 
shelldesktopdir = $(kde_appsdir)/Games/Arcade
shelldesktop_DATA = KGoldrunner.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir = $(kde_datadir)/kgoldrunner
shellrc_DATA = kgoldrunnerui.rc

AM_CXXFLAGS = -DQT3

gamedatadir = $(kde_datadir)/kgoldrun/system

SUBDIRS = system
kde_icon_KDEICON = kgoldrunner.xpm lo16-app-kgoldrunner.xpm lo32-app-kgoldrunner.xpm
