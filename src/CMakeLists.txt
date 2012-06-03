
########### next target ###############

set(kgoldrunner_SRCS 
   kgrplayfield.cpp 
   kgrsprite.cpp 
   kgoldrunner.cpp 
   kgrcanvas.cpp 
   kgrtheme.cpp 
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

kde4_add_app_icon(kgoldrunner_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kgoldrunner.png")
kde4_add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner ${KDE4_KDEUI_LIBS} kdegames kdegamesprivate)

########### install files ###############

install( PROGRAMS KGoldrunner.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kgoldrunner )

kde4_install_icons( ${ICON_INSTALL_DIR} )

########### next target ###############

set(kgrtest_SRCS # NOTE: This target (kgrtest) is for testing purposes only.
   g.cpp
   kgrthemetypes.cpp
   kgrrenderer.cpp
   testmain.cpp 
   )

kde4_add_executable(kgrtest ${kgrtest_SRCS})

target_link_libraries(kgrtest ${KDE4_KDEUI_LIBS} kdegames kdegamesprivate)

install(TARGETS kgoldrunner kgrtest  ${INSTALL_TARGETS_DEFAULT_ARGS} )






