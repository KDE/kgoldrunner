####### kdevelop will overwrite this part!!! (begin)##########

SUBDIRS = kgoldrunner 

EXTRA_DIST = kgoldrunner.kdevprj AUTHORS COPYING ChangeLog INSTALL README TODO kgoldrunner.lsm BUGS NEWS VERSION 

package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

init:
	####### Adjust the source code for Qt1 or Qt2 MOC compiler.  #######
	cd $(top_srcdir)/kgoldrunner && ./fix_src "$(VERSION)"
