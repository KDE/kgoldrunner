gamedatadir = $(kde_datadir)/kgoldrunner/system
gamedata_DATA = games.dat hi_level.dat hi_plws.dat hi_plwv.dat hi_wad.dat

# Untar the file levels.tar and install levels/*.grl files.  Options --mode=644
# --no-same-owner ensure that installer owns files: even if installer is "root".
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(gamedatadir)
	pwd=`cd $(srcdir) && pwd` ;\
	cd $(DESTDIR)$(gamedatadir) && tar -xf $$pwd/levels.tar ;\
	chown -R root.root levels 2> /dev/null || true ;\
	chmod 644 levels/* 2> /dev/null || true 

