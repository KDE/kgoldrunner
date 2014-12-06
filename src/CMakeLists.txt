
########### next target ###############

set(kgoldrunner_SRCS 
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

kde4_add_app_icon(kgoldrunner_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-apps-kgoldrunner.png")
add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner  KF5KDEGames KF5KDEGamesPrivate KF5::KDELibs4Support)

install(TARGETS kgoldrunner  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS org.kde.KGoldrunner.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kgoldrunner )

ecm_install_icons( ICONS hi128-apps-kgoldrunner.png  hi16-apps-kgoldrunner.png  hi22-apps-kgoldrunner.png  hi32-apps-kgoldrunner.png  hi48-apps-kgoldrunner.png  hi64-apps-kgoldrunner.png DESTINATION ${ICON_INSTALL_DIR} )
