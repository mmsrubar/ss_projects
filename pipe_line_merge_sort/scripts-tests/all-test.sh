echo "testing input values from 1 to 256"

for i in {1,2,4,8,16,32,64}; do 
  echo -n "Test $i: "
  sh test.sh $i | sh check.sh
done
