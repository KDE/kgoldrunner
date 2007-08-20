



########### install files ###############

install( FILES README
	       egypt.desktop
               default.desktop
               kgr_geek.desktop
               kgoldrunner.desktop
               apple_2.desktop
               midnight.desktop
               nostalgia.desktop
               nostalgia-blues.desktop
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes )

install( FILES default/set.svg default/actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/default )

install( FILES kgr_geek/set.svgz kgr_geek/actors.svgz
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/kgr_geek )

install( FILES nostalgia/blue-set.svg nostalgia/blue-actors.svg 
               nostalgia/set.svg nostalgia/actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/nostalgia )

install( FILES egypt/egypt_kgr.svg egypt/actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/egypt )
