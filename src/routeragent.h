#ifndef __ROUTERAGENT_H__
#define __ROUTERAGENT_H__
#include<map>
#include<vector>
#include"agentdata.h"
#include"isockapi.h"
#include"isockmsg.h"


class SockFrame;
class LoginCenter;
class AgentProto;
class CliCenter;
class AgentCli;
class AgentListener;

class AgentCenter { 
    typedef std::vector<AgentConf*> typeAgentConf; 
    
    typedef std::map<int, LinkAddr*> typeMapLink;
    typedef typeMapLink::iterator typeLinkItr;
    typedef typeMapLink::const_iterator typeLinkItrConst;

    typedef int (AgentCenter::*PFunc)(AgentBase*, int, NodeMsg*);

    struct Worker {
        unsigned m_cmd;
        PFunc m_fn;
    };
    
public:
    AgentCenter(SockFrame* frame, LoginCenter* center,
        AgentProto* proto, CliCenter* cli); 
    ~AgentCenter();

    int init();
    void finish(); 

    void start();

    int addListener(const char ip[], int port, 
        unsigned uid, int link_type);
    
    int addLink(const char ip[], int port, 
        unsigned uid, int link_type);

    int creatAgentSvr(int parentId, int newFd,
        AccptOption& opt);

    int creatAgentCli(int parentId, NodeMsg* msg);

    AgentConf* allocConf();
    void freeConf(AgentConf* conf);

    AgentBase* allocAgent(const char ip[],
        int port, int link_type);

    void freeAgent(AgentBase* data);

    int parseData(int fd, const char* buf, int size);

    int process(int hd, NodeMsg* msg);
    int procSouth(int hd, NodeMsg* msg);
    int procNorth(int hd, NodeMsg* msg);

    void onClose(int hd);
    void closeAgent(AgentBase* data);

    int onConnOK(int hd);
    void onConnFail(AgentBase* data);
 
    int chkTunnelReq(RouterCliData* cli, 
        const MsgTunnel* body);
    
    int chkTunnelAck(AgentBase* data, MsgTunnelAck* body);
    
    int procTunnelAck(AgentBase* data, int hd, NodeMsg* msg);

    int procPeerUsrExit(AgentBase* data, int, NodeMsg* msg);

    int sendTunnelMsg(AgentBase* data, const LinkAddr* addr);
    int sendCloseTunnelMsg(AgentBase* data);
    int sendTunnelAckMsg(AgentBase* data);

    int procTunnelClose(AgentBase* data, int, NodeMsg* msg);
    
    int procUnreach(AgentBase* data, int, NodeMsg* msg);
    
    int procSend(AgentBase* data, int, NodeMsg* msg);

    int procForward(AgentBase* data, int, NodeMsg* msg);

private:
    void freeLink(LinkAddr* addr);
    bool existLink(int link_type);
    const LinkAddr* findLink(int link_type);
    
    void _invalidPeer(AgentBase* data);
    
    int _forward(AgentBase* data, NodeMsg* msg);

    int _sendMsg(AgentBase* data, NodeMsg* msg);

private:
    static const Worker m_north_works[ENUM_MSG_CODE_END];
    static const Worker m_south_works[ENUM_MSG_CODE_END];
    SockFrame* m_frame;
    LoginCenter* m_center;
    AgentProto* m_proto;
    CliCenter* m_cli;
    AgentCli* m_agent_cli;
    AgentListener* m_agent_svr;
    typeMapLink m_links;
    typeAgentConf m_confs;
};

class AgentListener : public ISockSvr { 
public:
    AgentListener(AgentCenter* agent);

    virtual int onNewSock(int parentId,
        int newId, AccptOption& opt);
    
    virtual void onListenerClose(int hd);

    virtual void onClose(int hd);
    
    virtual int process(int hd, NodeMsg* msg);

    virtual int parseData(int fd, const char* buf, int size);

private:
    AgentCenter* m_agent_db;
};

class AgentCli : public ISockCli {   
public:
    AgentCli(AgentCenter* agent); 
    
    virtual int onConnOK(int hd, ConnOption& opt);

    virtual void onConnFail(long extra, int errcode);

    virtual void onClose(int hd);
    
    virtual int process(int hd, NodeMsg* msg);

    virtual int parseData(int fd, const char* buf, int size); 

    int startAgent(unsigned uid, const MsgTunnel* body);

private:
    AgentCenter* m_agent_db;
};

#endif

