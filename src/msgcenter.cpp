#include<cstring>
#include<cstdlib>
#include"isockmsg.h"
#include"agentdata.h"
#include"msgcenter.h"
#include"misc.h"
#include"msgtool.h"


unsigned MsgCenter::m_seq = MiscTool::getPid();

NodeMsg* MsgCenter::allocMsg(int size) {
    NodeMsg* msg = NULL;

    msg = MsgTool::creatPreMsg<PreHeadData>(size);
    return msg;
}

void MsgCenter::freeMsg(NodeMsg* msg) {
    MsgTool::freeMsg(msg);
}

NodeMsg* MsgCenter::refNodeMsg(NodeMsg* pb) {
    NodeMsg* msg = NULL;

    msg = MsgTool::refPreMsg<PreHeadData>(pb);
    return msg;
}

PreHeadData* MsgCenter::getPreHead(NodeMsg* pb) {
    PreHeadData* ph = NULL;

    ph = MsgTool::getPreHead<PreHeadData>(pb);
    return ph;
}

MsgHead_t* MsgCenter::getHead(NodeMsg* pMsg) {
    char* pc = NULL;

    pc = MsgTool::getMsg(pMsg);
    return reinterpret_cast<MsgHead_t*>(pc);
}

char* MsgCenter::_getBody(NodeMsg* pMsg) {
    char* pc = NULL;

    pc = MsgTool::getMsg(pMsg);
    pc += sizeof(MsgHead_t);
    return pc;
}

int MsgCenter::getMsgLeft(NodeMsg* msg) {
    int size = MsgTool::getMsgSize(msg);
    int pos = MsgTool::getMsgPos(msg);

    return size - pos;
}

void MsgCenter::setMsgSize(NodeMsg* msg, int size) {
    return MsgTool::setMsgSize(msg, size);
}

int MsgCenter::getMsgSize(NodeMsg* msg) {
    return MsgTool::getMsgSize(msg);
}

void MsgCenter::fillMsg(NodeMsg* msg, 
    const void* buf, int len) {
    char* psz = MsgTool::getMsg(msg);
    int pos = MsgTool::getMsgPos(msg);

    CacheUtil::bcopy(psz + pos, buf, len); 
    MsgTool::skipMsgPos(msg, len);
}

void MsgCenter::skipMsgPos(NodeMsg* msg, 
    int pos) {
    MsgTool::skipMsgPos(msg, pos);
}

bool MsgCenter::chkValidHead(const MsgHead_t* ph) {
    if (CURR_VERSION == ph->m_version
        && DEF_MSG_HEAD_SIZE <= ph->m_size
        && MAX_MSG_SIZE >=  ph->m_size) {
        return true;
    } else {
        LOG_ERROR("chk_valid_head| version=0x%x|"
            " size=%d| error=invalid head|", 
            ph->m_version, ph->m_size);
        return false;
    }
}

unsigned MsgCenter::calcMac(const void*, int len) {
    return len;
}

void MsgCenter::setHeadCmd(NodeMsg* msg, unsigned cmd) {
    MsgHead_t* ph = getHead(msg);
    
    ph->m_version = CURR_VERSION;
    ph->m_seq = ATOMIC_INC_FETCH(&m_seq);
    ph->m_cmd = cmd;
}

void MsgCenter::cloneAddr(NodeMsg* dstMsg, NodeMsg* srcMsg) {
    MsgHead_t* ph1 = getHead(dstMsg);
    MsgHead_t* ph2 = getHead(srcMsg);

    ph1->m_uid1 = ph2->m_uid1;
    ph1->m_sid1 = ph2->m_sid1;
    ph1->m_uid2 = ph2->m_uid2;
    ph1->m_sid2 = ph2->m_sid2;
}

void MsgCenter::inverseAddr(NodeMsg* dstMsg, NodeMsg* srcMsg) {
    MsgHead_t* ph1 = getHead(dstMsg);
    MsgHead_t* ph2 = getHead(srcMsg);

    ph1->m_uid1 = ph2->m_uid2;
    ph1->m_sid1 = ph2->m_sid2;
    ph1->m_uid2 = ph2->m_uid1;
    ph1->m_sid2 = ph2->m_sid1;
}
     
