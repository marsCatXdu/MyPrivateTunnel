#!/bin/bash

if [ "$1" -eq 1 ];then
    echo "SSH to IodineJP"
    ssh root@#your server
elif [ "$1" -eq 2 ];then
    echo "Send UdpServer.cpp to IodineJP"
    # scp ./UdpServer.cpp root@Your server ip:/root/LinkingCPP/
elif [ "$1" -eq 3 ];then
    echo "Complie UdpClient.cpp"
    g++ -std=c++11 ./client/UdpClient.cpp -o ./client.out -I /usr/include/boost/ -lpthread -lboost_thread -lboost_system 
else
    echo "需要输入参数"
    echo "1 -> SSH"
    echo "2 -> Send to server"
    echo "3 -> Complie UdpClient.cpp"
fi
