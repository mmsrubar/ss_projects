for in_num in {2,4,8,16,32,64,128};
do
  echo "the length of an input sequnce: $in_num"

  for i in {1..100}
  do 
    echo -n "test$i: "
    sh test.sh $in_num |grep time
  done
done
