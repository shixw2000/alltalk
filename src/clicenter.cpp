#include<cstdlib>
#include<cstring>
#include"clicenter.h"
#include"routeragent.h"
#include"sockframe.h"
#include"misc.h"
#include"msgcenter.h"
#include"logincenter.h"
#include"agentproto.h"
#include"cache.h"


const CliCenter::Worker CliCenter::m_north_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_PLAIN, &CliCenter::procPlain},

    {ENUM_MSG_TUNNEL_REQ, &CliCenter::procSend},
    {ENUM_MSG_TUNNEL_ACK, &CliCenter::procSend},
        
    {ENUM_MSG_TUNNEL_CLOSE, &CliCenter::procSendNoerr},
    {ENUM_MSG_UNREACH, &CliCenter::procSendNoerr},
        
    {0, NULL}
};

const CliCenter::Worker CliCenter::m_south_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_CIPHER, &CliCenter::procCipher}, 

    {ENUM_MSG_UID_EXIT_BROADCAST, &CliCenter::procPeerUsrExit},
    {ENUM_MSG_TUNNEL_REQ, &CliCenter::procTunnelReq}, 
    {ENUM_MSG_TUNNEL_ACK, &CliCenter::procForward},
        
    {ENUM_MSG_TUNNEL_CLOSE, &CliCenter::procForwardNoerr},
    {ENUM_MSG_UNREACH, &CliCenter::procForwardNoerr},
        
    {ENUM_MSG_ERROR_ACK, &CliCenter::procErrorAck}, 
    {ENUM_MSG_LOGOUT, &CliCenter::procLogout}, 
    {ENUM_MSG_HELO, &CliCenter::procHeloMsg},
    {ENUM_MSG_EXCH_KEY, &CliCenter::procExchKeyMsg},
    
    {0, NULL}
};


CliCenter::CliCenter(SockFrame* frame,
    LoginCenter* center, AgentProto* proto) {
    m_frame = frame;
    m_center = center;
    m_proto = proto; 
    m_agent = NULL;
}

CliCenter::~CliCenter() {
}

int CliCenter::init() {
    int ret = 0;

    return ret;
}

void CliCenter::finish() {
    RouterCliConf* conf = NULL;
    RouterCliData* data = NULL;

    if (!m_users.empty()) {
        for (typeMapUsrItr itr=m_users.begin(); 
            itr!=m_users.end(); ++itr) {
            data = itr->second;

            freeData(data);
        }

        m_users.clear();
    }
  
    if (!m_confs.empty()) {
        for (int i=0; i<(int)m_confs.size(); ++i) {
            conf = m_confs[i];

            freeConf(conf);
        }

        m_confs.clear();
    }
}

void CliCenter::setAgent(AgentCenter* agent) {
    m_agent = agent;
}

bool CliCenter::validUsr(unsigned uid) {
    RouterCliData* data = NULL;
    typeMapUsrItrConst itr;

    itr = m_users.find(uid);
    if (m_users.end() != itr) {
        data = itr->second;

        return ENUM_ROUTER_AUTH_OK == data->m_login.m_stat;
    } else {
        return false;
    }
}

bool CliCenter::hasUsr(unsigned uid) {
    RouterCliConf* conf = NULL;
    
    for (int i=0; i<(int)m_confs.size(); ++i) {
        conf = m_confs[i];
        if (conf->m_uid1 == uid) {
            return true;
        }
    }

    return false;
}

int CliCenter::addCli(const char ip[], int port, unsigned uid) {
    RouterCliConf* conf = NULL;
    int ret = 0;

    if (0 < uid && !hasUsr(uid)) {
        conf = allocConf();
        if (NULL != conf) {
            conf->m_port = port;
            conf->m_uid1 = uid;
            strncpy(conf->m_ip, ip, sizeof(conf->m_ip)-1);

            m_confs.push_back(conf);
        } else {
            ret = -1;
        }
    } else {
        ret = -1;
    }

    return ret;
}

void CliCenter::start() {
    RouterCliData* data = NULL;
    RouterCliConf* conf = NULL;
    
    for (int i=0; i<(int)m_confs.size(); ++i) {
        conf = m_confs[i];

        data = allocData(conf);
        addUsr(data->m_login.m_uid1, data);
        
        m_frame->creatCli(conf->m_ip, conf->m_port, this, (long)data);
    }
}