void MsgCenter::setHeadSize(NodeMsg* msg, int size) {
    MsgHead_t* ph = getHead(msg);
    
    ph->m_size = size;
}

void MsgCenter::setSrc(NodeMsg* msg, 
    unsigned uid, unsigned sid) {
    MsgHead_t* ph = getHead(msg);
    
    ph->m_uid1 = uid;
    ph->m_sid1 = sid;
}

void MsgCenter::setDst(NodeMsg* msg, 
    unsigned uid, unsigned sid) {
    MsgHead_t* ph = getHead(msg);
    
    ph->m_uid2 = uid;
    ph->m_sid2 = sid;
}

void MsgCenter::setSeq(NodeMsg* msg, unsigned seq) {
    MsgHead_t* ph = getHead(msg);
    
    ph->m_seq = seq;
}

unsigned MsgCenter::getSeq(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_seq;
}

unsigned MsgCenter::getSuid(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_uid1;
}

unsigned MsgCenter::getDuid(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_uid2;
}

unsigned MsgCenter::getSsid(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_sid1;
}

unsigned MsgCenter::getDsid(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_sid2;
}

void MsgCenter::setDirection(NodeMsg* msg, 
    int direction) {
    PreHeadData* ph = getPreHead(msg);
    
    ph->m_direction = direction;
}

int MsgCenter::getDirection(NodeMsg* msg) {
    PreHeadData* ph = getPreHead(msg);
    
    return ph->m_direction;
}

int MsgCenter::getHeadSize(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_size;
}

unsigned MsgCenter::getCmd(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);

    return ph->m_cmd;
}

void MsgCenter::dspMsgHead(const char* promt, 
    NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);
    int enDir = getDirection(msg);
    
    (void)promt;
    (void)ph;
    (void)enDir;
    LOG_INFO("******%s_msg_head| dir=%c| cmd=0x%x|"
        " version=0x%x| seq=%u| size=%d|"
        " suid=%u| ssid=%u| duid=%u| dsid=%u|",
        promt,
        ENUM_MSG_NORTH == enDir ? 'N' :
            ENUM_MSG_SOUTH == enDir ? 'S' : 'I',
        ph->m_cmd, ph->m_version,
        ph->m_seq, ph->m_size,
        ph->m_uid1, ph->m_sid1,
        ph->m_uid2, ph->m_sid2);
}

int MsgCenter::chkHeadMsg(NodeMsg* msg) {
    MsgHead_t* ph = getHead(msg);
    int ret = 0;
    int hsize = 0;
    int msize = 0;
    int enDir = 0;

    do {
        hsize = getHeadSize(msg);
        msize = getMsgSize(msg);
        enDir = getDirection(msg);
        
        if (hsize != msize) {
            ret = -1;
            break;
        }
        
        if (0 != ph->m_crc) {
            ret = -2;
            break;
        }

        if (ENUM_MSG_NORTH != enDir &&
            ENUM_MSG_SOUTH != enDir) {
            ret = -3;
            break;
        }

        return 0;
    } while (0);

    LOG_ERROR("chk_msg_head| ret=%d| hsize=%d| m_size=%d|"
        " crc=0x%x| dir=%c| cmd=0x%x| msg=invalid msg|",
        ret, hsize, msize, ph->m_crc,
        ENUM_MSG_NORTH == enDir ? 'N' :
            ENUM_MSG_SOUTH == enDir ? 'S' : 'I',
        ph->m_cmd);

    return 0;
}

int MsgCenter::getPlainOffset() {
    return DEF_MSG_HEAD_SIZE + sizeof(MsgPlainTxt);
}

int MsgCenter::getCipherOffset() {
    return DEF_MSG_HEAD_SIZE + sizeof(MsgCipherTxt);
}


