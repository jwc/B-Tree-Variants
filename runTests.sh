#!/bin/bash

TREES="ubp asb ab"

TESTS="1 2"

make $TREES

echo 

for tree in $TREES
do
    for test in $TESTS
    do
        if [ -f *.db ]
        then 
            rm *.db
        fi

        if [ -f *.dat ]
        then 
            rm *.dat
        fi

        echo "$tree$test"
        ./$tree$test.out
    done 

    echo
done
