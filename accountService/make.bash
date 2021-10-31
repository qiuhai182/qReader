#!/bin/bash

cd ../library/
mkdir out
cd out
cmake ..
make
cd ../../accountService/
mkdir out
cd out
rm *
cmake ..
make
cp server ../../../qReaderExecutable/server
cd ..
ln -s ../../qReaderExecutable/server server

