# #!/bin/bash
# set -x

# rm -f abttest1 abttest2
# rm -f ABTree.o abttest1.o abttest2.o
# rm -f mydb.table1.dat

# g++ -g -c -DABTREE -DDEBUG -o abttest1.o test1.cpp
# g++ -g -c -DABTREE -DDEBUG -o abttest2.o test2.cpp
# g++ -g -c -DABTREE -DDEBUG -o ABTree.o ABTree.cpp

# g++ -g -o abttest1 abttest1.o ABTree.o
# g++ -g -o abttest2 abttest2.o ABTree.o


#!/bin/bash
set -x

start=1
end=2

# remove executables
for i in $(seq $start $end); do rm -f abttest${i}; done
# for i in $(seq $start $end); do rm -f test${i}; done

# remove object files
rm -f ABTree.o 
for i in $(seq $start $end); do rm -f abttest${i}.o; done
# for i in $(seq $start $end); do rm -f test${i}.o; done

# remove data files
rm -f *.dat

# build object files
for i in $(seq $start $end)
do
  g++ -g -c -DABTREE -DVALUE_SIZE=16 -DDEBUG -o abttest${i}.o  abttest${i}.cpp
  # g++ -g -c -DABTREE -DVALUE_SIZE=16 -DDEBUG -o test${i}.o  test${i}.cpp
done
g++ -g -c -DABTREE -DVALUE_SIZE=16 -DDEBUG -o ABTree.o ABTree.cpp

# build tests
for i in $(seq $start $end)
do
  g++ -g -o abttest${i} abttest${i}.o ABTree.o
  # g++ -g -o test${i} test${i}.o ABTree.o
done
