#include<cstdlib>
#include<cstring>
#include"usercenter.h"
#include"sockframe.h"
#include"misc.h"
#include"msgcenter.h"
#include"logincenter.h"
#include"agentproto.h"
#include"cache.h"


const SvrCenter::Worker SvrCenter::m_north_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_CIPHER, &SvrCenter::procCipher},
        
    {ENUM_MSG_UNREACH, &SvrCenter::procForwardNoerr},
    {ENUM_MSG_TUNNEL_CLOSE, &SvrCenter::procForwardNoerr},
        
    {ENUM_MSG_TUNNEL_REQ, &SvrCenter::procForward},
    {ENUM_MSG_TUNNEL_ACK, &SvrCenter::procForward},
        
    {ENUM_MSG_ERROR_ACK, &SvrCenter::procErrorAck}, 
    {ENUM_MSG_LOGOUT, &SvrCenter::procLogout}, 
    {ENUM_MSG_LOGIN_REQ, &SvrCenter::procLoginReq},
    {ENUM_MSG_LOGIN_END, &SvrCenter::procLoginEnd},
    
    {0, NULL}
}; 

const SvrCenter::Worker SvrCenter::m_south_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_PLAIN, &SvrCenter::procPlain},
        
    {ENUM_MSG_UID_EXIT_BROADCAST, &SvrCenter::procSendNoerr},
        
    {ENUM_MSG_UNREACH, &SvrCenter::procSendNoerr},
    {ENUM_MSG_TUNNEL_CLOSE, &SvrCenter::procSendNoerr},

    {ENUM_MSG_TUNNEL_REQ, &SvrCenter::procSend},
    {ENUM_MSG_TUNNEL_ACK, &SvrCenter::procSend},
    
    {0, NULL}
};

static bool __timerOneSec(long param1, long) {
    SvrCenter* center = (SvrCenter*)param1;

    center->timerOneSec();
    return true;
}

SvrCenter::SvrCenter(SockFrame* frame, LoginCenter* center,
        AgentProto* proto) : m_cap(MAX_SVR_DATA_CAP) {
    m_frame = frame;
    m_timer_1sec = NULL;
    m_center = center;
    m_proto = proto;
}

SvrCenter::~SvrCenter() {
}

int SvrCenter::init() {
    int ret = 0;

    m_timer_1sec = m_frame->allocTimer();
    m_frame->setParam(m_timer_1sec, &__timerOneSec, (long)this); 

    return ret;
}

void SvrCenter::finish() {
    if (NULL != m_timer_1sec) {
        m_frame->freeTimer(m_timer_1sec);
        m_timer_1sec = NULL;
    }
    
    if (!m_users.empty()) {
        for (typeItr itr=m_users.begin(); 
            itr!=m_users.end(); ++itr) {
            freeData(itr->second);
        }
            
        m_users.clear();
    }

    if (!m_confs.empty()) {
        for (int i=0; i<(int)m_confs.size(); ++i) {
            freeConf(m_confs[i]);
        }

        m_confs.clear();
    }
}

void SvrCenter::start() {
    RouterSvrConf* conf = NULL;
    
    for (int i=0; i<(int)m_confs.size(); ++i) {
        conf = m_confs[i];
        
        m_frame->creatSvr(conf->m_ip, conf->m_port, this, (long)conf);
    }

    m_frame->startTimer(m_timer_1sec, 0, 1);
} 

void SvrCenter::timerOneSec() {
    LOG_DEBUG("timer_one_second|");
}

int SvrCenter::addListener(const char ip[],
    int port, unsigned uid) {
    RouterSvrConf* conf = NULL;
    int ret = 0;

    conf = allocConf();
    if (NULL != conf) {
        conf->m_port = port;
        conf->m_uid2 = uid;
        strncpy(conf->m_ip, ip, sizeof(conf->m_ip)-1);
        
        m_confs.push_back(conf);
    } else {
        ret = -1;
    }

    return ret;
}

void SvrCenter::onListenerClose(int hd) {
    RouterSvrConf* conf = NULL;

    conf = (RouterSvrConf*)m_frame->getExtra(hd);

    (void)conf;
}

void SvrCenter::onClose(int hd) {
    RouterSvrData* data = NULL; 

    data = (RouterSvrData*)m_frame->getExtra(hd);
    
    delUsr(data);

    broadcastUidExit(data->m_login.m_uid1);
    
    freeData(data);
}

