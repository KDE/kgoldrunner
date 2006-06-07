



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kgoldrunner/system FILES games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat )

# no chmod:chown stuff which was in original Makefile.am
# is it needed? dimsuz
install( CODE "EXEC_PROGRAM(tar ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR}/kgoldrunner/system
ARGS -xf ${CMAKE_CURRENT_SOURCE_DIR}/levels.tar)" )


#original Makefile.am contents follow:

#gamedatadir = $(kde_datadir)/kgoldrunner/system
#gamedata_DATA = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat
#
## Untar the file levels.tar and install levels/*.grl files.  The "chown" and
## "chmod" ensure that installer owns files: even if installer is "root".  The
## "|| true" commands make everything succeed when the installer is NOT "root".
#install-data-local:
#	$(mkinstalldirs) $(DESTDIR)$(gamedatadir)
#	pwd=`cd $(srcdir) && pwd` ;     \
#	cd $(DESTDIR)$(gamedatadir) && tar -xf $$pwd/levels.tar ;\
#	chown -R root:root levels 2> /dev/null || true ;\
#	chmod 644 levels/* 2> /dev/null || true 
#
