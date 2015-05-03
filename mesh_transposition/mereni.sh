n=12; ./a.out $n $n $n; echo "test fo $n" > results_$n; for i in {1..10}; do sh test.sh  2> /dev/null | grep time >> results_$n; done
