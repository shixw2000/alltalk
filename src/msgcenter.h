#ifndef __MSGCENTER_H__
#define __MSGCENTER_H__
#include"shareheader.h"
#include"isockmsg.h"


struct NodeMsg;
struct MsgHead_t;

class MsgCenter {
public:
    template<typename T>
    static NodeMsg* creatMsg(unsigned cmd, int extLen = 0) {
        NodeMsg* msg = NULL;
        int total = sizeof(T) 
            + DEF_MSG_HEAD_SIZE + extLen;

        msg = allocMsg(total);
        setHeadCmd(msg, cmd);
        setHeadSize(msg, total);
        return msg;
    }

    template<typename T>
    static T* getBody(NodeMsg* pMsg) {
        char* pc = _getBody(pMsg);

        return reinterpret_cast<T*>(pc);
    } 
    
    static void freeMsg(NodeMsg* pb);
    static NodeMsg* allocMsg(int size);
    static NodeMsg* refNodeMsg(NodeMsg* pb);
    static PreHeadData* getPreHead(NodeMsg* pb); 
    static MsgHead_t* getHead(NodeMsg* pMsg); 
    
    static void fillMsg(NodeMsg* msg, const void* buf, int len);
    static void skipMsgPos(NodeMsg* pb, int pos);
    static void flip(NodeMsg* pb);
    
    static void setDirection(NodeMsg* msg, int direction);
    static int getDirection(NodeMsg* msg);
    
    static unsigned calcMac(const void* data, int len);

    static void setHeadCmd(NodeMsg* msg, unsigned cmd); 
    static void setHeadSize(NodeMsg* msg, int size); 
    static void setSrc(NodeMsg* msg, unsigned uid, unsigned sid);
    static void setDst(NodeMsg* msg, unsigned uid, unsigned sid);
    static void cloneAddr(NodeMsg* dstMsg, NodeMsg* srcMsg);
    static void inverseAddr(NodeMsg* dstMsg, NodeMsg* srcMsg);

    static void setMsgSize(NodeMsg* msg, int size);
    static int getMsgLeft(NodeMsg* msg);
    static int getMsgSize(NodeMsg* msg);

    static void setSeq(NodeMsg* msg, unsigned seq);
    static unsigned getSeq(NodeMsg* msg);
    
    static int getHeadSize(NodeMsg* msg);
    static unsigned getCmd(NodeMsg* msg); 
    static unsigned getSuid(NodeMsg* msg);
    static unsigned getDuid(NodeMsg* msg);
    static unsigned getSsid(NodeMsg* msg);
    static unsigned getDsid(NodeMsg* msg);

    static void dspMsgHead(const char* promt, NodeMsg* msg);
    static int chkHeadMsg(NodeMsg* msg);

    static bool chkValidHead(const MsgHead_t*); 

    static int getPlainOffset();
    static int getCipherOffset(); 

private:
    static char* _getBody(NodeMsg* pMsg);

private:
    static unsigned m_seq;
};

#endif

