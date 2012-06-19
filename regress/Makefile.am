noinst_PROGRAMS=	dbdump
AM_CFLAGS=		-I${srcdir}/../src @SQLITE3_CFLAGS@ @CFLAGS@
dbdump_LDADD=		../src/libckmame.a @SQLITE3_LIBS@

EXTRA_DIST=runtest ${TESTS} ${ZIPS} ${CHDS} ${DBFILES} \
		gamelist-from-file.in \
		rom-from-superfluous-many-fixdat.fixdat

CLEANFILES=${DBS}

MAME_DB=mame.db
MAME_DB2=mame2.db
MAME_DB3=mame3.db
MAME_DB4=mame4.db
MAME_DB_8_IS_4=mame-1-8-is-4.db
MAME_DB_BADDUMP=mame-baddump.db
MAME_DB_CRCDIFF=mame-crcdiff.db
DBS=${MAME_DB} ${MAME_DB2} ${MAME_DB3} ${MAME_DB4} ${MAME_DB_8_IS_4} ${MAME_DB_BADDUMP} ${MAME_DB_CRCDIFF}

ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=ZIPCMP="${ZIPCMP}" MAME_DB="${MAME_DB}" $(srcdir)/runtest

TESTS=	\
		archive-broken.test \
		archive-broken2.test \
		baddump.test \
		ckmame.test \
		cleanup.test \
		cleanup-disk.test \
		cleanup-implicit.test \
		delete-used-superfluous.test \
		disk-chd-v5-integrity.test \
		disk-from-extra-remove.test \
		disk-from-extra.test \
		disk-from-superfluous-all.test \
		disk-from-superfluous.test \
		disk-merge.test \
		disk-miss.test \
		disk-noext.test \
		disk-nogood.test \
		disk-nogood-copy.test \
		disk-nogood-miss.test \
		disk-ok.test \
		disk-parent.test \
		disk-superfluous.test \
		disk-swap.test \
		disk-to-needed.test \
		disk-wrong.test \
		disk-wrong2.test \
		double.test \
		dumpgame-stats.test \
		dumpgame-unknown.test \
		extra-size-mismatch.test \
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
		identical-once.test \
		ignore-unknown.test \
		inchild.test \
		inconsistent-with-i.test \
		inconsistent-without-i.test \
		inparent.test \
		mid.test \
		misnamed-dryrun.test \
		misnamed.test \
		mkmamedb-broken-sha1.test \
		mkmamedb-duplicate-game.test \
		mkmamedb-duplicate-nodump.test \
		mkmamedb-lost-parent.test \
		mkmamedb-mame.test \
		mkmamedb-mess.test \
		mkmamedb-multi-dats.test \
		mkmamedb-no-name.test \
		mkmamedb-ok.test \
		mkmamedb-parent-crcdiff.test \
		mkmamedb-size-hex.test \
		multi.test \
		needed-cleanup.test \
		needed-error.test \
		needed-exists-different.test \
		needed-exists-same.test \
		needed.test \
		nogood-allmissing.test \
		nogood-miss-d.test \
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
		rom-dir.test \
		rom-duplicate-1st.test \
		rom-duplicate-2nd.test \
		rom-duplicate-ok.test \
		rom-duplicate-wrong.test \
		rom-from-extra-broken.test \
		rom-from-extra-broken-unwanted.test \
		rom-from-extra-child.test \
		rom-from-extra-detector.test \
		rom-from-extra-inconsistent.test \
		rom-from-extra-keep.test \
		rom-from-extra-partial.test \
		rom-from-extra-remove.test \
		rom-from-needed.test \
		rom-from-needed2.test \
		rom-from-romset.test \
		rom-from-romset2.test \
		rom-from-superfluous-cleanup.test \
		rom-from-superfluous-detector.test \
		rom-from-superfluous-dryrun.test \
		rom-from-superfluous-many.test \
		rom-from-superfluous-many-fixdat.test \
		rom-from-superfluous-some.test \
		rom-from-superfluous.test \
		rom-many.test \
		rom-two-long.test \
		rom-unused.test \
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
		swap-roms.test \
		tochild.test \
		tochildfix.test \
		tz-already.test \
		tz-ok.test \
		tz-from-romset.test \
		wrong.test \
		wrong2.test \
		zero.test \
		zero-good.test \
		zero-long.test \
		zero-long-badcrc.test \
		zero-miss.test \
		zero-superfluous.test
BROKEN_TEST=	\
		disk-swapmerge.test \
		rom-from-unknown.test