RouterCliConf* CliCenter::allocConf() {
    RouterCliConf* data = NULL;

    data = (RouterCliConf*)CacheUtil::mallocAlign(sizeof(RouterCliConf));
    resetConf(data);
    return data;
}

void CliCenter::resetConf(RouterCliConf* conf) {
    MiscTool::bzero(conf, sizeof(*conf));

    conf->m_passwd1 = 0x12345678;
    conf->m_passwd2 = 0x987654321;
}

void CliCenter::freeConf(RouterCliConf* conf) {
    CacheUtil::freeAlign(conf);
}

RouterCliData* CliCenter::allocData(RouterCliConf* conf) {
    RouterCliData* data = NULL;

    data = new RouterCliData;
    if (NULL != data) {
        resetData(data);
        
        data->m_conf = conf;
        cleanCli(data); 
    }
    
    return data;
}

void CliCenter::freeData(RouterCliData* data) {
    if (NULL != data) {
        delete data;
    }
}

void CliCenter::resetData(RouterCliData* data) {
    MiscTool::bzero(&data->m_login, sizeof(data->m_login));
    MiscTool::bzero(&data->m_cache, sizeof(data->m_cache));
    
    data->m_conf = NULL;
    data->m_self_fd = 0;
    data->m_conn_delay = 0;
    data->m_sids.clear(); 
}

unsigned CliCenter::nextSid() {
    if (0 != ++m_next_sid) {
        return m_next_sid;
    } else {
        return ++m_next_sid;
    }
}

void CliCenter::cleanCli(RouterCliData* data) { 
    MiscTool::bzero(&data->m_login, sizeof(data->m_login));
    MiscTool::bzero(&data->m_cache, sizeof(data->m_cache));

    data->m_self_fd = 0;
    data->m_sids.clear();
    
    if (NULL != data->m_conf) {
        data->m_login.m_passwd1 = data->m_conf->m_passwd1;
        data->m_login.m_passwd2 = data->m_conf->m_passwd2;
        data->m_login.m_uid1 = data->m_conf->m_uid1;
    } 
}

void CliCenter::onClose(int hd) {
    RouterCliData* data = NULL;

    data = (RouterCliData*)m_frame->getExtra(hd);
    
    closeChildren(data); 
    cleanCli(data);
    
    reConn(data);
}

void CliCenter::closeChildren(RouterCliData* data) {
    if (!data->m_sids.empty()) {
        for (typeSidsItrConst itr = data->m_sids.begin();
            data->m_sids.end() != itr; ++itr) {
            m_frame->closeData(itr->second);
        }

        data->m_sids.clear();
    }
}

void CliCenter::onConnFail(long extra, int) { 
    RouterCliData* data = (RouterCliData*)extra;
    
    reConn(data);
}

void CliCenter::reConn(RouterCliData* data) {
    static const int DELAY_TIMEOUT[] = {
        1, 15, 60, 120,
    };
    
    for (int i=0; i<ARR_CNT(DELAY_TIMEOUT); ++i) {
        if (data->m_conn_delay < DELAY_TIMEOUT[i]) {
            data->m_conn_delay = DELAY_TIMEOUT[i];
            break;
        }
    }

    LOG_INFO("router_agent_reconn| delay=%d| addr=%s:%d|",
        data->m_conn_delay, data->m_conf->m_ip,
        data->m_conf->m_port);

    m_frame->sheduleCli(data->m_conn_delay, 
        data->m_conf->m_ip, data->m_conf->m_port, 
        this, (long)data);
}

int CliCenter::onConnOK(int hd, ConnOption&) {
    RouterCliData* data = NULL;
    int ret = 0;
    
    data = (RouterCliData*)m_frame->getExtra(hd);
    
    data->m_self_fd = hd;
    data->m_conn_delay = 0;
    data->m_login.m_stat = ENUM_ROUTER_INIT;

    return ret;
}

int CliCenter::parseData(int fd, const char* buf,
    int size, const SockAddr*) {
    RouterCliData* data = NULL;
    int ret = 0;

    data = (RouterCliData*)m_frame->getExtra(fd);
    ret = m_proto->parseData(ENUM_MSG_SOUTH, fd, 
        &data->m_cache, buf, size);
    return ret;
}

