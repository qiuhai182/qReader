#!/bin/bash

#---------------------运行脚本-----------------------------
#-------------编译，运行，终止脚本中数组一致-----------------



function run(){
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    
    if [ ! "$pid" = "" ];then
        echo -e "\033[31m Hit:Port $2 is already occupied ,$1 Start-up was failed  \033[0m"
        return 
    else
        cd ../service/$1/out
        nohup ./server > $1.log  2>&1 &
        cd ../../../scripts
    fi
    #没有该行的sleep 0.1最后一个lsof检查不到导致运行成功却显示失败，该bug未解决
    sleep 0.1
    #检测运行
    res="$(lsof -i:$2)"
    list=($res)
    pid=${list[10]}
    if [  "$pid" = "" ];then
        echo -e "\033[31m Hit:$1 Failed to start  \033[0m"
        return 
    else
        echo -e "\033[32m Hit: $1 Start-up was successful \033[0m"
    fi
}

#参数个数判断
if [ ! $# = 1 ] ; then
    echo -e "\033[31m USAGE: $0 Need more parameters \033[0m"
    echo -e "\033[31m e.g. : $0 -ac \033[0m"
    echo -e "\033[31m Parameters :-ec -bc -bs -cd -cm  -fi -ac  -all  \033[0m"
    exit 1;
fi
#将服务和端口对应
serverArr=("echoService" "bookCityService" "bookShelfService" "collectDataService"
             "bookCommentService" "fileUpDownService" "accountService")
portArr=(8001 8002 8003 8004 8005 8006 8007)


#参数对应 -ec -bc -bs -cd -cm  -fi -ac  -all(全部运行)
case "$1" in
		"-ec" )
            run  ${serverArr[0]}  ${portArr[0]}
			;;
        "-bc" )
            run  ${serverArr[1]}  ${portArr[1]}
			;;
        "-bs" )
            run  ${serverArr[2]}  ${portArr[2]}
			;;
        "-cd" )
            run  ${serverArr[3]}  ${portArr[3]}
			;;
        "-cm" )
            run  ${serverArr[4]}  ${portArr[4]}
			;;
        "-fi" )
            run  ${serverArr[5]}  ${portArr[5]}
			;;
        "-ac" )
            run  ${serverArr[6]}  ${portArr[6]}
			;;
        "-all" )
            #全部终止
            for ((i=0;i<=6;i++))
            do
                run  ${serverArr[i]}  ${portArr[i]}
            done
			;;
		* )
			echo -e "\033[31m Hint: Error parameters \033[0m"
			exit 1
			;;
	esac

exit 0