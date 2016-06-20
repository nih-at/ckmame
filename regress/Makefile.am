SUFFIXES=.db
.txt.db:
	../src/mkmamedb -o $@ $<

AUTOMAKE_OPTIONS=	parallel-tests

noinst_PROGRAMS=	dbdump dbrestore
AM_CFLAGS=		-I${top_srcdir}/src -I${top_builddir}/src @LIBZIP_CFLAGS@ @SQLITE3_CFLAGS@ @CFLAGS@
dbdump_LDADD=		../src/libckmame.a @SQLITE3_LIBS@
dbrestore_LDADD=	../src/libckmame.a @SQLITE3_LIBS@
noinst_SCRIPTS=		runtest

pkglib_LTLIBRARIES=	fwrite.la
fwrite_la_SOURCES=	fwrite.c
fwrite_la_LDFLAGS=	-module -avoid-version

EXTRA_DIST=runtest.in CkmameDB.pm NiHTest.pm ${TESTS} ${ZIPS} ${CHDS} ${DBFILES} \
		gamelist-from-file.in \
		fixdat-missing-all.dat \
		fixdat-missing-child.dat \
		fixdat-missing-parent.dat \
		fixdat-missing-part.dat \
		rom-from-superfluous-many-fixdat.dat \
		04.rom

CLEANFILES=${DBS} runtest

ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=      ZIPCMP=${ZIPCMP} MAME_DB="${MAME_DB}"
TEST_EXTENSIONS= .test
TEST_LOG_COMPILER= ${builddir}/runtest

