noinst_PROGRAMS=	dbdump
AM_CFLAGS=		-I${srcdir}/../src \
			 -DDDB_INCLUDE=\"db-@DBTYPE@.h\"
dbdump_LDADD=		../src/libckmame.a @DBLIBS@

EXTRA_DIST=runtest ${TESTCASES} ${ZIPS} ${CHDS} ${DBFILES}

CLEANFILES=${MAME_DB}

MAME_DB=mame.db
ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=	ZIPCMP="${ZIPCMP}" MAME_DB="${MAME_DB}"

TESTCASES=	baddump.test \
		disk-from-extra-remove.test \
		disk-from-superfluous.test \
		disk-mis.test \
		disk-noext.test \
		disk-nogood.test \
		disk-nogoodmis.test \
		disk-ok.test \
		disk-superfluous.test \
		disk-wrong.test \
		double.test \
		dumpgame-unknown.test \
		garbage.test \
		inchild.test \
		inparent.test \
		hasherr.test \
		hasherr2.test \
		hasherr-c.test \
		hasherr-p.test \
		hasherr-pc.test \
		mid.test \
		misnamed.test \
		misnamed-dryrun.test \
		mkmamedb-lost-parent.test \
		mkmamedb-ok.test \
		multi.test \
		needed.test \
		needed2.test \
		needed3.test \
		needed-error.test \
		nogood.test \
		nogood-miss.test \
		nonexist.test \
		norom.test \
		ok.test \
		punknown.test \
		punused.test \
		rom-broken.test \
		rom-from-extra-keep.test \
		rom-from-extra-partial.test \
		rom-from-extra-remove.test \
		rom-from-needed.test \
		rom-from-needed2.test \
		rom-from-romset.test \
		rom-from-romset2.test \
		rom-from-superfluous.test \
		rom-from-superfluous-dryrun.test \
		skipbad.test \
		superfluous.test \
		swap.test \
		tochildfix.test \
		wrong.test

ZIPS=		1-4-dup.zip \
		1-4-garbage.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-8-ok.zip \
		1-c-ok.zip \
		2-48-ok.zip \
		2-4c-ok.zip \
		2-84.zip \
		deadbeef.zip \
		deadclonedbeef.zip \
		deadfish.zip \
		deadfish4.zip \
		deadpork.zip \
		garbage.zip

CHDS=		108-5.chd \
		12-1.chd


DBFILES=	mamedb.txt \
		mamedb-lost-parent.txt \
		mamedb-lost-parent-ok.txt \
		mkmamedb-lost-parent.dump \
		mkmamedb-ok.dump

check: ${MAME_DB}
	@failed=0; all=0; \
	srcdir=$(srcdir); export srcdir; \
	for tst in ${TESTCASES}; do \
	  t=`basename $$tst .test`; \
	  if ${TESTS_ENVIRONMENT} $(srcdir)/runtest $$t; then \
	    all=`expr $$all + 1`; \
	    echo "PASS: $$tst"; \
	  elif test $$? -ne 77; then \
	    all=`expr $$all + 1`; \
	    failed=`expr $$failed + 1`; \
	    echo "FAIL: $$tst"; \
	  fi; \
	done; \
	if test "$$failed" -eq 0; then \
	  banner="All $$all tests passed"; \
	else \
	  banner="$$failed of $$all tests failed"; \
	fi; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"; \
	test "$$failed" -eq 0

${MAME_DB}: ../src/mkmamedb ${srcdir}/mamedb.txt
	../src/mkmamedb ${srcdir}/mamedb.txt

update-mkmamedb.dump: ${MAME_DB}
	./dbdump ${MAME_DB} | sort > mkmamedb-ok.dump
