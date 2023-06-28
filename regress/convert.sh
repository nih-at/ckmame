set -e

cd old

for test in *.test
do
  perl ../convert-test.pl $test > ../$test
done