TESTS=	\
		archive-broken.test \
		archive-broken2.test \
		baddump.test \
		ckmame.test \
		cleanup-disk.test \
		cleanup-implicit.test \
		cleanup.test \
		delete-used-superfluous.test \
		detector-interaction-with-headerless.test \
		detector-prefer-file-with-header.test \
		disk-chd-v5-integrity.test \
		disk-conflict.test \
		disk-from-extra-remove.test \
		disk-from-extra.test \
		disk-from-superfluous-all.test \
		disk-from-superfluous.test \
		disk-merge.test \
		disk-miss.test \
		disk-noext.test \
		disk-nogood-copy.test \
		disk-nogood-miss.test \
		disk-nogood.test \
		disk-ok.test \
		disk-parent.test \
		disk-superfluous.test \
		disk-swap.test \
		disk-swapmerge.test \
		disk-to-needed.test \
		disk-wrong.test \
		disk-wrong2.test \
		diskfull-dir.test \
		diskfull-zip.test \
		doublezip.test \
		dumpgame-stats.test \
		dumpgame-unknown.test \
		extra-file-dir.test \
		extra-file-zip.test \
		extra-size-mismatch.test \
		fixdat-missing-all.test \
		fixdat-missing-child.test \
		fixdat-missing-parent.test \
		fixdat-missing-part.test \
		gamelist-from-file.test \
		garbage-archive-exists.test \
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
		misnamed-nofix.test \
		misnamed.test \
		mkmamedb-broken-sha1.test \
		mkmamedb-dir-in-name.test \
		mkmamedb-duplicate-game.test \
		mkmamedb-duplicate-nodump.test \
		mkmamedb-duplicate-rom-name.test \
		mkmamedb-input-dir-dir.test \
		mkmamedb-input-dir-zip.test \
		mkmamedb-lost-parent.test \
		mkmamedb-mame.test \
		mkmamedb-merge-no-parent.test \
		mkmamedb-merge-three-generations.test \
		mkmamedb-merge-wrong-name.test \
		mkmamedb-mess.test \
		mkmamedb-multi-dats.test \
		mkmamedb-no-name.test \
		mkmamedb-ok.test \
		mkmamedb-parent-crcdiff-mame.test \
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
		nogood-allmissing-suppress.test \
		nogood-allmissing.test \
		nogood-diskgood.test \
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
		rom-add-from-crc-different.test \
		rom-add-from-crc-only.test \
		rom-broken-replace.test \
		rom-broken-without-i.test \
		rom-broken.test \
		rom-dir.test \
		rom-dir-in-extra-dirs-subdir-dir.test \
		rom-dir-in-extra-dirs-subdir-zip.test \
		rom-dir-in-extra-dirs.test \
		rom-duplicate-1st.test \
		rom-duplicate-2nd.test \
		rom-duplicate-ok.test \
		rom-duplicate-wrong.test \
		rom-from-child-renamed.test \
		rom-from-extra-broken-unwanted.test \
		rom-from-extra-broken.test \
		rom-from-extra-child.test \
		rom-from-extra-commit-error.test \
		rom-from-extra-dbcache-cleanup.test \
		rom-from-extra-detector.test \
		rom-from-extra-duplicate.test \
		rom-from-extra-ignore-db.test \
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
		rom-from-superfluous-many-fixdat.test \
		rom-from-superfluous-many.test \
		rom-from-superfluous-nofix.test \
		rom-from-superfluous-some.test \
		rom-from-superfluous.test \
		rom-from-unknown.test \
		rom-long-double.test \
		rom-long-end.test \
		rom-long-mid.test \
		rom-many.test \
		rom-need-partial-and-whole.test \
		rom-two-long.test \
		rom-unused.test \
		sample-miss.test \
		sample-ok.test \
		sample-parent-ok.test \
		sample-superfluous.test \
		skip-full.test \
		skip.test \
		skipbad.test \
		superfluous-broken.test \
		superfluous-cleanup.test \
		superfluous-cleanup-disk.test \
		superfluous-inconsistent.test \
		superfluous.test \
		swap-roms-2.test \
		swap-roms.test \
		swap.test \
		tochild.test \
		tochildfix.test \
		uncompressed-ckmamedb-add-game.test \
		uncompressed-ckmamedb-changed-new-mtime.test \
		uncompressed-ckmamedb-changed-same-mtime-nofix.test \
		uncompressed-ckmamedb-changed-same-mtime-zip.test \
		uncompressed-ckmamedb-changed-same-mtime.test \
		uncompressed-ckmamedb-extra-dir.test \
		uncompressed-ckmamedb-no-change.test \
		uncompressed-ckmamedb-remove-game-2.test \
		uncompressed-ckmamedb-remove-game-3.test \
		uncompressed-ckmamedb-remove-game.test \
		uncompressed-ckmamedb-sort-order-reverse.test \
		uncompressed-ckmamedb-sort-order.test \
		uncompressed-toplevel-extra.test \
		uncompressed-toplevel-file.test \
		uncompressed-toplevel-roms.test \
		wrong.test \
		wrong2.test \
		zero-good.test \
		zero-long-badcrc.test \
		zero-long.test \
		zero-miss.test \
		zero-superfluous.test \
		zero.test
XFAIL_TESTS=	\
		detector-interaction-with-headerless.test \
		detector-prefer-file-with-header.test \
		disk-conflict.test \
		disk-swapmerge.test \
		mkmamedb-duplicate-nodump.test \
		mkmamedb-merge-no-parent.test \
		mkmamedb-merge-three-generations.test \
		mkmamedb-merge-wrong-name.test \
		mkmamedb-parent-crcdiff-mame.test \
		nogood-diskgood.test \
		rom-from-extra-duplicate.test \
		uncompressed-toplevel-roms.test

