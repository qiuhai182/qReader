#!/bin/bash

#---------------------初始化脚本-----------------------------




#创建lib_iReader.a
function link_lib(){
    if [ ! -d "../library/out" ]; then
        mkdir ../library/out
    fi
    if [ ! -f "../library/out/lib_iReader.a" ]; then
        cd ../library/out  &&  cmake ..  && make 
        cd ../../scripts
    fi
    if [ ! -f "../../out/lib_iReader.a" ]; then
        echo -e "\033[32m Hit: create lib_iReader.a successful \033[0m"
    else
        echo -e "\033[31m Hit: create lib_ireader fialed \033[0m"
        exit 1
    fi 
}

#编译
function compile_init(){
    if [ ! -d "init/out" ]; then
        mkdir init/out
    else
        #全部删除
        rm -r init/out/*
    fi
    if [ ! -d "init/out" ]; then
        #创建失败
        echo -e "\033[31m Hit: can't create directory qReader/scripts/init/out \033[0m"
        exit 1 
    else
        cd init/out
        cmake .. && make 
    fi
    if [ ! -x "init" ]; then
        echo -e "\033[31m Hit: can't create file qReader/scripts/init/out/init \033[0m"
        exit 1
    else
    #删除无用文件
        rm *ake* -r
        echo -e "\033[32m Hit: create file qReader/scripts/init/out/init successful \033[0m"
    fi
    
}


link_lib

compile_init

./init

cd ../../../

echo -e "\033[32m Hit: init project successful \033[0m"