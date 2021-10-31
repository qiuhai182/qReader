#!/bin/bash

cd ../
git pull
git add .
git commit -m "上传最新可执行文件、代码"
git push

cd ../qReaderExecutable
git pull
git add .
git commit -m "上传最新可执行文件"
git push

