EXTRA_DIST=runtest mamedb.txt ${TESTCASES} ${ZIPS}

CLEANFILES=mamedb

ZIPCMP=@ZIPCMP@

TESTS_ENVIRONMENT=	ZIPCMP="${ZIPCMP}"

TESTCASES=	baddump.test \
		double.test \
		garbage.test \
		inchild.test \
		inparent.test \
		mid.test \
		misnamed.test \
		multi.test \
		nogood.test \
		nonexist.test \
		ok.test \
		punknown.test \
		punused.test \
		tochild.test \
		tochildfix.test \
		unused.test \
		wrong.test

ZIPS=		1-4-dup.zip \
		1-4-mid.zip \
		1-4-misnamed.zip \
		1-4-ok.zip \
		1-8-ok.zip \
		1-c-ok.zip \
		2-48-ok.zip \
		2-4c-ok.zip

check: mamedb
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

mamedb: ../mkmamedb ${srcdir}/mamedb.txt
	../mkmamedb -o mamedb ${srcdir}/mamedb.txt
