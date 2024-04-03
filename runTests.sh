#!/bin/bash

TIMEFORMAT="     Exec  Time: %R s"

TREES="ubp"

TESTS="0 1 2 3 4 5 6"

make $TREES

echo 

for tree in $TREES
do
    for test in $TESTS
    do
        if [ -f $tree$test.txt ]
        then
            rm $tree$test.txt
        fi

        echo "$tree$test:"

        time ./$tree$test.out

        if [ -f $tree$test.txt ]
        then
            stat -c "     Tree  size: %s B" $tree$test.txt
        fi

        echo
    done
done
