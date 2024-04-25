#ifndef __SERVICE_H__
#define __SERVICE_H__


struct GlobalConf;
class SockFrame;
class LoginCenter;
class AgentProto;
class SvrCenter;
class CliCenter;
class AgentCenter;

class Service {
public:
    Service();
    ~Service();
    
    int init();
    int parseInit(const char* conf_file);
    
    void finish(); 

    int addSvr(const char ip[], int port, unsigned uid);
    int addCli(const char ip[], int port, unsigned uid);

    int addAgentListener(const char ip[], int port, 
        unsigned uid, int link_type);
    
    int addLink(const char ip[], int port, 
        unsigned uid, int link_type);

    void start();
    void stop();
    void wait(); 

private:
    int prepare(const GlobalConf* global);

private:
    SockFrame* m_frame;
    LoginCenter* m_center;
    AgentProto* m_proto;
    SvrCenter* m_svr;
    CliCenter* m_cli;
    AgentCenter* m_agent;
    unsigned m_rd_timeout;
    unsigned m_wr_timeout;
    char m_log_dir[256];
};

#endif

