CLEANFILES=	${HTML} mkmamedb.1

EXTRA_DIST=	${MANDOC} ${MANDOC2} ${MAN}

MANFMT=@MANFMT@
SUFFIXES=.man .mdoc .html
.PHONY: update-man update-html

man1_MANS=${MANDOC:.mdoc=.${MANFMT}} mkmamedb.1

MANDOC=ckmame.mdoc dumpgame.mdoc
MANDOC2=mkmamedb.mdoc
ALLMANDOC=${MANDOC} ${MANDOC2}
MAN=${ALLMANDOC:.mdoc=.man}
HTML=${ALLMANDOC:.mdoc=.html}

mkmamedb.1: mkmamedb.${MANFMT}
	sed 's,PREFIX,${prefix},g' ${srcdir}/mkmamedb.${MANFMT} > $@

.mdoc.man:
	mdoc2man $< > $@.$$$$ && diff -I NiH $@ $@.$$$$ || mv $@.$$$$ $@; \
	rm -f $@.$$$$

.mdoc.html:
	nroff -mdoc2html $< | sed -e "s,../html1/,," > $@.$$$$ && mv $@.$$$$ $@

update-man: ${MAN}
update-html: ${HTML}
