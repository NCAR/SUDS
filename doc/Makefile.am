AUTOMAKE_OPTIONS = foreign

docdir = ${datadir}/doc/suds
doc_DATA = suds.pdf

EXTRA_DIST = suds.pdf *.tex *.sty

CLEANFILES = *.aux *.dvi *.log *.ps *.toc sudsndx.dat*

suds.pdf: suds.ps
	ps2pdf $^ $@

suds.dvi: suds.tex
	touch sudsndx.dats
	latex suds.tex
	texindex sudsndx.dat	# makes sorted file sudsndx.dats
	latex suds.tex

suds.ps: suds.dvi
	dvips -o $@ $^
