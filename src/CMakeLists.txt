
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

add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner KF5KDEGames
        KF5KDEGamesPrivate
        KF5::XmlGui
        KF5::I18n
        Qt5::Xml
        KF5::KIOCore)

install(TARGETS kgoldrunner  ${INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install( PROGRAMS KGoldrunner.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/kgoldrunner )

ecm_install_icons(ICONS "${CMAKE_CURRENT_SOURCE_DIR}/16-apps-kgoldrunner.png"
                        "${CMAKE_CURRENT_SOURCE_DIR}/22-apps-kgoldrunner.png"
                        "${CMAKE_CURRENT_SOURCE_DIR}/32-apps-kgoldrunner.png"
                        "${CMAKE_CURRENT_SOURCE_DIR}/48-apps-kgoldrunner.png"
                        "${CMAKE_CURRENT_SOURCE_DIR}/64-apps-kgoldrunner.png"
                        "${CMAKE_CURRENT_SOURCE_DIR}/128-apps-kgoldrunner.png"
                  DESTINATION ${ICON_INSTALL_DIR} )
