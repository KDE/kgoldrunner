####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = kgoldrunner

## INCLUDES were found outside kdevelop specific part

kgoldrunner_SOURCES = main.cpp kgrobj.cpp kgoldrunnerwidget.cpp kgoldrunner.cpp 
SOURCES = main.cpp kgrobj.cpp kgoldrunnerwidget.cpp kgoldrunner.cpp 
INTERFACES = 
TRANSLATIONS = 
KDE_OPTIONS = qtonly
kgoldrunner_LDADD   = $(LIB_QPE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = filenew.xpm filesave.xpm fileopen.xpm kgoldrunner.cpp kgoldrunner.h kgoldrunnerwidget.cpp kgoldrunnerwidget.h kgrobj.cpp kgrobj.h main.cpp back.xpm beton.xpm brick.xpm editenemy.xpm edithero.xpm enemy1.xpm enemy2.xpm forward.xpm hero.xpm hgbrick.xpm hladder.xpm home.xpm ladder.xpm mini-kgoldrunner.xpm nugget.xpm pole.xpm 

####### kdevelop will overwrite this part!!! (end)############

KDE_CXXFLAGS = -DQT3

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) -I/opt/kde3/include

kgoldrunner_METASOURCES = AUTO

# the library search path. 
kgoldrunner_LDFLAGS = $(all_libraries) -L/opt/kde3/lib -lkdecore

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(kgoldrunner_METASOURCES)