int SvrCenter::onNewSock(int parentId,
    int newFd, AccptOption& opt) {
    RouterSvrConf* conf = NULL;
    RouterSvrData* data = NULL; 
    NodeMsg* pRsp = NULL;
    
    conf = (RouterSvrConf*)m_frame->getExtra(parentId);

    data = allocData();
    if (NULL != data) {
        data->m_conf = conf;
        data->m_self_fd = newFd; 
        
        data->m_login.m_timestamp = MiscTool::getTimeSec();
        data->m_login.m_passwd1 = conf->m_passwd1;
        data->m_login.m_passwd2 = conf->m_passwd2;
        data->m_login.m_uid2 = conf->m_uid2;
        data->m_login.m_stat = ENUM_ROUTER_SVR_HELO; 
        
        pRsp = m_center->genHeloMsg(data->m_login); 
        _sendMsg(data, pRsp);
            
        opt.m_extra = (long)data;
        return 0;
    } else {
        return -1;
    }
}

int SvrCenter::parseData(int fd, const char* buf, 
    int size, const SockAddr*) {
    RouterSvrData* data = NULL;
    int ret = 0;

    data = (RouterSvrData*)m_frame->getExtra(fd);
    ret = m_proto->parseData(ENUM_MSG_NORTH, fd, 
        &data->m_cache, buf, size);
    return ret;
}

int SvrCenter::process(int hd, NodeMsg* msg) { 
    int ret = 0;
    int enDir = 0;
    
    ret = MsgCenter::chkHeadMsg(msg);
    if (0 == ret) {
        enDir = MsgCenter::getDirection(msg); 
        switch (enDir) {
        case ENUM_MSG_NORTH:
            ret = procNorthMsg(hd, msg);
            break;

        case ENUM_MSG_SOUTH:
            ret = procSouthMsg(hd, msg);
            break;

        default:
            LOG_ERROR("invalid direction msg|");
            ret = -1;
            break;
        } 
    } 

    if (0 != ret) {
        MsgCenter::dspMsgHead("proc_router", msg);
    }
        
    return ret;
}

int SvrCenter::procNorthMsg(int hd, NodeMsg* msg) {
    RouterSvrData* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (RouterSvrData*)m_frame->getExtra(hd); 

    LOG_DEBUG("proc_north_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_login.m_stat);

    for (pHd = m_north_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            break;
        }
    } 
    
    if (NULL != pHd->m_fn) {
        ret = (this->*(pHd->m_fn))(data, hd, msg);
        if (0 != ret) {
            LOG_ERROR("proc_north_msg| ret=%d| cmd=0x%x| stat=%d|" 
                " msg=deal error|",
                ret, cmd, data->m_login.m_stat);
        }
    } else {
        LOG_DEBUG("proc_north_msg| cmd=0x%x| stat=%d|" 
            " msg=unknown msg|",
            cmd, data->m_login.m_stat);
    }

    return ret;
}

int SvrCenter::procSouthMsg(int hd, NodeMsg* msg) {
    RouterSvrData* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (RouterSvrData*)m_frame->getExtra(hd); 

    LOG_DEBUG("proc_south_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_login.m_stat);

    for (pHd = m_south_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            break;
        }
    } 
    
    if (NULL != pHd->m_fn) {
        ret = (this->*(pHd->m_fn))(data, hd, msg);
        if (0 != ret) {
            LOG_ERROR("proc_south_msg| ret=%d| cmd=0x%x| stat=%d|"
                " msg=deal error|",
                ret, cmd, data->m_login.m_stat);
        }
    } else {
        LOG_DEBUG("proc_south_msg| cmd=0x%x| stat=%d|"
            " msg=unknown msg|", 
            cmd, data->m_login.m_stat);
    }

    return ret;
}

void SvrCenter::resetConf(RouterSvrConf* conf) {
    MiscTool::bzero(conf, sizeof(*conf));

    conf->m_passwd1 = 0x12345678;
    conf->m_passwd2 = 0x987654321;
}

void SvrCenter::reset(RouterSvrData* data) {
    MiscTool::bzero(data, sizeof(*data));
}

RouterSvrData* SvrCenter::allocData() {
    RouterSvrData* data = NULL;

    data = (RouterSvrData*)CacheUtil::mallocAlign(sizeof(RouterSvrData));
    reset(data);
    return data;
}

