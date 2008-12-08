AUTOMAKE_OPTIONS = foreign

SUBDIRS = doc help

bin_PROGRAMS = suds

sudslibdir = ${libdir}/suds
sudslib_DATA = suds.lf

suds_SOURCES = suds.c \
	analyze.c cape.c class.c color.c contour.c convert.c \
	drexel.c edit.c eformat.c fgge.c fields.c flags.c fld_derive.c \
	foote.c gale.c hodograph.c init_globals.c interpolate.c \
	jaws.c mist.c ncar.c netcdf.c nmc.c noaa.c nssl.c nws.c \
	output.c rsanal.c skewt.c sounding.c spline.c util.c \
	windstep.c wmo.c xsect.c xyplot.c \
	\
	color.h fields.h flags.h globals.h keywords.h sounding.h

INCLUDES = -DLOADFILE=\"${sudslibdir}/suds.lf\" -DHELPDIR=\"${sudslibdir}/help\"

#
# UI loadfile stuff
#
suds.lf: suds.state suds.menu keywords.h
	$(RM) suds.lf
	cat suds.state | $(CPP) - | grep -v '^# [0-9]' | cat -s > ./suds.i
	@RDSS_DIR@/bin/uic < ./make-lf
	$(RM) suds.i

EXTRA_DIST = suds.menu suds.state *.dat make-lf
CLEANFILES = suds.lf