int CliCenter::process(int hd, NodeMsg* msg) { 
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
        MsgCenter::dspMsgHead("proc_cli", msg);
    }
        
    return ret;
}

int CliCenter::procNorthMsg(int hd, NodeMsg* msg) {
    RouterCliData* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (RouterCliData*)m_frame->getExtra(hd);

    LOG_DEBUG("proc_cli_north_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_login.m_stat);
  
    for (pHd = m_north_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            break;
        }
    } 

    if (NULL != pHd->m_fn) {
        ret = (this->*(pHd->m_fn))(data, hd, msg);
        if (0 != ret) {
            LOG_ERROR("proc_cli_north_msg| ret=%d|"
                " cmd=0x%x| stat=%d|"
                " msg=deal error|", 
                ret, cmd, data->m_login.m_stat);
        }
    } else {
        LOG_DEBUG("proc_cli_north_msg| cmd=0x%x| stat=%d|"
            " msg=unknown msg|", 
            cmd, data->m_login.m_stat);
    }

    return ret;
}

int CliCenter::procSouthMsg(int hd, NodeMsg* msg) {
    RouterCliData* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (RouterCliData*)m_frame->getExtra(hd);

    LOG_DEBUG("proc_cli_south_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_login.m_stat);

    for (pHd = m_south_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            break;
        }
    } 

    if (NULL != pHd->m_fn) {
        ret = (this->*(pHd->m_fn))(data, hd, msg);
        if (0 != ret) {
            LOG_ERROR("proc_cli_south_msg| ret=%d|"
                " cmd=0x%x| stat=%d|"
                " msg=deal error|", 
                ret, cmd, data->m_login.m_stat);
        }
    } else {
        LOG_DEBUG("proc_cli_south_msg|"
            " cmd=0x%x| stat=%d| msg=unknown msg|", 
            cmd, data->m_login.m_stat);
    }

    return ret;
} 

void CliCenter::rspSidUnreach(int reason,
    RouterCliData* data, NodeMsg* origin) {
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
                 reason, uid, 0, origin);
            
            /* down to children sid */
            _forward(data, ssid, errMsg);
        } else if (duid == uid && suid != uid) {
            errMsg = m_center->genUnreachMsg(
                reason, uid, 0, origin);
            
            /* rebound to other uid */
            _sendMsg(data, errMsg);
        } else {
            /* ignore */
        }
    }
}

int CliCenter::procCipher(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned dsid = 0;
    int ret = 0;
    
    ret = m_center->parseCipherMsg(data->m_login, msg);
    if (0 == ret) { 
        outMsg = m_center->cipher2PlainMsg(
            data->m_login.m_secret, msg);

        dsid = MsgCenter::getDsid(outMsg);

        /* here donot fail */
        ret = _forward(data, dsid, outMsg);
        if (0 != ret) {
            rspSidUnreach(ENUM_ERR_NO_SID, data, msg);
            ret = 0;
        }
    }

    return ret;
}

int CliCenter::procPlain(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0;

    ret = m_center->parsePlainMsg(data->m_login, msg);
    if (0 == ret) { 
        outMsg = m_center->plain2CipherMsg(
            data->m_login.m_secret, msg);

        ret = _sendMsg(data, outMsg);
        if (0 != ret) {
            rspSidUnreach(ENUM_ERR_NO_SID, data, msg);
        }
    } else {
        /* here donot fail */
        ret = 0;
    }
    
    return ret;
}

int CliCenter::procTunnelReq(RouterCliData*, 
    int hd, NodeMsg* msg) { 
    int ret = 0;

    /* here donot fail */
    m_agent->creatAgentCli(hd, msg); 
    
    return ret;
}

bool CliCenter::_existSid(const RouterCliData* data, 
    const unsigned& sid) {
    typeSidsItrConst itr = data->m_sids.find(sid);

    return data->m_sids.end() != itr;
}

int CliCenter::_findSid(const RouterCliData* data, 
    const unsigned& sid) {
    typeSidsItrConst itr = data->m_sids.find(sid);

    if (data->m_sids.end() != itr) {
        return itr->second;
    } else {
        return 0;
    }
}

