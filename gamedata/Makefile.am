####### kdevelop will overwrite this part!!! (begin)##########

EXTRA_DIST = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat levels.tar 

mysystemdir = $(kde_datadir)/kgoldrun/system
mysystem_DATA = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat levels.tar

####### kdevelop will overwrite this part!!! (end)############

####### Commands and rules to handle KGoldrunner levels files. #######

####### Uninstall the levels subdirectories and all the levels files. #######
uninstall-local:
	-rm -rf $(DESTDIR)$(kde_datadir)/kgoldrun/system/levels
	-rmdir $(DESTDIR)$(kde_datadir)/kgoldrun/system
	-rmdir $(DESTDIR)$(kde_datadir)/kgoldrun

####### Install the levels files by extracting them from "levels.tar". #######

####### We use the extra optional target "install-exec-local" to avoid #######
####### overwrite by "kdevelop" and a one-line command to ensure "tar" #######
####### gets done in the same SHELL as "cd" (i.e. in the proper dir.). #######

install-exec-local: install-data-local
	-cd $(DESTDIR)$(kde_datadir)/kgoldrun/system && tar xf levels.tar
	-chmod 644 $(DESTDIR)$(kde_datadir)/kgoldrun/system/levels/*
	-chown -R root.root $(DESTDIR)$(kde_datadir)/kgoldrun/system/levels
	-rm -f $(DESTDIR)$(kde_datadir)/kgoldrun/system/levels.tar
