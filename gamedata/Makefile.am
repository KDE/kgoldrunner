gamedatadir = $(kde_datadir)/kgoldrunner/system
gamedata_DATA = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat

# Untar the file levels.tar and install levels/*.grl files.  Options --mode=644
# --no-same-owner ensure that installer owns files: even if installer is "root".
install:
	$(mkinstalldirs) $(gamedatadir)
	tar --mode=644 --no-same-owner -xf $(srcdir)/levels.tar -C $(gamedatadir)
	cd $(srcdir) && $(INSTALL_DATA) $(gamedata_DATA) $(gamedatadir)
