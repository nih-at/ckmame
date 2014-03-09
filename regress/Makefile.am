AUTOMAKE_OPTIONS=	parallel-tests

noinst_PROGRAMS=	dbdump dbrestore
AM_CFLAGS=		-I${top_srcdir}/src -I${top_builddir}/src @LIBZIP_CFLAGS@ @SQLITE3_CFLAGS@ @CFLAGS@
dbdump_LDADD=		../src/libckmame.a @SQLITE3_LIBS@
dbrestore_LDADD=	../src/libckmame.a @SQLITE3_LIBS@
noinst_SCRIPTS=		runtest

noinst_LTLIBRARIES=	fwrite.la
fwrite_la_SOURCES=	fwrite.c
fwrite_la_LDFLAGS=	-module -avoid-version

EXTRA_DIST=runtest.in NiHTest.pm ${TESTS} ${ZIPS} ${CHDS} ${DBFILES} \
		gamelist-from-file.in \
		rom-from-superfluous-many-fixdat.fixdat \
		04.rom

CLEANFILES=${DBS} runtest

MAME_DB=mame.db
MAME_DB2=mamedb-lost-parent-ok.db
MAME_DB3=mamedb-disk.db
MAME_DB_8_IS_4=mamedb-1-8-is-4.db
MAME_DB_BADDUMP=mamedb-baddump.db
MAME_DB_CRCDIFF=mamedb-crcdiff.db
MAME_DB_NOCOMMON=mamedb-parent-no-common.db
MAME_DB_SMALL=mamedb-small.db
DBS=${MAME_DB} ${MAME_DB2} ${MAME_DB3} ${MAME_DB_8_IS_4} ${MAME_DB_BADDUMP} ${MAME_DB_CRCDIFF} ${MAME_DB_NOCOMMON} ${MAME_DB_SMALL}

ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=      ZIPCMP=${ZIPCMP} MAME_DB="${MAME_DB}"
TEST_EXTENSIONS= .test
TEST_LOG_COMPILER= ${builddir}/runtest

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
		doublezip.test \
		dumpgame-stats.test \
		dumpgame-unknown.test \
		extra-file.test \
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
		mkmamedb-dir-in-name.test \
		mkmamedb-duplicate-game.test \
		mkmamedb-duplicate-nodump.test \
		mkmamedb-input-dir.test \
		mkmamedb-lost-parent.test \
		mkmamedb-mame.test \
		mkmamedb-mess.test \
		mkmamedb-multi-dats.test \
		mkmamedb-no-name.test \
		mkmamedb-ok.test \
		mkmamedb-parent-crcdiff.test \
		mkmamedb-parent-no-common.test \
		mkmamedb-size-hex.test \
		multi.test \
		needed-cleanup.test \
		needed-error.test \
		needed-exists-different.test \
		needed-exists-same.test \
		needed-keep.test \
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
		replace-broken-rom.test \
		rom-broken-replace.test \
		rom-broken-without-i.test \
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
		rom-from-extra-keep-dir.test \
		rom-from-extra-keep-zip.test \
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
		rom-from-unknown.test \
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
		swap-roms-2.test \
		tochild.test \
		tochildfix.test \
		tz-already.test \
		uncompressed-ckmamedb-changed-new-mtime.test \
		uncompressed-ckmamedb-add-game.test \
		uncompressed-toplevel-extra.test \
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
		nogood-diskgood.test \
		tz-ok.test \
		tz-from-romset.test \
		uncompressed-ckmamedb-changed-same-mtime.test \
		uncompressed-ckmamedb-no-change.test \
		uncompressed-ckmamedb-remove-game.test \
		uncompressed-ckmamedb-remove-game-2.test \
		uncompressed-toplevel-roms.test

ZIPS=		1-4-2.zip \
		1-4-crcerror.zip \
		1-4-dup.zip \
		1-4-garbage.zip \
		1-4-inconsistent.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-4-ok-doublezip.zip \
		1-4-path.zip \
		1-4-wrong.zip \
		1-4-wrong-renamed.zip \
		1-8-broken.zip \
		1-8-long.zip \
		1-8-ok.zip \
		1-a-ok.zip \
		1-c-ok.zip \
		12345678.zip \
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
		pathtest1.zip \
		pathtest2.zip \
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

