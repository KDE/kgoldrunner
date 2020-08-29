



########### install files ###############

install( FILES README
	       black-on-white.desktop
	       egypt.desktop
               default.desktop
               kgr_geek.desktop
               nostalgia.desktop
               nostalgia-blues.desktop
               DESTINATION ${KDE_INSTALL_DATADIR}/kgoldrunner/themes )

install( FILES 
	default/set.svg default/actors.svg
        default/default.png
 	default/gold.ogg default/falling.ogg default/step.wav default/climb.wav
 	default/dig.ogg default/ladder.ogg default/completed.ogg
 	default/death.ogg default/gameover.ogg default/victory.ogg
	DESTINATION 
	${KDE_INSTALL_DATADIR}/kgoldrunner/themes/default )

install( FILES accessible/black-on-white-set.svg
               accessible/black-on-white-actors.svg
               accessible/black-on-white.png
               DESTINATION ${KDE_INSTALL_DATADIR}/kgoldrunner/themes/accessible )

install( FILES kgr_geek/set.svgz kgr_geek/actors.svgz
               kgr_geek/kgr_geek.png
               DESTINATION ${KDE_INSTALL_DATADIR}/kgoldrunner/themes/kgr_geek )

install( FILES nostalgia/blue-set.svg nostalgia/blue-actors.svg 
               nostalgia/set.svg nostalgia/actors.svg
               nostalgia/nostalgia.png
               nostalgia/nostalgia-blues.png
               DESTINATION ${KDE_INSTALL_DATADIR}/kgoldrunner/themes/nostalgia )

install( FILES egypt/egypt_kgr.svgz egypt/actors.svgz
               egypt/egypt.png
               DESTINATION ${KDE_INSTALL_DATADIR}/kgoldrunner/themes/egypt )

install( FILES kgoldrunner.knsrc  DESTINATION  ${KDE_INSTALL_DATADIR}/knsrcfiles )

