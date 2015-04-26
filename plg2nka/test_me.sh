PRG="plg-2-nka"

make
for in_gram in tests/*.in; do
  infile=`basename $in_gram`
  name=${infile%.*}

  ./$PRG -2 $in_gram > tests/$name.nka.out

  diff tests/$name.nka.out tests/$name.nka.ok
  if [ "$?" -eq "0" ]; then
    echo "> Test for $in_gram OK"
  else
    echo "Error: $in_gram failed"
    exit 1
  fi
done

rm -rf tests/*.nka.out
