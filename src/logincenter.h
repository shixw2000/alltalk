#ifndef __LOGINCENTER_H__
#define __LOGINCENTER_H__
#include"agentdata.h"
#include"isockmsg.h"


struct NodeMsg;

class LoginCenter {
public:
    LoginCenter();

    int init();
    void finish();

    int getEncLen(const unsigned char secret[], int inlen);
    int encData(const unsigned char secret[],
        const void* in, int inlen, void* out);

    int getDecLen(const unsigned char secret[], int inlen);
    int decData(const unsigned char secret[],
        const void* in, int inlen, void* out);
    
    NodeMsg* genHeloMsg(const LoginData& login);
    
    NodeMsg* genErrorMsg(const LoginData& login);

    NodeMsg* genLogoutMsg(const LoginData& login);
    
    NodeMsg* genLoginReqMsg(const LoginData& login);
    
    NodeMsg* genLoginEndMsg(const LoginData& login);
    
    NodeMsg* genExchKeyMsg(const LoginData& login); 
    
    NodeMsg* genUnreachMsg(int reason, 
        unsigned from_uid, unsigned from_sid, 
        NodeMsg* origin);

    NodeMsg* genPlainMsg(unsigned link_no,
        const char* txt, int len);

    NodeMsg* genTunnelMsg(unsigned uid, unsigned sid,
        unsigned link_no, int link_type, 
        const char ip[], int port);

    NodeMsg* genTunnelAckMsg(unsigned sid1, unsigned uid2, 
        unsigned sid2, unsigned link_no);

    NodeMsg* genCloseTunnelMsg(unsigned uid,
        unsigned sid, int reason);

    NodeMsg* genUserExitMsg(unsigned uid, int reason);

    void creatTokenReq(unsigned long& token,
        const unsigned long& passwd1, 
        const unsigned uid1,
        const unsigned random1);

    void creatTokenExch(unsigned long& token,
        const unsigned long& passwd1, 
        const unsigned long& passwd2,
        const unsigned random1,
        const unsigned random2);

    void creatTokenEnd(unsigned long& token,
        const unsigned char secret[], 
        const unsigned long& taskid,
        const unsigned timestamp,
        const unsigned uid1);

    void creatSecret(unsigned char secret[],
        const unsigned long& passwd1, 
        const unsigned long& passwd2,
        const unsigned random1,
        const unsigned random2,
        const unsigned uid1);

    int chkLoginReqMsg(
        const LoginData& login,
        const MsgLoginReq* body); 

    int chkLoginEndMsg(
        const LoginData& login,
        const MsgLoginEnd* body);

    int parseErrorAck(LoginData& login, NodeMsg* msg);
    
    int parseLogoutMsg(LoginData& login, NodeMsg* msg);
    
    int parseLoginReqMsg(LoginData& login,
        NodeMsg* msg);

    int parseExchKeyMsg(LoginData& login, NodeMsg* msg);

    int parseHeloMsg(LoginData& login, NodeMsg* msg);

    int parseLoginEnd(LoginData& login, NodeMsg* msg);

    int parseCipherMsg(LoginData& login, NodeMsg* msg);

    int parsePlainMsg(LoginData& login, NodeMsg* msg);

    NodeMsg* cipher2PlainMsg(
        const unsigned char secret[], NodeMsg* msg);

    NodeMsg* plain2CipherMsg(
        const unsigned char secret[], NodeMsg* msg);

    bool hasAuthed(const LoginData& login);

private:
    unsigned long next();

    int _encData(const unsigned char secret[],
        const void* in, int inlen, void* out);

private:
    unsigned long m_seq;
};

#endif

