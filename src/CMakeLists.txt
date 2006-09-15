
include_directories( ${CMAKE_SOURCE_DIR}/libkdegames ${QDBUS_INCLUDE_DIRS} )

########### next target ###############

set(kgoldrunner_SRCS 
   kgrscene.cpp 
   kgrsprite.cpp 
   kgoldrunner.cpp 
   kgrcanvas.cpp 
   kgrfigure.cpp 
   kgrgame.cpp 
   kgrobject.cpp 
   main.cpp 
   kgrdialog.cpp 
   kgrgamecanvas.cpp )

kde4_automoc(${kgoldrunner_SRCS})

kde4_add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner  ${KDE4_KDEUI_LIBS} kdegames ${KDE4_KDEPRINT_LIBS} )

install(TARGETS kgoldrunner  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES KGoldrunner.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kgoldrunner )

kde4_install_icons( ${ICON_INSTALL_DIR}   )




