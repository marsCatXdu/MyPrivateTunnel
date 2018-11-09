/**
 * Client
*/

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp> 

using namespace std;
using namespace boost::asio;

io_service io;                      // global IO we're using in this program
ip::udp::endpoint send_ep;          // global endpoint we're using in this program for sending message
//ip::udp::endpoint send_ep(ip::address::from_string("47.74.42.53"), 6699);
ip::udp::socket sock(io);           // global socket for communications
ip::udp::endpoint recv_ep;          // the endpoint we are listening on
ip::udp::endpoint send_ep2;

void listenThread() {       // refresh after heard something
    vector<char> v;
    cout<<"Listening thread created.\n";
    sock.receive_from(buffer(v), recv_ep);  // listening
    std::cout<<"Remote: "<<v[0];
    listenThread();
}

class commonUtils {
public:
    void heartbeat() {      // heartbeat packet sender.

    }
};

class remoteEndpoint {
public:
    remoteEndpoint(vector<char> in) {
        string rep=bufferToStr(in);
        int idx=rep.find(":");
        this->rmIp=rep.substr(0, idx);
        string tmp=rep.substr(idx+1, rep.length()-1);
        this->rmPort=boost::lexical_cast<int>(tmp);
    }

    remoteEndpoint(string rep) {
        int idx=rep.find(":");
        this->rmIp=rep.substr(0, idx);
        string tmp=rep.substr(idx+1, rep.length()-1);
        this->rmPort=boost::lexical_cast<int>(tmp);
    }

    ~remoteEndpoint() {}

    int traverser() {

        return 0;
    }

    string bufferToStr(vector<char> in) {
        string str;
        for(int i=0; i<in.size(); i++) {
            if(in[i]!=0) str+=in[i];
        }
        return str;
    }

    string rmEndpoint;
    string rmIp;
    int rmPort;
};

int main() {
    string interchargeServer;
    std::cout << "Input your intercharge server:";
    std::cin >> interchargeServer;          // it should be a string, format example: 1.2.3.4:1234
    remoteEndpoint iServerEp = remoteEndpoint(interchargeServer);
    send_ep = ip::udp::endpoint(ip::address::from_string(iServerEp.rmIp), iServerEp.rmPort);
    iServerEp.~remoteEndpoint();            // useless now, destory

    sock.open(ip::udp::v4());               // start local socket

    cout<<"Sending communication request to intercharge server\n";
    char buf[2];
    sock.send_to(buffer(buf), send_ep);     // send_ep is the target we send packet to

    vector<char> v(100, 0);

    cout<<"Waiting for responce........\n";
    sock.receive_from(buffer(v), send_ep);
    cout<<"Responce received. Ready for transportion\n";
    remoteEndpoint rep(v);                  // get remote endpoint. ancient things, now a trash. reserve for preventing bugs
    //cout<<"Done.\nRemote endpoint is "<<&v[0]<<endl;    // Get remote endpoint

    for(; ; ) {
        thread listenTh(listenThread);
        listenTh.detach();
        string tmp;
        cout << "input your text to send: ";
        cin >> tmp;
        if(tmp=="EXIT") break;
        sock.send_to(buffer(tmp), send_ep);     // I think it wont't pass compile.... no test today, i need to do other work now
    }
/*
    for(int i=0; i<1000; i++) {
        cout<<"Sending traverse packet "<<i<<endl;
        send_ep2=ip::udp::endpoint(ip::address::from_string(rep.rmIp), rep.rmPort);
        sock.send_to(buffer(buf), send_ep2);
        vector<char> v2(100, 0);
        thread listenTh(listenThread);
        listenTh.detach();
        //listenThread();
        usleep(10000);
    }
    usleep(5000000);
*/
/*  ancient codes...

    vector<char> v2(100, 0);
    cout<<"Listening.\n";
    sock.receive_from(buffer(v2), recv_ep);
    cout<<"RECEIVED!.\n";
    cout<<v2[0]<<endl<<"from "<<recv_ep.address()<<":"<<recv_ep.port();
*/    
    return 0;
}