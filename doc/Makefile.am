AUTOMAKE_OPTIONS = foreign

docdir = ${datadir}/doc/suds
doc_DATA = suds.pdf

EXTRA_DIST = ${doc_DATA}

suds.pdf: suds.ps
	ps2pdf $^ $@

suds.ps: suds.dvi
	dvips -o $@ $^

suds.dvi: suds.tex
	latex suds.tex
	texindex sudsndx.dat
	latex suds.tex

