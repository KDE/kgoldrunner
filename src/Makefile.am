## Makefile.am for kgoldrunner

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = kgoldrunner

# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# the library search path.
kgoldrunner_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against.
kgoldrunner_LDADD   = $(LIB_KDEGAMES) $(LIB_KFILE) $(LIB_KDEPRINT)

# which sources should be compiled for kgoldrunner
kgoldrunner_SOURCES = kgoldrunner.cpp kgrcanvas.cpp kgrfigure.cpp kgrgame.cpp kgrobject.cpp main.cpp kgrdialog.cpp

# these are the headers for your project
noinst_HEADERS = back.xpm beton.xpm brick.xpm editenemy.xpm edithero.xpm enemy1.xpm enemy2.xpm filenew.xpm hero.xpm hgbrick.xpm hladder.xpm home.xpm ladder.xpm nugget.xpm pole.xpm fileopen.xpm filesave.xpm forward.xpm kgoldrunner.h kgrcanvas.h kgrconsts.h kgrdialog.h kgrfigure.h kgrgame.h kgrobject.h

# client stuff


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kgoldrunner.pot

KDE_ICON = kgoldrunner

# this is where the kdelnk file will go
kdelnkdir = $(kde_appsdir)/Games/Arcade
kdelnk_DATA = KGoldrunner.desktop

# this is where the XML-GUI resource file goes
rcdir = $(kde_datadir)/kgoldrunner
rc_DATA = kgoldrunnerui.rc

kde_icon_KDEICON = hi16-app-kgoldrunner.png hi32-app-kgoldrunner.png

AM_CXXFLAGS = -DQT3

SUBDIRS = system
