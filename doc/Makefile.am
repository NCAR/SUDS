AUTOMAKE_OPTIONS = foreign

docdir = ${datadir}/doc/suds
doc_DATA = suds.pdf

EXTRA_DIST = ${doc_DATA}

CLEANFILES = *.aux *.dvi *.log *.ps *.toc sudsndx.dat*

suds.pdf: suds.ps
	ps2pdf $^ $@

suds.ps: suds.dvi
	dvips -o $@ $^

suds.dvi: suds.tex
	touch sudsndx.dats
	latex suds.tex
	texindex sudsndx.dat	# makes sorted file sudsndx.dats
	latex suds.tex

