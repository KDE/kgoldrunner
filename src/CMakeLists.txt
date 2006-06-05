
include_directories( ${CMAKE_SOURCE_DIR}/libkdegames ${QDBUS_INCLUDE_DIRS} )

ADD_DEFINITIONS(-DQT3)
########### next target ###############

set(kgoldrunner_SRCS 
   kgoldrunner.cpp 
   kgrcanvas.cpp 
   kgrfigure.cpp 
   kgrgame.cpp 
   kgrobject.cpp 
   main.cpp 
   kgrdialog.cpp )

kde4_automoc(${kgoldrunner_SRCS})

kde4_add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner  ${KDE4_KDEUI_LIBS} kdegames kdeprint )

install_targets(/bin kgoldrunner )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES KGoldrunner.desktop )
install_files( ${DATA_INSTALL_DIR}/kgoldrunner FILES kgoldrunnerui.rc )

kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )




#original Makefile.am contents follow:

### Makefile.am for kgoldrunner
#
## this is the program that gets installed.  it's name is used for all
## of the other Makefile.am variables
#bin_PROGRAMS = kgoldrunner
#
## set the include path for X, qt and KDE
#INCLUDES = -I$(top_srcdir)/libkdegames $(all_includes)
#
## the library search path.
#kgoldrunner_LDFLAGS = $(KDE_RPATH) $(all_libraries)
#
## the libraries to link against.
#kgoldrunner_LDADD   = $(LIB_KDEGAMES) $(LIB_KFILE) $(LIB_KDEPRINT)
#kgoldrunner_DEPENDENCIES = $(LIB_KDEGAMES_DEP)
#
## which sources should be compiled for kgoldrunner
#kgoldrunner_SOURCES = kgoldrunner.cpp kgrcanvas.cpp kgrfigure.cpp kgrgame.cpp kgrobject.cpp main.cpp kgrdialog.cpp
#
## these are the headers for your project
#noinst_HEADERS = enemy1.xpm enemy2.xpm hero.xpm kgoldrunner.h kgrcanvas.h kgrconsts.h kgrdialog.h kgrfigure.h kgrgame.h kgrobject.h kgraphics.h
#
## client stuff
#
#
## let automoc handle all of the meta source files (moc)
#METASOURCES = AUTO
#
#messages: rc.cpp
#	$(XGETTEXT) *.cpp -o $(podir)/kgoldrunner.pot
#
#KDE_ICON = kgoldrunner
#
## this is where the kdelnk file will go
#xdg_apps_DATA = KGoldrunner.desktop
#
## this is where the XML-GUI resource file goes
#rcdir = $(kde_datadir)/kgoldrunner
#rc_DATA = kgoldrunnerui.rc
#
#AM_CXXFLAGS = -DQT3
#
