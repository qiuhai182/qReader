#!/bin/bash

#---------------------编译脚本-----------------------------
#-------------编译，运行，终止脚本中数组一致-----------------
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


#编译服务 $1为服务目录
function com_service(){
    if [ ! -d "../service/$1/out" ]; then
        mkdir ../service/$1/out
    else
        #全部删除
        rm -r ../service/$1/out/*
    fi
    if [ ! -d "../service/$1/out" ]; then
        #创建失败
        echo -e "\033[31m Hit: can't create directory qReader/$1/out \033[0m"
        return 
    else
        cd ../service/$1/out
        cmake .. && make 
    fi
    if [ ! -x "server" ]; then
        echo -e "\033[31m Hit: can't create file qReader/service/$1/out/server \033[0m"
        return 
    else
    #删除无用文件
        rm *pb* *ake* -r
        echo -e "\033[32m Hit: create file qReader/service/$1/out/server successful \033[0m"
    fi
    cd ../../../scripts
}

#创建lib_iReader.a 
link_lib

#参数个数判断
if [$# != 1 ] ; then
    echo -e "\033[31m USAGE: $0 Need more parameters \033[0m"
    echo -e "\033[31m e.g. : $0 -ac \033[0m"
    echo -e "\033[31m Parameters :-ec -bc -bs -cd -cm  -fi -ac -pc -all  \033[0m"
    exit 1;
fi
#将服务和端口对应
serverArr=("echoService" "bookCityService" "bookShelfService" "collectDataService"
             "bookCommentService" "fileUpDownService" "accountService" "pageCommentService")
#portArr=(8001 8002 8003 8004 8005 8006 8007)
#echo bookCity bookShelf collectData common fileUpDown account pageComment
#参数对应 -ec -bc -bs -cd -cm  -fi -ac -pc -all(全部编译)
case "$1" in
		"-ec" )
            com_service  ${serverArr[0]}
			;;
        "-bc" )
            com_service  ${serverArr[1]}
			;;
        "-bs" )
            com_service  ${serverArr[2]}
			;;
        "-cd" )
            com_service  ${serverArr[3]}
			;;
        "-cm" )
            com_service  ${serverArr[4]}
			;;
        "-fi" )
            com_service  ${serverArr[5]}
			;;
        "-ac" )
            com_service  ${serverArr[6]}
			;;
        "-pc" )
            com_service  ${serverArr[7]}
			;;
        "-all" )
            #全部编译
            for ((i=0;i<=7;i++))
            do
                com_service  ${serverArr[i]}
            done
			;;
		* )
			echo -e "\033[31m Hint: Error parameters \033[0m"
			exit 1
			;;
	esac

exit 0












