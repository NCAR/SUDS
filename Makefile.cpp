TOP = ..
LOCALCFLAGS = -Bstatic $(XINCLUDE) $(NETCDFINCLUDE) -DLOADFILE=\"$(RDSSLIBRARIES)/suds.lf\" -DHELPDIR=\"$(ROOT)/suds/help\" $(NETCDFFLAG) 

OBJS =	suds.o analyze.o cape.o class.o color.o contour.o \
	convert.o edit.o eformat.o \
	fgge.o fields.o flags.o fld_derive.o foote.o gale.o hodograph.o \
	init_globals.o jaws.o met_formulas.o mist.o ncar.o netcdf.o nmc.o \
	noaa.o nws.o output.o rsanal.o skewt.o sounding.o util.o xsect.o \
	xyplot.o

all:	suds suds.lf

xsaber:	$(OBJS)
	# load $(CFLAGS) $(OBJS) $(RDSSLIBRARIES)/librdss.a XToolkitLibs XLibrary -ltermcap -lm $(NETCDFLIB) /locallib/gcc-gnulib
	# link
	

install: suds suds.lf
	install -c suds $(RDSSBIN)/suds
	ar r libsuds.a *.o
	ranlib libsuds.a

suds:	$(OBJS)
	$(CC) $(CFLAGS) -o suds $(OBJS) $(RDSSLIBRARIES)/librdss.a XToolkitLibs $(XLIBRARIES) XLibrary -ltermcap -lm $(NETCDFLIB) $(SUNVIEWFLAG)

suds.lf: $(RDSSLIBRARIES)/suds.lf

$(RDSSLIBRARIES)/suds.lf:	suds.state suds.menu keywords.h
	@ cc -E suds.state | grep -v '^# [0-9]' | cat -s > suds.i
	@ uic < make-lf
	@ install -c suds.lf $(RDSSLIBRARIES)/suds.lf
	@ rm -f suds.i

clean:
	rm -f core *.o

coda:
	(CODA=./.codarc; export CODA; coda sources)

depend:
	makedepend $(CFLAGS) *.c

