
########### next target ###############

set(kgoldrunner_SRCS
   kgoldrunner_debug.cpp 
   kgrview.cpp
   kgrscene.cpp
   kgrsprite.cpp
   kgoldrunner.cpp 
   kgrthemetypes.cpp
   kgrrenderer.cpp
   kgrgame.cpp 
   kgrlevelplayer.cpp
   kgrlevelgrid.cpp
   kgrrunner.cpp
   kgrrulebook.cpp
   kgrgameio.cpp 
   kgrtimer.cpp
   kgreditor.cpp
   main.cpp 
   kgrselector.cpp
   kgrsounds.cpp 
   kgrdialog.cpp )

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-apps-kgoldrunner.png")
ecm_add_app_icon(kgoldrunner_SRCS ICONS ${ICONS_SRCS})
add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner  KF5KDEGames KF5KDEGamesPrivate KF5::KDELibs4Support KF5::DBusAddons)

install(TARGETS kgoldrunner  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS org.kde.KGoldrunner.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kgoldrunner )

ecm_install_icons( ICONS hi128-apps-kgoldrunner.png  hi16-apps-kgoldrunner.png  hi22-apps-kgoldrunner.png  hi32-apps-kgoldrunner.png  hi48-apps-kgoldrunner.png  hi64-apps-kgoldrunner.png DESTINATION ${KDE_INSTALL_ICONDIR} )
