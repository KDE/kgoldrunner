####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = kgoldrunner
kgoldrunner_SOURCES = kgoldrunnerwidget.cpp kgrobj.cpp kgoldrunner.cpp main.cpp 
SOURCES = kgoldrunnerwidget.cpp kgrobj.cpp kgoldrunner.cpp main.cpp 
INTERFACES = 
TRANSLATIONS = 
KDE_OPTIONS = qtonly
kgoldrunner_LDADD   = -lqt -lkdecore -lXext -lX11 $(LIBSOCKET)

SUBDIRS = docs system 

EXTRA_DIST = main.cpp kgoldrunner.cpp kgoldrunner.h filenew.xpm filesave.xpm fileopen.xpm kgrobj.h kgrobj.cpp kgoldrunnerwidget.h kgoldrunnerwidget.cpp hero.xpm ladder.xpm back.xpm beton.xpm brick.xpm editenemy.xpm edithero.xpm enemy1.xpm enemy2.xpm forward.xpm hgbrick.xpm hladder.xpm home.xpm nugget.xpm pole.xpm kgoldrunner.xpm mini-kgoldrunner.xpm KGoldrunner.desktop fix_src 

install-data-local:
	$(mkinstalldirs) $(kde_icondir)/locolor/32x32/apps/
	$(INSTALL_DATA) $(srcdir)/kgoldrunner.xpm $(kde_icondir)/locolor/32x32/apps/kgoldrunner.xpm
	$(mkinstalldirs) $(kde_icondir)/locolor/16x16/apps/
	$(INSTALL_DATA) $(srcdir)/mini-kgoldrunner.xpm $(kde_icondir)/locolor/16x16/apps/kgoldrunner.xpm
	$(mkinstalldirs) $(kde_appsdir)/Games/
	$(INSTALL_DATA) $(srcdir)/KGoldrunner.desktop $(kde_appsdir)/Games/KGoldrunner.desktop

uninstall-local:
	-rm -f $(kde_icondir)/locolor/32x32/apps/kgoldrunner.xpm
	-rm -f $(kde_icondir)/locolor/16x16/apps/kgoldrunner.xpm
	-rm -f $(kde_appsdir)/Games/KGoldrunner.desktop

####### kdevelop will overwrite this part!!! (end)############


# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

kgoldrunner_METASOURCES = AUTO

# the library search path. 
kgoldrunner_LDFLAGS = $(all_libraries) 

# them while "make clean", use CLEANFILES
DISTCLEANFILES = $(kgoldrunner_METASOURCES)

# this is to enable Qt behavoir of am_edit
KDE_OPTIONS = qtonly



