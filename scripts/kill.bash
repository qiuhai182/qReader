#!/bin/bash

#---------------------终止脚本-----------------------------
#-------------编译，运行，终止脚本中数组一致-----------------


#杀死进程 $1: service  $2:port
function kill_pro(){
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    if [ ! "$pid" = "" ]; then 
        kill -9 $pid
        echo -e "\033[32m Hit: Kill process $1 successful \033[0m"
    else 
        echo -e "\033[31m Hit:The process $1 isn't running  \033[0m"
    fi 
}


#参数个数判断
if [ $# != 1 ] ; then
    echo -e "\033[31m USAGE: $0 Need more parameters \033[0m"
    echo -e "\033[31m e.g. : $0 -ac \033[0m"
    echo -e "\033[31m Parameters :-ec -bc -bs -cd -cm  -fi -ac -pc -all  \033[0m"
    exit 1;
fi
#将服务和端口对应
serverArr=("echoService" "bookCityService" "bookShelfService" "collectDataService"
             "commonService" "fileUpDownService" "accountService" "pageCommentService")
portArr=(8001 8002 8003 8004 8005 8006 8007 8008)
#echo bookCity bookShelf collectData common fileUpDown account pageComment
#参数对应 -ec -bc -bs -cd -cm  -fi -ac -pc -all(全部编译)
case "$1" in
		"-ec" )
            kill_pro  ${serverArr[0]}  ${portArr[0]}
			;;
        "-bc" )
            kill_pro  ${serverArr[1]}  ${portArr[1]}
			;;
        "-bs" )
            kill_pro  ${serverArr[2]}  ${portArr[2]}
			;;
        "-cd" )
            kill_pro  ${serverArr[3]}  ${portArr[3]}
			;;
        "-cm" )
            kill_pro  ${serverArr[4]}  ${portArr[4]}
			;;
        "-fi" )
            kill_pro  ${serverArr[5]}  ${portArr[5]}
			;;
        "-ac" )
            kill_pro  ${serverArr[6]}  ${portArr[6]}
			;;
        "-pc" )
            kill_pro  ${serverArr[7]}  ${portArr[7]}
			;;
        "-all" )
            #全部终止
            for ((i=0;i<=7;i++))
            do
                kill_pro  ${serverArr[i]}  ${portArr[i]}
            done
			;;
		* )
			echo -e "\033[31m Hint: Error parameters \033[0m"
			exit 1
			;;
	esac

exit 0












