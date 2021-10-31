#!/bin/bash

cd ../library/
mkdir out
cd out
# rm *
cmake ..
make
cd ../../accountService/
mkdir out
cd out
# rm *
# cmake ..
make
cp server ../../../qReaderExecutable/