ZIPS=		1-4-2.zip \
		1-4-crcerror.zip \
		1-4-dup.zip \
		1-4-garbage.zip \
		1-4-inconsistent.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-4-path.zip \
		1-4-wrong.zip \
		1-8-broken.zip \
		1-8-long.zip \
		1-8-ok.zip \
		1-a-ok.zip \
		1-c-ok.zip \
		2-44-ok.zip \
		2-48-broken.zip \
		2-48-long.zip \
		2-48-ok.zip \
		2-48-ok.tzip \
		2-4a-ok.zip \
		2-4c-ok.zip \
		2-84.zip \
		2-cc-ok.zip \
		baddump.zip \
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
		zero-long.zip \
		zero-ok.zip

CHDS=		108-2.chd \
		108-5.chd \
		108-5a.chd \
		12-1.chd \
		512v5.chd \
		diff.chd

DBFILES=	mamedb.txt \
		mamedb-1-8-is-4.txt \
		mamedb-2.txt \
		mamedb-baddump.txt \
		mamedb-disk.txt \
		mamedb-multi-dats.dump \
		mamedb-ok.dump \
		mkmamedb-broken-sha1.dump \
		mkmamedb-broken-sha1.txt \
		mkmamedb-duplicate-game.dump \
		mkmamedb-duplicate-nodump.dump \
		mkmamedb-duplicate-nodump.txt \
		mkmamedb-lost-parent-ok.txt \
		mkmamedb-lost-parent.dump \
		mkmamedb-lost-parent.txt \
		mkmamedb-mame.dump \
		mkmamedb-mame.xml \
		mkmamedb-mess.dump \
		mkmamedb-mess.xml \
		mkmamedb-no-name.dump \
		mkmamedb-no-name.txt \
		mkmamedb-parent-crcdiff.dump \
		mkmamedb-parent-crcdiff.txt \
		mkmamedb-size-hex.dump \
		mkmamedb-size-hex.txt \
		skip.xml \
		skipped.txt

check-am: ${DBS}

${MAME_DB}: ../src/mkmamedb ${srcdir}/mamedb.txt
	../src/mkmamedb ${srcdir}/mamedb.txt

${MAME_DB2}: ../src/mkmamedb ${srcdir}/mkmamedb-lost-parent-ok.txt
	../src/mkmamedb -o ${MAME_DB2} ${srcdir}/mkmamedb-lost-parent-ok.txt

${MAME_DB3}: ../src/mkmamedb ${srcdir}/mamedb-disk.txt
	../src/mkmamedb -o ${MAME_DB3} ${srcdir}/mamedb-disk.txt

${MAME_DB4}: ../src/mkmamedb ${srcdir}/mamedb.txt ${srcdir}/mkmamedb-lost-parent-ok.txt
	../src/mkmamedb -o ${MAME_DB4} ${srcdir}/mamedb.txt ${srcdir}/mkmamedb-lost-parent-ok.txt

${MAME_DB_8_IS_4}: ../src/mkmamedb ${srcdir}/mamedb-1-8-is-4.txt
	../src/mkmamedb -o ${MAME_DB_8_IS_4} ${srcdir}/mamedb-1-8-is-4.txt

${MAME_DB_BADDUMP}: ../src/mkmamedb ${srcdir}/mamedb-baddump.txt
	../src/mkmamedb -o ${MAME_DB_BADDUMP} ${srcdir}/mamedb-baddump.txt

${MAME_DB_CRCDIFF}: ../src/mkmamedb ${srcdir}/mkmamedb-parent-crcdiff.txt
	../src/mkmamedb -o ${MAME_DB_CRCDIFF} ${srcdir}/mkmamedb-parent-crcdiff.txt

update-mamedb.dump: ${MAME_DB} ${MAME_DB2} ${MAME_DB4}
	./dbdump ${MAME_DB} > mamedb-ok.dump
	./dbdump ${MAME_DB2} > mkmamedb-lost-parent.dump
	./dbdump ${MAME_DB4} > mkmamedb-duplicate-game.dump
	./dbdump ${MAME_DB_CRCDIFF} > mkmamedb-parent-crcdiff.dump
	set -e \
	f=$$$$.db; \
	../src/mkmamedb -o "$$f".db \
			${srcdir}/mamedb.txt ${srcdir}/mamedb-2.txt \
		&& ./dbdump "$$f".db > mamedb-multi-dats.dump \
		&& rm "$$f".db
