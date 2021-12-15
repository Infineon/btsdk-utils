#! /bin/bash

echo "###"

echo "( in: $(pwd) )"
echo "+ make -B -j1 -f ../../src/makefile all"
        make -B -j1 -f ../../src/makefile all

echo "###"

##  make -B -j1 -f ../../src/makefile DEBUG all

echo "###"
