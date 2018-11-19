/**
 * 服务端，在自己 ip 的指定端口上监听
 * 客户端发来报文暴露位置，返回信息
 * 
 * This program should be running on traverse server
 * and forward nat-traverse info(i.e. two endpoint)
 * automaticly
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

io_service io;
ip::udp::socket sock(io, ip::udp::endpoint(ip::udp::v4(), 6699));
ip::udp::endpoint ep;       // first incomming client
ip::udp::endpoint ep2;      // second incomming client


enum class WorkerState {
    Starting,
    Started,
    Stopping,
    Stopped,
    Killing
};

class Worker {
public:
    Worker(std::string const& _name = "anon") {
        std::cout<<"Worker constructor called\n";
    }
    void startWorking() {                   // part of utility for worker
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
    // 'std::lock_guard<std::mutex>' named Guard in aleth
	bool isWorking() const { std::lock_guard<std::mutex> l(x_work); return m_state == WorkerState::Started; }

    void stopWorking() {}

    void terminate() {}

    void workLoop()                 // part of utility for worker
    {
        while (m_state == WorkerState::Started)
        {
            //if (m_idleWaitMs)
            //    this_thread::sleep_for(chrono::milliseconds(m_idleWaitMs));
            this_thread::sleep_for(chrono::milliseconds(100));
            doWork();
        }
    }

    // void startWorking();                // 基类中已经实现的函数这么声明，这个函数不需要子类再去实现，子类不用再声明一遍直接调用即可
	virtual void startedWorking() {}    
    virtual void doWork() {}            // 声明并在基类中调用但基类没有实现的虚函数，声明使用花括号
	// virtual void workLoop();     这样应该是先定义在.h中然后再实现的，需要分开
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
//---------------------------------Worker--------------------------------



//---------------------------------MptListener--------------------------------
class MptListener: public Worker {  // listener thread
public:
    MptListener(ip::udp::endpoint _ep, int _id): epListening(_ep), epId(_id) {         // 会先使用基类的默认构造器，然后再走子类构造器
        std::cout<<"MptListener constructor called\n";
    }

    void start() {
        startWorking();
    }
private:
    virtual void doWork();
    virtual void doneWorking();

    ip::udp::endpoint epListening;
    int epId;
};

void MptListener::doWork() {
        char m_buf[1024]={0};
        std::cout<<"Hearing\n";
        sock.receive_from(buffer(m_buf) ,epListening);
        if(epId==1) {
            std::cout<<"Heard from ep_1, Forwarding to ep_2.\n";
            sock.send_to(buffer(m_buf), ep2);
            m_buf[1024]={0};
        } else {
            std::cout<<"Heard from ep_2, Forwarding to ep_1.\n";
            sock.send_to(buffer(m_buf), ep);
            m_buf[1024]={0};
        }
}

void MptListener::doneWorking() {

}
//---------------------------------MptListener--------------------------------


// @brief Storage incomming endpoint info
class targetEndpoint {
public:
    targetEndpoint(string i, string p):tgtIp(i), tgtProt(p){}
    string tgtIp;
    string tgtProt;
};

// @brief Forward one endpoint information to the other
class endpointForwarder {

};

// @brief traverse aux server implementation
class traverseServer {
public:
    traverseServer() {}
};

int main() {

    char buf[2];
    char buf2[2];
/**
 * @brief: process incomming requests.(say hello to each other)
 * @TODO: Make it like a handshake
*/
    sock.receive_from(buffer(buf) ,ep, 0);      // ep1 info
    targetEndpoint tea(ep.address().to_string(), boost::lexical_cast<string>(ep.port()));

    sock.receive_from(buffer(buf2) ,ep2, 0);    // ep2 info
    targetEndpoint teb(ep2.address().to_string(), boost::lexical_cast<string>(ep2.port()));

    MptListener lisa(ep, 1);       // lisa means "Listener A"
    MptListener lisb(ep2, 2);

    sock.send_to(buffer(tea.tgtIp+":"+tea.tgtProt), ep2);
    sock.send_to(buffer(teb.tgtIp+":"+teb.tgtProt), ep);
// hello end.

/**
 * @brief: forwarding working loop
 * @TODO: make it useable
*/
    lisa.start();
    lisb.start();
    for(; ; ) {
        char m_buf[1024]={0}, m_buf2[1024]={0};
        if(sock.receive_from(buffer(m_buf) ,ep)) {
            std::cout<<"Hear from ep, Sending to ep2\n";
            sock.send_to(buffer(m_buf), ep2);
            m_buf[1024]={0};
            m_buf2[1024]={0};
        }
        if(sock.receive_from(buffer(m_buf2) ,ep2)) {
            std::cout<<"Hear from ep2, Sending to ep\n";
            sock.send_to(buffer(m_buf2), ep);
            m_buf[1024]={0};
            m_buf2[1024]={0};
        }
    }

    return 0;
}