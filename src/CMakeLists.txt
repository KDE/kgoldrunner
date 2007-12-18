
########### next target ###############

set(kgoldrunner_SRCS 
   kgrplayfield.cpp 
   kgrsprite.cpp 
   kgoldrunner.cpp 
   kgrcanvas.cpp 
   kgrtheme.cpp 
   kgrfigure.cpp 
   kgrgame.cpp 
   kgrgameio.cpp 
   kgrobject.cpp 
   main.cpp 
   kgrdialog.cpp )

set(kgoldrunner_icons
	hi16-app-kgoldrunner.png
	hi32-app-kgoldrunner.png
	hi48-app-kgoldrunner.png
	hi64-app-kgoldrunner.png
	hi128-app-kgoldrunner.png
)
kde4_add_win32_app_icon(kgoldrunner_SRCS ${kgoldrunner_icons})
kde4_add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner ${KDE4_KDEUI_LIBS} kdegames )

install(TARGETS kgoldrunner  DESTINATION ${BIN_INSTALL_DIR} )


########### install files ###############

install( FILES KGoldrunner.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kgoldrunner )

kde4_install_icons( ${ICON_INSTALL_DIR} )




