
########### next target ###############

set(kgoldrunner_SRCS 
   kgrplayfield.cpp 
   kgrsprite.cpp 
   kgoldrunner.cpp 
   kgrcanvas.cpp 
   kgrtheme.cpp 
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
   kgrdialog.cpp )

if(KGOLDRUNNER_OPENAL)
    set(kgoldrunner_SRCS
        ${kgoldrunner_SRCS}
        kgrsounds.cpp 
        TagaroAudio/audioscene.cpp
        TagaroAudio/sound.cpp )
else(KGOLDRUNNER_OPENAL)
#    KGoldrunner sound is supported only with OpenAL, else silent (2011-05-09).
#    set(kgoldrunner_SRCS
#        ${kgoldrunner_SRCS}
#        kgrsoundbank.cpp )
endif(KGOLDRUNNER_OPENAL)

kde4_add_app_icon(kgoldrunner_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/hi*-app-kgoldrunner.png")
kde4_add_executable(kgoldrunner ${kgoldrunner_SRCS})

if(KGOLDRUNNER_OPENAL)
    target_link_libraries(kgoldrunner ${KDE4_KDEUI_LIBS} kdegames
                                      ${OPENAL_LIBRARY} ${SNDFILE_LIBRARIES} )
else(KGOLDRUNNER_OPENAL)
#    KGoldrunner sound is supported only with OpenAL, else silent (2011-05-09).
#    target_link_libraries(kgoldrunner ${KDE4_KDEUI_LIBS} kdegames
#                                      ${KDE4_PHONON_LIBS})
    target_link_libraries(kgoldrunner ${KDE4_KDEUI_LIBS} kdegames)
endif(KGOLDRUNNER_OPENAL)

install(TARGETS kgoldrunner  ${INSTALL_TARGETS_DEFAULT_ARGS} )


########### install files ###############

install( PROGRAMS KGoldrunner.desktop  DESTINATION  ${XDG_APPS_INSTALL_DIR} )
install( FILES kgoldrunnerui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kgoldrunner )

kde4_install_icons( ${ICON_INSTALL_DIR} )