unsigned CliCenter::regSid(const unsigned& uid, int fd) {
    RouterCliData* data = NULL;
    unsigned sid = 0;

    if (validUsr(uid)) {
        data = findUsr(uid);
        
        do {
            sid = nextSid();
        } while (_existSid(data, sid));
        
        data->m_sids[sid] = fd;

        LOG_DEBUG("reg_sid| uid=%u| fd=%d| sid=%u|",
            uid, fd, sid);
    } else {
        LOG_DEBUG("reg_sid| uid=%u| fd=%d| msg=invalid usr|",
            uid, fd);
    }

    return sid;
}

void CliCenter::unregSid(const unsigned& uid, 
    const unsigned& sid) {
    RouterCliData* data = NULL;
    typeSidsItr itr;

    data = findUsr(uid);
    if (NULL != data) { 
        itr = data->m_sids.find(sid);
        if (data->m_sids.end() != itr) {
            LOG_DEBUG("unreg_sid| uid=%u| sid=%u| fd=%d|",
                uid, sid, itr->second);
            
            data->m_sids.erase(itr); 
        } else {
            LOG_DEBUG("unreg_sid| uid=%u| sid=%u|", uid, sid);
        }
    }
}

RouterCliData* CliCenter::findUsr(const unsigned& uid) {
    typeMapUsrItrConst itr = m_users.find(uid);

    if (m_users.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

void CliCenter::addUsr(const unsigned& uid, 
    RouterCliData* data) {
    if (0 < uid && NULL != data) {
        m_users[uid] = data;
    }
}

void CliCenter::delUsr(const unsigned& uid) {
    m_users.erase(uid); 
}

void CliCenter::closeCli(RouterCliData* data) {
    if (0 < data->m_self_fd) { 
        m_frame->closeData(data->m_self_fd);
    }
}

int CliCenter::procLogout(RouterCliData* data,
    int, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->parseLogoutMsg(data->m_login, msg);
    if (0 == ret) { 
        /* exit now */
        closeCli(data);
    } else {
        /* ignore */
    } 
    
    return ret;
}

int CliCenter::procErrorAck(RouterCliData* data,
    int, NodeMsg* msg) {
    int ret = 0;

    ret = m_center->parseErrorAck(data->m_login, msg);
    if (0 == ret) { 
        /* exit now */
        closeCli(data);
    } else {
        /* ignore */
    } 
    
    return ret;
}

void CliCenter::errRsp(RouterCliData* data) {
    NodeMsg* pRsp = NULL;
    
    pRsp = m_center->genErrorMsg(data->m_login); 
    
    _sendMsg(data, pRsp);
    return;
}

int CliCenter::procHeloMsg(RouterCliData* data,
    int, NodeMsg* msg) {
    NodeMsg* pRsp = NULL;
    int ret = 0;

    ret = m_center->parseHeloMsg(data->m_login, msg); 
    if (0 == ret) {
        pRsp = m_center->genLoginReqMsg(data->m_login); 
        MsgCenter::setDirection(pRsp, ENUM_MSG_NORTH);
        
        ret = _sendMsg(data, pRsp);
    } else {
        errRsp(data);
    }
    
    return ret;
}

int CliCenter::procExchKeyMsg(RouterCliData* data, 
    int, NodeMsg* msg) {
    NodeMsg* pRsp = NULL;
    int ret = 0;

    ret = m_center->parseExchKeyMsg(data->m_login, msg);
    if (0 == ret) { 
        /* start to work */
        pRsp = m_center->genLoginEndMsg(data->m_login);
        MsgCenter::setDirection(pRsp, ENUM_MSG_NORTH);
        
        ret = _sendMsg(data, pRsp);
    } else {
        errRsp(data);
    }

    return ret;
}

int CliCenter::procSend(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 
        
    outMsg = MsgCenter::refNodeMsg(msg);
    ret = _sendMsg(data, outMsg);
    if (0 != ret) {
        rspSidUnreach(ENUM_ERR_NO_SID, data, msg);
    }
    
    return ret;
}

int CliCenter::procForward(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned dsid = 0;
    int ret = 0; 

    if (m_center->hasAuthed(data->m_login)) { 
        outMsg = MsgCenter::refNodeMsg(msg);
        
        dsid = MsgCenter::getDsid(outMsg);

        /* here donot fail */
        ret = _forward(data, dsid, outMsg);
        if (0 != ret) {
            rspSidUnreach(ENUM_ERR_NO_SID, data, msg);
            ret = 0;
        }
    }
    
    return ret;
}

void CliCenter::notifyAllSid(RouterCliData* data,
    NodeMsg* msg) {
    NodeMsg* ref = NULL;
    unsigned sid = 0;
    
    if (!data->m_sids.empty()) {
        for (typeSidsItrConst itr = data->m_sids.begin();
            itr != data->m_sids.end(); ++itr) {
            sid = itr->first;
            
            ref = MsgCenter::refNodeMsg(msg);
            _forward(data, sid, ref);
        } 
    }
}

int CliCenter::procPeerUsrExit(RouterCliData* data, 
    int, NodeMsg* msg) { 
    MsgUsrExitBroadcast* body = NULL;
    unsigned peerUid = 0;
    int ret = 0; 

    if (m_center->hasAuthed(data->m_login)) { 
        body = MsgCenter::getBody<MsgUsrExitBroadcast>(msg);
        peerUid = body->m_uid;

        if (peerUid != data->m_login.m_uid1) {
            notifyAllSid(data, msg);
        }
    }
    
    return ret;
}

int CliCenter::procSendNoerr(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 
        
    outMsg = MsgCenter::refNodeMsg(msg);
    
    /* here donot fail */
    _sendMsg(data, outMsg);
    
    return ret;
}

int CliCenter::procForwardNoerr(RouterCliData* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    unsigned dsid = 0;
    int ret = 0; 

    if (m_center->hasAuthed(data->m_login)) { 
        outMsg = MsgCenter::refNodeMsg(msg);
        
        dsid = MsgCenter::getDsid(outMsg);

        /* here donot fail */
        _forward(data, dsid, outMsg);
    }
    
    return ret;
}

/* uid cli send msg to server */
int CliCenter::_sendMsg(RouterCliData* data, 
    NodeMsg* msg) {
    int fd = data->m_self_fd;
    int ret = 0;

    MsgCenter::setDirection(msg, ENUM_MSG_NORTH);
        
    ret = m_frame->sendMsg(fd, msg);
    if (0 != ret) {
        LOG_DEBUG("cli_send| fd=%d| ret=%d| msg=error|", fd, ret);
    }

    return ret;
}

/* sid to uid msgs to deal */
int CliCenter::collect(unsigned uid, NodeMsg* msg) {
    RouterCliData* data = NULL;
    int fd = 0;
    int ret = 0;

    if (validUsr(uid)) {
        data = findUsr(uid);
        fd = data->m_self_fd;
        
        MsgCenter::setDirection(msg, ENUM_MSG_NORTH);
        
        ret = m_frame->dispatch(fd, msg); 
        if (0 != ret) {
            LOG_DEBUG("cli_collect| uid=%u|"
                " fd=%d| ret=%d| msg=dispatch error|", uid, fd, ret);
        }
    } else {
        LOG_DEBUG("cli_collect| uid=%u| msg=invalid usr|", uid);
    
        /* delete msg if failed */
        MsgCenter::freeMsg(msg);
        ret = -1;
    }

    return ret;
}

/* uid to sid msgs to deal */
int CliCenter::_forward(RouterCliData* data, 
    unsigned sid, NodeMsg* msg) {
    int ret = 0;
    int fd = 0; 
  
    if (0 < sid && _existSid(data, sid)) { 
        fd = _findSid(data, sid); 

        MsgCenter::setDirection(msg, ENUM_MSG_SOUTH);
        ret = m_frame->dispatch(fd, msg);
        if (0 != ret) {
            LOG_DEBUG("cli_forward| sid=%u| fd=%d|"
                " ret=%d| msg=dispatch error|", sid, fd, ret);
        }
    } else {
        LOG_DEBUG("cli_forward| sid=%u| msg=invalid sid|", sid);
    
        ret = -1;
        MsgCenter::freeMsg(msg);
    } 
    
    return ret;
}

