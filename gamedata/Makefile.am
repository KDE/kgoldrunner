gamedatadir = $(kde_datadir)/kgoldrunner/system
gamedata_DATA = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat

install:
	$(mkinstalldirs) $(gamedatadir)
	tar -xf $(srcdir)/levels.tar -C $(gamedatadir)
	cd $(srcdir) && $(INSTALL_DATA) $(gamedata_DATA) $(gamedatadir)
	chmod 644 $(gamedatadir)/levels/*
	chown -R root.root $(gamedatadir)/levels
