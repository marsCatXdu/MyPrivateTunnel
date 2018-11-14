/**
 * Client
*/

#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

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

enum class WorkerState {
    Starting,
    Started,
    Stopping,
    Stopped,
    Killing
};

class Worker {
public:
    Worker() {}
    void Worker::startWorking() {
        std::unique_lock<std::mutex> l(x_work);
        if(m_work) {
            WorkerState ex = WorkerState::Stopped;
            m_state.compare_exchange_strong(ex, WorkerState::Starting);
            m_state_notifier.notify_all();
        } else {
            m_state = WorkerState::Starting;
            m_state_notifier.notify_all();
            m_work.reset(new thread([&](){
                while(m_state!=WorkerState::Killing) {
                    WorkerState ex = WorkerState::Starting;
                    {
                        unique_lock<mutex> l(x_work);
                        m_state = WorkerState::Started;
                    }
                    m_state_notifier.notify_all();

                    try
                    {
                        startedWorking();
                        workLoop();
                        doneWorking();
                    }
                    catch (std::exception const& _e)
                    {
                        std::cout << "Exception thrown in Worker thread: " << _e.what();
                    }

                    {
                        // the condition variable-related lock
                        unique_lock<mutex> l(x_work);
                        ex = m_state.exchange(WorkerState::Stopped);
    //					cnote << "State: Stopped: Thread was" << (unsigned)ex;
                        if (ex == WorkerState::Killing || ex == WorkerState::Starting)
                            m_state.exchange(ex);
                    }
                    m_state_notifier.notify_all();
    //				cnote << "Waiting until not Stopped...";

                    {
                        unique_lock<mutex> l(x_work);
                        while (m_state == WorkerState::Stopped)
                            m_state_notifier.wait(l);
                    }

                }
            }));
        }
    }

    void Worker::workLoop()
    {
        while (m_state == WorkerState::Started)
        {
            if (m_idleWaitMs)
                this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
            doWork();
        }
    }

	virtual void startedWorking() {}
    virtual void doWork() {}
	virtual void workLoop();
	virtual void doneWorking() {}

private:
	std::string m_name;

    unsigned m_idleWaitMs = 0;
    
    mutable std::mutex x_work;
    std::unique_ptr<std::thread> m_work;    // working thread
    mutable std::condition_variable m_state_notifier;
    std::atomic<WorkerState> m_state = {WorkerState::Starting};
    bool workingState = false;
};

class MptListener: public Worker {
public:
    MptListener() {}
private:

};

class commonUtils {
public:
    void heartbeat() {      // heartbeat packet sender.

    }

    string bufferToStr(vector<char> in) {
        string str;
        for(int i=0; i<in.size(); i++) {
            if(in[i]!=0) str+=in[i];
        }
        return str;
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
    commonUtils cutils;                     // init common utils.
    string interchargeServer;
    std::cout << "Make sure your intercharge server is running forwarding program and destination port is open!\nInput your intercharge server: example: 1.2.3.4:1234 \n>>>";
    std::cin >> interchargeServer;          // it should be a string, format example: 1.2.3.4:1234
    remoteEndpoint iServerEp = remoteEndpoint(interchargeServer);
    send_ep = ip::udp::endpoint(ip::address::from_string(iServerEp.rmIp), iServerEp.rmPort);
    iServerEp.~remoteEndpoint();            // useless now, destory

    sock.open(ip::udp::v4());               // start local socket

    cout<<"Sending transportion request to intercharge server\n";
    char buf[2];
    sock.send_to(buffer(buf), send_ep);     // send_ep is the target we send packet to
    cout<<"Request sent.\n";

    vector<char> v(100, 0);

    cout<<"Waiting for responce........\n";
    sock.receive_from(buffer(v), send_ep);
    cout<<"Responce received. Ready for transportion\n";
//    remoteEndpoint rep(v);                  // get remote endpoint. ancient things, now a trash. reserve for preventing bugs
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