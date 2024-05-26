#ifndef __CLICENTER_H__
#define __CLICENTER_H__
#include<vector>
#include<map>
#include"isockapi.h"
#include"agentdata.h"


class SockFrame;
class LoginCenter;
class AgentProto;
class AgentCenter;

class CliCenter : public ISockCli {
    typedef std::vector<RouterCliConf*> typeConfs;
    
    typedef std::map<unsigned, RouterCliData*> typeMapUsr;
    typedef typeMapUsr::iterator typeMapUsrItr;
    typedef typeMapUsr::const_iterator typeMapUsrItrConst;

    typedef int (CliCenter::*PFunc)(RouterCliData*, int, NodeMsg*);

    struct Worker {
        unsigned m_cmd;
        PFunc m_fn;
    };
    
public:
    CliCenter(SockFrame* frame, LoginCenter* center,
        AgentProto* proto);
    ~CliCenter();
    
    int init();
    void finish(); 

    void start();

    bool validUsr(unsigned uid); 
    bool hasUsr(unsigned uid);
    
    void setAgent(AgentCenter* agent);
    
    int addCli(const char ip[], int port, unsigned uid);
    
    /* get sid from uid and fd */
    unsigned regSid(const unsigned& uid, int fd);

    /* release a sid in uid */
    void unregSid(const unsigned& uid, const unsigned& sid);

    /* sid to uid msgs to deal */
    int collect(unsigned uid, NodeMsg* msg);

    virtual int onConnOK(int hd, ConnOption& opt);

    virtual void onConnFail(long extra, int errcode);

    virtual void onClose(int hd);
    
    virtual int process(int hd, NodeMsg* msg);

    virtual int parseData(int fd, const char* buf, 
        int size, const SockAddr* addr); 

    RouterCliData* findUsr(const unsigned& uid);
    void addUsr(const unsigned& uid, RouterCliData* data);
    void delUsr(const unsigned& uid);

    void rspSidUnreach(int reason,
        RouterCliData* data, NodeMsg* origin);
    
    
private:
    unsigned nextSid(); 

    RouterCliConf* allocConf();
    void resetConf(RouterCliConf* conf);
    void freeConf(RouterCliConf* conf);

    RouterCliData* allocData(RouterCliConf* conf);
    void freeData(RouterCliData* data);
    void resetData(RouterCliData* data);
    void cleanCli(RouterCliData* data);

    void closeChildren(RouterCliData* data);

    void reConn(RouterCliData* data);
    
    int procNorthMsg(int hd, NodeMsg* msg);
    int procSouthMsg(int hd, NodeMsg* msg);

    void closeCli(RouterCliData* data);

    int procCipher(RouterCliData* data, int, NodeMsg* msg);

    int procPlain(RouterCliData* data, int, NodeMsg* msg);

    int procTunnelReq(RouterCliData* data, int, NodeMsg* msg);

    int procLogout(RouterCliData* data, int, NodeMsg* msg);

    int procErrorAck(RouterCliData* data, int, NodeMsg* msg);
    
    void errRsp(RouterCliData* data);
    
    int procHeloMsg(RouterCliData* data, int hd, NodeMsg* msg);
    
    int procExchKeyMsg(RouterCliData* data, int hd, NodeMsg* msg);
    
    int procSend(RouterCliData* data, int, NodeMsg* msg);
    
    int procForward(RouterCliData* data, int, NodeMsg* msg);

    int procSendNoerr(RouterCliData* data, int, NodeMsg* msg);
    
    int procForwardNoerr(RouterCliData* data, int, NodeMsg* msg); 

    void notifyAllSid(RouterCliData* data, NodeMsg* msg);
    
    int procPeerUsrExit(RouterCliData* data, int, NodeMsg* msg);

    bool _existSid(const RouterCliData* data, const unsigned& sid);
    
    int _findSid(const RouterCliData* data, const unsigned& sid);

    int _sendMsg(RouterCliData* data, NodeMsg* msg);
    
    int _forward(RouterCliData* data, unsigned sid, NodeMsg* msg);

private:
    static const Worker m_north_works[ENUM_MSG_CODE_END];
    static const Worker m_south_works[ENUM_MSG_CODE_END];
    SockFrame* m_frame;
    LoginCenter* m_center;
    AgentProto* m_proto;
    AgentCenter* m_agent;
    unsigned m_next_sid;
    typeMapUsr m_users;
    typeConfs m_confs;
};

#endif

