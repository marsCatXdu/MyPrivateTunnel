#!/bin/bash

if [ "$1" -eq 1 ];then
    echo "Complie UdpClient.cpp"
    g++ -std=c++11 -o ./ForServer.out ./interchargeServer/UdpServer.cpp -I /usr/include/boost/ -lpthread -lboost_thread -lboost_system -lboost_date_time -lboost_regex
else
    echo "需要输入参数"
    echo "1 -> Complie UdpClient.cpp"
fi
