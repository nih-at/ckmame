EXTRA_DIST=runtest mamedb.txt ${TESTCASES} ${ZIPS} ${CHDS}

CLEANFILES=${MAME_DB}

MAME_DB=mame.db
ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=	ZIPCMP="${ZIPCMP}" MAME_DB="${MAME_DB}"

TESTCASES=	baddump.test \
		disk-mis.test \
		disk-noext.test \
		disk-nogood.test \
		disk-nogoodmis.test \
		disk-ok.test \
		disk-wrong.test \
		double.test \
		from-needed.test \
		garbage.test \
		inchild.test \
		inparent.test \
		mid.test \
		misnamed.test \
		multi.test \
		needed.test \
		nogood.test \
		nogood-miss.test \
		nonexist.test \
		norom.test \
		ok.test \
		punknown.test \
		punused.test \
		superfluous.test \
		tochild.test \
		tochildfix.test \
		unused.test \
		wrong.test \
		wrong2.test

ZIPS=		1-4-dup.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-8-ok.zip \
		1-c-ok.zip \
		2-48-ok.zip \
		2-4c-ok.zip
CHDS=		108-5.chd \
		12-1.chd

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
