AUTOMAKE_OPTIONS = foreign

docdir = ${datadir}/doc/suds
doc_DATA = suds.pdf

EXTRA_DIST = suds.pdf *.tex *.sty

CLEANFILES = *.aux *.dvi *.log *.ps *.toc sudsndx.dat*

.ps.pdf:
	ps2pdf $<

suds.pdf:

suds.dvi: suds.tex
	touch sudsndx.dats
	latex suds.tex
	texindex sudsndx.dat	# makes sorted file sudsndx.dats
	latex suds.tex

SUDS_DVI = suds.dvi
suds.ps: ${SUDS_DVI}
	dvips -o $@ ${SUDS_DVI}