void SvrCenter::freeData(RouterSvrData* data) {
    CacheUtil::freeAlign(data);
}

RouterSvrConf* SvrCenter::allocConf() {
    RouterSvrConf* data = NULL;

    data = (RouterSvrConf*)CacheUtil::mallocAlign(sizeof(RouterSvrConf));
    resetConf(data);
    return data;
}

void SvrCenter::freeConf(RouterSvrConf* conf) {
    CacheUtil::freeAlign(conf);
}

bool SvrCenter::existUsr(unsigned uid) {
    typeConstItr itr = m_users.find(uid);

    if (m_users.end() != itr) {
        return true;
    } else {
        return false;
    }
}

RouterSvrData* SvrCenter::findUsr(unsigned uid) {
    typeConstItr itr = m_users.find(uid);

    if (m_users.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

void SvrCenter::addUsr(RouterSvrData* data) {
    unsigned uid = data->m_login.m_uid1;
    
    if (0 < uid) {
        m_users[uid] = data;
    }
}

void SvrCenter::delUsr(RouterSvrData* data) {
    unsigned uid = data->m_login.m_uid1;
    typeItr itr = m_users.find(uid);

    if (m_users.end() != itr) {
        m_users.erase(itr);
    } 
}

void SvrCenter::errRsp(RouterSvrData* data) {
    NodeMsg* pRsp = NULL;
    
    pRsp = m_center->genErrorMsg(data->m_login); 
    
    _sendMsg(data, pRsp);
    return;
}

int SvrCenter::procLoginReq(RouterSvrData* data, int, NodeMsg* msg) {
    NodeMsg* pRsp = NULL; 
    int ret = 0; 

    ret = m_center->parseLoginReqMsg(data->m_login, msg);
    if (0 == ret) {
        pRsp = m_center->genExchKeyMsg(data->m_login);
        
        ret = _sendMsg(data, pRsp);
    } else {
        errRsp(data);
    } 
    
    return ret;
}

int SvrCenter::procLoginEnd(RouterSvrData* data,
    int, NodeMsg* msg) {
    int ret = 0;
    unsigned uid = 0;
    
    ret = m_center->parseLoginEnd(data->m_login, msg);
    if (0 == ret) {
        uid = data->m_login.m_uid1;
        
        if (!existUsr(uid)) {
            /* start to work normally */ 
            addUsr(data);
        } else {
            /* */
            LOG_ERROR("proc_login| uid=%u| msg=user already login|",
                uid);
            ret = -1;
        }
    } else {
        errRsp(data);
    }
    
    return ret;
} 

int SvrCenter::procLogout(RouterSvrData* data,
    int, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->parseLogoutMsg(data->m_login, msg);
    if (0 == ret) { 
        /* exit now */
        closeSvr(data);
    } else {
        /* ignore */
    } 
    
    return ret;
}

int SvrCenter::procErrorAck(RouterSvrData* data,
    int, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->parseErrorAck(data->m_login, msg);
    if (0 == ret) { 
        /* exit now */
        closeSvr(data);
    } else {
        /* ignore */
    } 
    
    return ret;
}

void SvrCenter::closeSvr(RouterSvrData* data) {
    if (0 < data->m_self_fd) {
        m_frame->closeData(data->m_self_fd);
    }
}

int SvrCenter::procCipher(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned duid = 0;
    int ret = 0;

    ret = m_center->parseCipherMsg(data->m_login, msg);
    if (0 == ret) {
        outMsg = m_center->cipher2PlainMsg(
            data->m_login.m_secret, msg);
        
        duid = MsgCenter::getDuid(outMsg);
        ret = _forward(duid, outMsg); 
        if (0 != ret) {
            rspUsrUnreach(data, msg);

            /* here donot fail */
            ret = 0;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

int SvrCenter::procPlain(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0;

    if (m_center->hasAuthed(data->m_login)) {
        outMsg = m_center->plain2CipherMsg(
            data->m_login.m_secret, msg);

        ret = _sendMsg(data, outMsg);
        if (0 != ret) {
            rspUsrUnreach(data, msg);

            /* here donot fail */
            ret = 0;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

int SvrCenter::procForward(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned duid = 0;
    int ret = 0; 

    if (m_center->hasAuthed(data->m_login)) {
        outMsg = MsgCenter::refNodeMsg(msg);
        
        duid = MsgCenter::getDuid(outMsg);
        ret = _forward(duid, outMsg);
        if (0 != ret) {
            rspUsrUnreach(data, msg);

            /* here donot fail */
            ret = 0;
        }
    } else {
        ret = -1;
    }
    
    return ret;
}

int SvrCenter::procSend(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 
        
    outMsg = MsgCenter::refNodeMsg(msg);
    
    ret = _sendMsg(data, outMsg); 
    if (0 != ret) {
        rspUsrUnreach(data, msg);
    }
    
    return ret;
} 

int SvrCenter::procForwardNoerr(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned duid = 0;
    int ret = 0; 

    if (m_center->hasAuthed(data->m_login)) {
        outMsg = MsgCenter::refNodeMsg(msg);
        
        duid = MsgCenter::getDuid(outMsg);
        ret = _forward(duid, outMsg);

        /* here donot fail */
        ret = 0;
    } else {
        ret = -1;
    }
    
    return ret;
}

int SvrCenter::procSendNoerr(RouterSvrData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 
        
    outMsg = MsgCenter::refNodeMsg(msg);
    
    ret = _sendMsg(data, outMsg); 
    
    return ret;
} 

void SvrCenter::notifyAllUsr(NodeMsg* msg) {
    RouterSvrData* data = NULL;
    NodeMsg* ref = NULL;
    
    if (!m_users.empty()) {
        for (typeConstItr itr = m_users.begin();
            itr != m_users.end(); ++itr) {
            data = itr->second;
            
            ref = MsgCenter::refNodeMsg(msg);
            _forward(data->m_login.m_uid1, ref);
        } 
    }
}

void SvrCenter::broadcastUidExit(unsigned uid) {
    NodeMsg* msg = NULL;

    if (!m_users.empty()) {
        msg = m_center->genUserExitMsg(uid, 0); 

        notifyAllUsr(msg); 
        MsgCenter::freeMsg(msg);
    }
}

void SvrCenter::rspUsrUnreach(
    RouterSvrData* data, NodeMsg* origin) {
    NodeMsg* errMsg = NULL;
    unsigned uid = 0;
    unsigned suid = 0;
    unsigned ssid = 0;
    unsigned duid = 0;

    uid = data->m_login.m_uid1;
    suid = MsgCenter::getSuid(origin);
    ssid = MsgCenter::getSsid(origin);
    duid = MsgCenter::getDuid(origin);

    if (0 < ssid) {
        if (suid == uid && duid != uid) {
            errMsg = m_center->genUnreachMsg(
                ENUM_ERR_NO_USER, uid, 0, origin);
            
            /* down to children */
            _sendMsg(data, errMsg);
        } else if (duid == uid && suid != uid) {
            errMsg = m_center->genUnreachMsg(
                ENUM_ERR_NO_USER, uid, 0, origin);
            
            /* rebound to src */
            _forward(suid, errMsg);
        } else {
            /* ignore */
        }
    }
}

int SvrCenter::_sendMsg(RouterSvrData* data,
    NodeMsg* msg) {
    int fd = data->m_self_fd;
    int ret = 0;

    MsgCenter::setDirection(msg, ENUM_MSG_NORTH);
        
    ret = m_frame->sendMsg(fd, msg);
    if (0 != ret) {
        LOG_INFO("user_send| uid=%u|"
            " fd=%d| ret=%d| msg=error|", 
            data->m_login.m_uid1, fd, ret);
    }

    return ret;
}

int SvrCenter::_forward(unsigned uid, NodeMsg* msg) {
    RouterSvrData* data = NULL;
    int ret = 0;
    int fd = 0;

    if (0 < uid && existUsr(uid)) {
        data = findUsr(uid);
        fd = data->m_self_fd; 

        MsgCenter::setDirection(msg, ENUM_MSG_SOUTH);
        
        ret = m_frame->dispatch(fd, msg);
        if (0 != ret) {
            LOG_INFO("user_forward| uid=%u| fd=%d|"
                " ret=%d| msg=dispatch error|", 
                uid, fd, ret);
        }
    } else {
        LOG_INFO("user_forward| uid=%u| msg=not exists|", uid); 

        /* delete msg if failed */
        MsgCenter::freeMsg(msg);
        ret = -1;
    }

    return ret;
}

