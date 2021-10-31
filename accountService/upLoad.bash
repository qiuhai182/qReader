#!/bin/bash

cd ../../qReaderExecutable
git add .
git commit -m "上传最新可执行文件"
git push

cd ../qReader/
git add .
git commit -m "上传最新可执行文件链接"
git push

