CFLAGS = -I/rdss/include -g -Bstatic
XLIBS = -lXaw -lXmu -lXt -lXext -lX11 -lsuntool -lsunwindow -lpixrect
CC = gcc

OBJS =	suds.o analyze.o class.o color.o contour.o convert.o edit.o eformat.o \
	fgge.o fields.o flags.o fld_derive.o foote.o gale.o hodograph.o \
	init_globals.o jaws.o met_formulas.o mist.o ncar.o netcdf.o nmc.o \
	noaa.o nws.o output.o rsanal.o skewt.o sounding.o util.o xsect.o \
	xyplot.o

all:	suds /locallib/suds.lf

test:	sudstest /locallib/suds.lf

install: suds
	install -c suds /localbin/suds
	ar r libsuds.a *.o
	ranlib libsuds.a

suds:	$(OBJS)
	$(CC) $(CFLAGS) -o suds $(OBJS) -lrdss -lnetcdf $(XLIBS) -ltermcap -lm

suds.lf:	/locallib/suds.lf

/locallib/suds.lf:	suds.state suds.menu keywords.h
	@ cp /locallib/suds.lf /locallib/suds.lf~
	@ cc -P suds.state
	@ suds -n make-lf
	@ rm -f suds.i

clean:
	rm -f core *.o

Makefile: Makefile.cpp
	mv Makefile Makefile~
	cp Makefile.cpp Makefile.c
	echo "# DO NOT EDIT!  EDIT Makefile.cpp INSTEAD" > Makefile
	cc -E Makefile.c >> Makefile
	rm -f Makefile.c
	make depend

coda:
	(CODA=./.codarc; export CODA; coda sources)

depend:
	makedepend $(CFLAGS) *.c
