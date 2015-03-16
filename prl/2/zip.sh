PRG=pms
LOGIN=xsruba03

echo "vlna u latex!"
cp doc/doc.pdf $LOGIN.pdf
cp scripts-tests/performance_test.sh  performance_test.sh
tar cvf $LOGIN.tar $PRG.{c,h} test.sh  $LOGIN.pdf performance_test.sh
