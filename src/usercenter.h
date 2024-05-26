#ifndef __USERCENTER_H__
#define __USERCENTER_H__
#include<map>
#include<vector>
#include"agentdata.h" 
#include"isockapi.h"


class SockFrame;
class LoginCenter;
class AgentProto;

class SvrCenter : public ISockSvr {
public:
    typedef int (SvrCenter::*PFunc)(RouterSvrData*, int, NodeMsg*);

    struct Worker {
        unsigned m_cmd;
        PFunc m_fn;
    };
    
    typedef std::map<unsigned, RouterSvrData*> typeMapUsr;
    typedef typeMapUsr::const_iterator typeConstItr;
    typedef typeMapUsr::iterator typeItr;

    typedef std::vector<RouterSvrConf*> typeVecConfs;
    
public:
    SvrCenter(SockFrame* frame, LoginCenter* center,
        AgentProto* proto);
    ~SvrCenter();
    
    int init();
    void finish(); 

    void start(); 

    int addListener(const char ip[],
        int port, unsigned uid);
    
    bool existUsr(unsigned uid);
    RouterSvrData* findUsr(unsigned uid);
    void addUsr(RouterSvrData* data);
    void delUsr(RouterSvrData* data); 

    virtual int onNewSock(int parentId,
        int newId, AccptOption& opt);

    virtual void onListenerClose(int hd);

    virtual int parseData(int fd, const char* buf, 
        int size, const SockAddr* addr);
    
    virtual int process(int hd, NodeMsg* msg);
    
    virtual void onClose(int hd);

    void onCloseListener(int hd);

    void timerOneSec();

private:
    int procNorthMsg(int hd, NodeMsg* msg);
    int procSouthMsg(int hd, NodeMsg* msg);

    int _sendMsg(RouterSvrData* data, NodeMsg* msg); 
    int _forward(unsigned uid, NodeMsg* msg);
    
    void closeSvr(RouterSvrData* data);
    
    void resetConf(RouterSvrConf* conf);
    void reset(RouterSvrData* data);
    
    RouterSvrData* allocData();
    void freeData(RouterSvrData*);
    
    RouterSvrConf* allocConf();
    void freeConf(RouterSvrConf*);

    RouterSvrData* reg(int fd);
    void unreg(RouterSvrData* data); 
    bool existsFd(int fd);

    int procLoginReq(RouterSvrData* data,
        int hd, NodeMsg* msg);
    
    int procLoginEnd(RouterSvrData* data,
        int hd, NodeMsg* msg);

    void errRsp(RouterSvrData* data);

    int chkLogoutMsg(const RouterSvrData* data,
        const MsgLogout* body);

    int procLogout(RouterSvrData* data,
        int hd, NodeMsg* msg);

    int procErrorAck(RouterSvrData* data,
        int hd, NodeMsg* msg);

    int procCipher(RouterSvrData* data,
        int hd, NodeMsg* msg);

    int procPlain(RouterSvrData* data, 
        int hd, NodeMsg* msg);

    int procForward(RouterSvrData* data, 
        int, NodeMsg* msg);

    int procSend(RouterSvrData* data, int, NodeMsg* msg);

    int procForwardNoerr(RouterSvrData* data, 
        int, NodeMsg* msg);

    int procSendNoerr(RouterSvrData* data, int, NodeMsg* msg);

    void rspUsrUnreach(RouterSvrData* data, NodeMsg* origin);

    void notifyAllUsr(NodeMsg* msg);
    
    void broadcastUidExit(unsigned uid);

private:
    static const Worker m_north_works[ENUM_MSG_CODE_END];
    static const Worker m_south_works[ENUM_MSG_CODE_END];
    const int m_cap;
    SockFrame* m_frame;
    LoginCenter* m_center;
    AgentProto* m_proto;
    typeMapUsr m_users;
    typeVecConfs m_confs;
};

#endif