DBFILES=	\
		detector.xml \
		mamedb-1-8-is-4.txt \
		mamedb-2.txt \
		mamedb-baddump.txt \
		mamedb-broken-sha1.dump \
		mamedb-broken-sha1.txt \
		mamedb-dir-in-name.dump \
		mamedb-dir-in-name.txt \
		mamedb-disk.txt \
		mamedb-duplicate-game.dump \
		mamedb-duplicate-nodump.dump \
		mamedb-duplicate-nodump.txt \
		mamedb-lost-parent-ok.txt \
		mamedb-lost-parent.dump \
		mamedb-lost-parent.txt \
		mamedb-mame.dump \
		mamedb-mame.xml \
		mamedb-mess.dump \
		mamedb-mess.xml \
		mamedb-multi-dats.dump \
		mamedb-no-name.dump \
		mamedb-no-name.txt \
		mamedb-ok.dump \
		mamedb-parent-crcdiff.dump \
		mamedb-parent-crcdiff.txt \
		mamedb-parent-no-common.dump \
		mamedb-parent-no-common.txt \
		mamedb-size-hex.dump \
		mamedb-size-hex.txt \
		mamedb-skipped.txt \
		mamedb-small.dump \
		mamedb-small.txt \
		mamedb.txt

check-am: ${DBS} runtest

${MAME_DB}: ../src/mkmamedb ${srcdir}/mamedb.txt
	../src/mkmamedb ${srcdir}/mamedb.txt

${MAME_DB2}: ../src/mkmamedb ${srcdir}/mamedb-lost-parent-ok.txt
	../src/mkmamedb -o ${MAME_DB2} ${srcdir}/mamedb-lost-parent-ok.txt

${MAME_DB3}: ../src/mkmamedb ${srcdir}/mamedb-disk.txt
	../src/mkmamedb -o ${MAME_DB3} ${srcdir}/mamedb-disk.txt

${MAME_DB_8_IS_4}: ../src/mkmamedb ${srcdir}/mamedb-1-8-is-4.txt
	../src/mkmamedb -o ${MAME_DB_8_IS_4} ${srcdir}/mamedb-1-8-is-4.txt

${MAME_DB_BADDUMP}: ../src/mkmamedb ${srcdir}/mamedb-baddump.txt
	../src/mkmamedb -o ${MAME_DB_BADDUMP} ${srcdir}/mamedb-baddump.txt

${MAME_DB_CRCDIFF}: ../src/mkmamedb ${srcdir}/mamedb-parent-crcdiff.txt
	../src/mkmamedb -o ${MAME_DB_CRCDIFF} ${srcdir}/mamedb-parent-crcdiff.txt

${MAME_DB_NOCOMMON}: ../src/mkmamedb ${srcdir}/mamedb-parent-no-common.txt
	../src/mkmamedb -o ${MAME_DB_NOCOMMON} ${srcdir}/mamedb-parent-no-common.txt

${MAME_DB_SMALL}: ../src/mkmamedb ${srcdir}/mamedb-small.txt
	../src/mkmamedb -o ${MAME_DB_SMALL} ${srcdir}/mamedb-small.txt

update-mamedb.dump: ${MAME_DB} ${MAME_DB2} ${MAME_DB_CRCDIFF} ${MAME_DB_NOCOMMON} dbdump ../src/mkmamedb
	./dbdump ${MAME_DB} > mamedb-ok.dump
	./dbdump ${MAME_DB2} > mamedb-lost-parent.dump
	../src/mkmamedb -o mame.db.$$ ${srcdir}/mamedb.txt ${srcdir}/mamedb-lost-parent-ok.txt && ./dbdump mame.db.$$ > mamedb-duplicate-game.dump && rm mame.db.$$
	./dbdump ${MAME_DB_CRCDIFF} > mamedb-parent-crcdiff.dump
	./dbdump ${MAME_DB_NOCOMMON} > mamedb-parent-no-common.dump
	set -e \
	f=$$$$.db; \
	../src/mkmamedb -o "$$f".db \
			${srcdir}/mamedb.txt ${srcdir}/mamedb-2.txt \
		&& ./dbdump "$$f".db > mamedb-multi-dats.dump \
		&& rm "$$f".db

runtest: runtest.in
	sed -e 's!@[s]rcdir@!${srcdir}!g' -e 's!@[a]bs_srcdir@!${abs_srcdir}!g' ${srcdir}/runtest.in > runtest
	chmod +x runtest

cleanup:
	rm -rf ${builddir}/sandbox-*.d[0-9]*/
