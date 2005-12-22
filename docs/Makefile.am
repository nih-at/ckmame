pkgdocdir=${prefix}/share/doc/@PACKAGE@

pkgdoc_DATA=mame-0.96.dtd mess-0.97.dtd

CLEANFILES=	${HTML} mkmamedb.1

EXTRA_DIST=	${MANDOC} ${MANDOC2} ${MAN} ${pkgdoc_DATA}

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
	nroff -mdoc2html $< | sed -e "s,../html[0-9]/,," > $@.$$$$ && mv $@.$$$$ $@

update-man: ${MAN}
update-html: ${HTML}
