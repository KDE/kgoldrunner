####### kdevelop will overwrite this part!!! (begin)##########


EXTRA_DIST = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat levels.tar 

install-data-local:
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/games.dat $(kde_datadir)/kgoldrun/system/games.dat
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/hi_level.dat $(kde_datadir)/kgoldrun/system/hi_level.dat
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/hi_plws.dat $(kde_datadir)/kgoldrun/system/hi_plws.dat
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/hi_plwv.dat $(kde_datadir)/kgoldrun/system/hi_plwv.dat
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/hi_wad.dat $(kde_datadir)/kgoldrun/system/hi_wad.dat
	$(mkinstalldirs) $(kde_datadir)/kgoldrun/system/
	$(INSTALL_DATA) $(srcdir)/levels.tar $(kde_datadir)/kgoldrun/system/levels.tar

uninstall-local:
	-rm -f $(kde_datadir)/kgoldrun/system/games.dat
	-rm -f $(kde_datadir)/kgoldrun/system/hi_level.dat
	-rm -f $(kde_datadir)/kgoldrun/system/hi_plws.dat
	-rm -f $(kde_datadir)/kgoldrun/system/hi_plwv.dat
	-rm -f $(kde_datadir)/kgoldrun/system/hi_wad.dat
	-rm -f $(kde_datadir)/kgoldrun/system/levels.tar

####### kdevelop will overwrite this part!!! (end)############

####### Commands and rules to handle KGoldrunner levels files. #######

####### Uninstall the levels subdirectories and all the levels files. #######
	-rm -rf $(kde_datadir)/kgoldrun/system/levels
	-rmdir $(kde_datadir)/kgoldrun/system
	-rmdir $(kde_datadir)/kgoldrun

####### Install the levels files by extracting them from "levels.tar". #######

####### We use the extra optional target "install-exec-local" to avoid #######
####### overwrite by "kdevelop" and a one-line command to ensure "tar" #######
####### gets done in the same SHELL as "cd" (i.e. in the proper dir.). #######

install-exec-local: install-data-local
	-cd $(kde_datadir)/kgoldrun/system && tar xf levels.tar
	-rm -f $(kde_datadir)/kgoldrun/system/levels.tar
