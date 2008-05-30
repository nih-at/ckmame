noinst_PROGRAMS=	dbdump
AM_CFLAGS=		-I${srcdir}/../src @SQLITE3_CFLAGS@ @CFLAGS@
dbdump_LDADD=		../src/libckmame.a @SQLITE3_LIBS@

EXTRA_DIST=runtest ${TESTS} ${ZIPS} ${CHDS} ${DBFILES} \
		gamelist-from-file.in

CLEANFILES=${DBS}

MAME_DB=mame.db
MAME_DB2=mame2.db
MAME_DB3=mame3.db
DBS=${MAME_DB} ${MAME_DB2} ${MAME_DB3}

ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=ZIPCMP="${ZIPCMP}" MAME_DB="${MAME_DB}" $(srcdir)/runtest

TESTS=	\
		archive-broken.test \
		archive-broken2.test \
		baddump.test \
		cleanup.test \
		cleanup-disk.test \
		cleanup-implicit.test \
		delete-used-superfluous.test \
		disk-from-extra-remove.test \
		disk-from-superfluous-all.test \
		disk-from-superfluous.test \
		disk-merge.test \
		disk-miss.test \
		disk-noext.test \
		disk-nogood.test \
		disk-nogoodmiss.test \
		disk-ok.test \
		disk-parent.test \
		disk-superfluous.test \
		disk-swap.test \
		disk-to-needed.test \
		disk-wrong.test \
		disk-wrong2.test \
		double.test \
		dumpgame-unknown.test \
		gamelist-from-file.test \
		garbage.test \
		garbage2.test \
		hasherr-c.test \
		hasherr-extra.test \
		hasherr-no-i.test \
		hasherr-p.test \
		hasherr-pc.test \
		hasherr.test \
		hasherr2.test \
		ignore-unknown.test \
		inchild.test \
		inconsistent-with-i.test \
		inconsistent-without-i.test \
		inparent.test \
		mid.test \
		misnamed-dryrun.test \
		misnamed.test \
		mkmamedb-duplicate-game.test \
		mkmamedb-lost-parent.test \
		mkmamedb-multi-dats.test \
		mkmamedb-ok.test \
		multi.test \
		needed-error.test \
		needed.test \
		needed2.test \
		needed3.test \
		nogood-miss.test \
		nogood.test \
		nonexist.test \
		norom.test \
		ok.test \
		old-complete.test \
		old-disk.test \
		old-keep.test \
		old-partial.test \
		punknown.test \
		punused.test \
		rom-broken-replace.test \
		rom-broken.test \
		rom-duplicate-1st.test \
		rom-duplicate-2nd.test \
		rom-duplicate-ok.test \
		rom-duplicate-wrong.test \
		rom-from-extra-broken.test \
		rom-from-extra-child.test \
		rom-from-extra-inconsistent.test \
		rom-from-extra-keep.test \
		rom-from-extra-partial.test \
		rom-from-extra-remove.test \
		rom-from-needed.test \
		rom-from-needed2.test \
		rom-from-romset.test \
		rom-from-romset2.test \
		rom-from-superfluous-cleanup.test \
		rom-from-superfluous-dryrun.test \
		rom-from-superfluous-some.test \
		rom-from-superfluous.test \
		rom-many.test \
		sample-miss.test \
		sample-ok.test \
		sample-parent-ok.test \
		sample-superfluous.test \
		skip.test \
		skip-full.test \
		skipbad.test \
		superfluous-broken.test \
		superfluous-cleanup.test \
		superfluous-inconsistent.test \
		superfluous.test \
		swap.test \
		tochildfix.test \
		tz-ok.test \
		tz-from-romset.test \
		wrong.test \
		zero.test \
		zero-good.test \
		zero-miss.test \
		zero-superfluous.test
BROKEN_TESTS=	\
		disk-swapmerge.test \
		identical-once.test \
		rom-from-superfluous-many.test \
		tochild.test \
		unused.test \
		wrong2.test

ZIPS=		1-4-2.zip \
		1-4-dup.zip \
		1-4-garbage.zip \
		1-4-inconsistent.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-8-broken.zip \
		1-8-ok.zip \
		1-a-ok.zip \
		1-c-ok.zip \
		2-44-ok.zip \
		2-48-broken.zip \
		2-48-ok.zip \
		2-4a-ok.zip \
		2-4c-ok.zip \
		2-84.zip \
		2-cc-ok.zip \
		deadbeef.zip \
		deadclonedbeef.zip \
		deadfish.zip \
		deadfish4.zip \
		deadpork.zip \
		garbage.zip \
		garbage2.zip \
		many.zip \
		samptest.zip \
		samptest2.zip \
		samptest3.zip \
		samptest3-garbage.zip \
		zero-4-ok.zip \
		zero-ok.zip

CHDS=		108-2.chd \
		108-5.chd \
		108-5a.chd \
		12-1.chd \
		diff.chd

DBFILES=	mamedb.txt \
		mamedb-2.txt \
		mamedb-disk.txt \
		mamedb-lost-parent.txt \
		mamedb-lost-parent-ok.txt \
		mkmamedb-duplicate-game.dump \
		mkmamedb-lost-parent.dump \
		mkmamedb-multi-dats.dump \
		mkmamedb-ok.dump \
		skip.xml \
		skipped.txt

check-am: ${DBS}

${MAME_DB}: ../src/mkmamedb ${srcdir}/mamedb.txt
	../src/mkmamedb ${srcdir}/mamedb.txt

${MAME_DB2}: ../src/mkmamedb ${srcdir}/mamedb-lost-parent-ok.txt
	../src/mkmamedb -o ${MAME_DB2} ${srcdir}/mamedb-lost-parent-ok.txt
${MAME_DB3}: ../src/mkmamedb ${srcdir}/mamedb-disk.txt
	../src/mkmamedb -o ${MAME_DB3} ${srcdir}/mamedb-disk.txt

update-mkmamedb.dump: ${MAME_DB}
	./dbdump ${MAME_DB} > mkmamedb-ok.dump
	./dbdump ${MAME_DB2} > mkmamedb-lost-parent.dump
	./dbdump ${MAME_DB} > mkmamedb-ok.dump
	../src/mkmamedb -o $$$$.db \
			${srcdir}/mamedb.txt ${srcdir}/mamedb-2.txt \
		&& ./dbdump $$$$.db > mkmamedb-multi-dats.dump \
		&& rm $$$$.db