ZIPS=		1-4-2.zip \
		1-4-crcerror.zip \
		1-4-dup.zip \
		1-4-end.zip \
		1-4-foo.zip \
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
		1-8-called-04.zip \
		1-8-long.zip \
		1-8-ok.zip \
		1-8-skip-ok.zip \
		1-a-ok.zip \
		1-c-ok.zip \
		12345678.zip \
		2-44-ok.zip \
		2-48-broken.zip \
		2-48-long.zip \
		2-48-ok.zip \
		2-4a-ok.zip \
		2-4c-ok.zip \
		2-4c-reverse.zip \
		2-84.zip \
		2-8c-ok.zip \
		2-cc-ok.zip \
		baddump.zip \
		deadbeef.zip \
		deadclonedbeef.zip \
		deadfish.zip \
		deadfish4.zip \
		deadpork.zip \
		garbage-1-8-ok.zip \
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
		ckmamedb-1-4-ok-1-8-ok.dump \
		ckmamedb-1-4-ok-with-mtime.dump \
		ckmamedb-1-4-ok.dump \
		ckmamedb-1-8-ok.dump \
		ckmamedb-2-4c-ok.dump \
		ckmamedb-2-4c-reverse.dump \
		ckmamedb-empty.dump \
		ckmamedb-wrongname-nohash.dump \
		detector.xml \
		mame.txt \
		mamedb-1-8-is-4.txt \
		mamedb-2.txt \
		mamedb-baddump.txt \
		mamedb-broken-sha1.dump \
		mamedb-broken-sha1.txt \
		mamedb-deadbeefish.txt \
		mamedb-dir-in-name.dump \
		mamedb-dir-in-name.txt \
		mamedb-disk-conflict.txt \
		mamedb-disk-many.txt \
		mamedb-disk.txt \
		mamedb-duplicate-game.dump \
		mamedb-duplicate-nodump.dump \
		mamedb-duplicate-nodump.txt \
		mamedb-duplicate-rom-name.dump \
		mamedb-duplicate-rom-name.txt \
		mamedb-lost-parent-ok.txt \
		mamedb-lost-parent.dump \
		mamedb-lost-parent.txt \
		mamedb-mame.dump \
		mamedb-mame.xml \
		mamedb-merge-no-parent.dump \
		mamedb-merge-no-parent.xml \
		mamedb-merge-parent.dump \
		mamedb-merge-parent.txt \
		mamedb-merge-wrong-name.dump \
		mamedb-merge-wrong-name.xml \
		mamedb-mess.dump \
		mamedb-mess.xml \
		mamedb-multi-dats.dump \
		mamedb-no-name.dump \
		mamedb-no-name.txt \
		mamedb-ok.dump \
		mamedb-one-game-two-roms.txt \
		mamedb-parent-crcdiff-mame.dump \
		mamedb-parent-crcdiff-mame.xml \
		mamedb-parent-crcdiff.dump \
		mamedb-parent-crcdiff.txt \
		mamedb-parent-no-common.dump \
		mamedb-parent-no-common.txt \
		mamedb-size-hex.dump \
		mamedb-size-hex.txt \
		mamedb-skipped.txt \
		mamedb-small.dump \
		mamedb-small.txt \
		mamedb-two-games.txt

check-am: ${DBS} runtest

DBS=\
	mame.db \
	mamedb-1-8-is-4.db \
	mamedb-baddump.db \
	mamedb-deadbeefish.db \
	mamedb-disk.db \
	mamedb-disk-conflict.db \
	mamedb-disk-many.db \
	mamedb-lost-parent-ok.db \
	mamedb-merge-parent.db \
	mamedb-one-game-two-roms.db \
	mamedb-parent-crcdiff.db \
	mamedb-parent-no-common.db \
	mamedb-small.db \
	mamedb-two-games.db

update-mamedb.dump: ${DBS} dbdump ../src/mkmamedb
	./dbdump mame.db > mamedb-ok.dump
	./dbdump mamedb-lost-parent-ok.db > mamedb-lost-parent.dump
	../src/mkmamedb -o mame.db.$$ ${srcdir}/mamedb.txt ${srcdir}/mamedb-lost-parent-ok.txt && ./dbdump mame.db.$$ > mamedb-duplicate-game.dump && rm mame.db.$$
	./dbdump mamedb-parent-crcdiff.db > mamedb-parent-crcdiff.dump
	./dbdump mamedb-parent-no-common.db > mamedb-parent-no-common.dump
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

install-pkglibLTLIBRARIES:
	@echo not installing shared objects needed only for testing
