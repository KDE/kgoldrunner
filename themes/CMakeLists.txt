



########### install files ###############

install( FILES README
               default.desktop
               kgr_geek.desktop
               kgoldrunner.desktop
               apple_2.desktop
               midnight.desktop
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes )

install( FILES default/set.svg default/actors.svg
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/default )

install( FILES kgr_geek/set.svgz kgr_geek/actors.svgz
               DESTINATION ${DATA_INSTALL_DIR}/kgoldrunner/themes/kgr_geek )

