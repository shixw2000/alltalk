#include<cstring>
#include<cstdlib>
#include"routeragent.h" 
#include"sockframe.h"
#include"agentdata.h"
#include"misc.h"
#include"logincenter.h"
#include"agentproto.h"
#include"msgcenter.h"
#include"clicenter.h"
#include"cache.h"


AgentListener::AgentListener(AgentCenter* agent) {
    m_agent_db = agent;
}

int AgentListener::onNewSock(int parentId,
    int childFd, AccptOption& opt) {
    int ret = 0;

    ret = m_agent_db->creatAgentSvr(parentId, childFd, opt);
    
    return ret;
}

void AgentListener::onListenerClose(int) {
}

void AgentListener::onClose(int hd) {
    m_agent_db->onClose(hd);
} 

int AgentListener::parseData(int fd, const char* buf, 
    int size, const SockAddr*) {
    int ret = 0;

    ret = m_agent_db->parseData(fd, buf, size);
    return ret;
}

int AgentListener::process(int hd, NodeMsg* msg) {
    int ret = 0;

    ret = m_agent_db->process(hd, msg);
    
    return ret;
}

AgentCli::AgentCli(AgentCenter* agent) {
    m_agent_db = agent;
}

void AgentCli::onClose(int hd) { 
    m_agent_db->onClose(hd);
}

void AgentCli::onConnFail(long extra, int) {
    AgentBase* data = (AgentBase*)extra;

    m_agent_db->onConnFail(data); 
}

int AgentCli::onConnOK(int hd, ConnOption&) {
    int ret = 0;

    ret = m_agent_db->onConnOK(hd); 
    return ret;
}

int AgentCli::parseData(int fd, const char* buf, 
    int size, const SockAddr* ) {
    int ret = 0;

    ret = m_agent_db->parseData(fd, buf, size);
    return ret;
}

int AgentCli::process(int hd, NodeMsg* msg) {
    int ret = 0;

    ret = m_agent_db->process(hd, msg); 
    return ret;
}

const AgentCenter::Worker AgentCenter::m_north_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_PLAIN, &AgentCenter::procForward},
    
    {0, NULL}
};

const AgentCenter::Worker AgentCenter::m_south_works[ENUM_MSG_CODE_END] = {
    {ENUM_MSG_FRAME_PLAIN, &AgentCenter::procSend},

    {ENUM_MSG_UID_EXIT_BROADCAST, &AgentCenter::procPeerUsrExit},
    {ENUM_MSG_TUNNEL_ACK, &AgentCenter::procTunnelAck},
    {ENUM_MSG_UNREACH, &AgentCenter::procUnreach},
    {ENUM_MSG_TUNNEL_CLOSE, &AgentCenter::procTunnelClose},
    
    {0, NULL}
};

AgentCenter::AgentCenter(SockFrame* frame, 
    LoginCenter* center, AgentProto* proto,
    CliCenter* cli)
    : m_frame(frame), m_center(center),
    m_proto(proto), m_cli(cli) {

    m_agent_cli = NULL;
    m_agent_svr = NULL;
}

AgentCenter::~AgentCenter() {
}

int AgentCenter::init() {
    int ret = 0;

    m_agent_cli = new AgentCli(this);
    m_agent_svr = new AgentListener(this);

    m_cli->setAgent(this);

    return ret;
}

void AgentCenter::finish() {
    AgentConf* conf = NULL;
    LinkAddr* addr = NULL;

    if (NULL != m_agent_cli) {
        delete m_agent_cli;
        m_agent_cli = NULL;
    }

    if (NULL != m_agent_svr) {
        delete m_agent_svr;
        m_agent_svr = NULL;
    }
    
    if (!m_links.empty()) {
        for (typeLinkItr itr=m_links.begin();
            itr!=m_links.end(); ++itr) {
            addr = itr->second;

            freeLink(addr);
        }

        m_links.clear();
    }

    if (!m_confs.empty()) {
        for (int i=0; i<(int)m_confs.size(); ++i) {
            conf = m_confs[i];

            freeConf(conf);
        }

        m_confs.clear();
    }
}

void AgentCenter::start() {
    AgentConf* conf = NULL;
    
    for (int i=0; i<(int)m_confs.size(); ++i) {
        conf = m_confs[i];
        
        m_frame->creatSvr(conf->m_ip, conf->m_port, 
            m_agent_svr, (long)conf);
    }
}

int AgentCenter::addListener(const char ip[], 
    int port, unsigned uid, int link_type) {
    AgentConf* conf = NULL;
    int ret = 0;
    
    if (m_cli->hasUsr(uid)) {
        conf = allocConf();
        if (NULL != conf) {
            conf->m_port = port;
            conf->m_uid = uid;
            conf->m_link_type = link_type;
            strncpy(conf->m_ip, ip, sizeof(conf->m_ip)-1); 

            m_confs.push_back(conf);
        } else {
            ret = -2;
        }
    } else {
        ret = -1;
    }

    return ret;
}

int AgentCenter::addLink(const char ip[], 
    int port, unsigned uid, int link_type) {
    int ret = 0;
    
    if (!existLink(link_type)) {
        LinkAddr* addr = (LinkAddr*)CacheUtil::mallocAlign(sizeof(LinkAddr));

        MiscTool::bzero(addr, sizeof(LinkAddr));
        addr->m_uid = uid;
        addr->m_link_type = link_type;
        addr->m_port = port;
        strncpy(addr->m_ip, ip, sizeof(addr->m_ip)-1);

        m_links[link_type] = addr;
    } else {
        ret = -1;
    }

    return ret;
}

void AgentCenter::freeLink(LinkAddr* addr) {
    CacheUtil::freeAlign(addr);
}

bool AgentCenter::existLink(int link_type) {
    typeLinkItrConst itr = m_links.find(link_type);

    if (m_links.end() != itr) {
        return true;
    } else {
        return false;
    }
}

const LinkAddr* AgentCenter::findLink(int link_type) {
    typeLinkItrConst itr = m_links.find(link_type);

    if (m_links.end() != itr) {
        return itr->second;
    } else {
        return NULL;
    }
}

int AgentCenter::creatAgentSvr(int parentId, 
    int newFd, AccptOption& opt) {
    const LinkAddr* addr = NULL;
    AgentConf* conf = NULL;
    AgentBase* data = NULL;
    unsigned sid = 0;
    unsigned uid = 0;
    int link_type = 0;
    int ret = 0;

    conf = (AgentConf*)m_frame->getExtra(parentId);
    link_type = conf->m_link_type;
    uid = conf->m_uid;
    
    if (existLink(link_type)) {
        addr = findLink(link_type);
    } else {
        return -1;
    }
 
    data = allocAgent(addr->m_ip, addr->m_port,
        addr->m_link_type); 
    if (NULL != data) { 
        sid = m_cli->regSid(uid, newFd);
        if (0 < sid) {
            data->m_self_fd = newFd;
            data->m_cli_uid = uid;
            data->m_session = sid; 

            /* peer session is 0 for unknown */
            data->m_peer_uid = addr->m_uid; 

            /* here link_no used as authtication */
            data->m_link_no = data->m_cli_uid + 
                data->m_peer_uid;
            
            data->m_stat = ENUM_ROUTER_INIT;
            data->m_peer_stat = ENUM_ROUTER_INVALID; 

            sendTunnelMsg(data, addr);

            opt.m_extra = (long)data;
            opt.m_delay = true;
            ret = 0;
        } else {
            ret = -1;
            freeAgent(data);
        } 
    } else {
        ret = -1;
    }
    
    return ret;
}

int AgentCenter::creatAgentCli(int parentId, NodeMsg* msg) {
    RouterCliData* cli = NULL;
    MsgTunnel* body = NULL;
    AgentBase* data = NULL;
    int ret = 0;
    unsigned uid = 0;

    cli = (RouterCliData*)m_frame->getExtra(parentId);
    body = MsgCenter::getBody<MsgTunnel>(msg);

    LOG_DEBUG("creat_agent_cli| stat=%d| uid1=%u|"
        " sid1=%u| link_no=%u| link_type=%d| addr=%s:%d|",
        cli->m_login.m_stat,
        body->m_uid1, body->m_sid1,
        body->m_link_no, body->m_link_type, 
        body->m_ip, body->m_port);

    do {
        uid = cli->m_login.m_uid1;
        
        ret = chkTunnelReq(cli, body);
        if (0 != ret) {
            ret = ENUM_ERR_TUNNEL_PARAM;
            break;
        } 
 
        data = allocAgent(body->m_ip, body->m_port,
            body->m_link_type); 
        if (NULL == data) {
            ret = ENUM_ERR_TUNNEL_RESOURCE;
            break;
        }
        
        data->m_cli_uid = uid;
        
        /* get the peer info */
        data->m_peer_uid = body->m_uid1;
        data->m_peer_session = body->m_sid1; 

        data->m_link_no = body->m_link_no;

        data->m_stat = ENUM_ROUTER_INVALID;
        data->m_peer_stat = ENUM_ROUTER_AUTH_OK;

        m_frame->creatCli(data->m_ip, data->m_port, 
            m_agent_cli, (long)data);

        return 0;
    } while (0);

    m_cli->rspSidUnreach(ret, cli, msg);
    return ret;
}

AgentConf* AgentCenter::allocConf() {
    AgentConf* conf = NULL;

    conf = (AgentConf*)CacheUtil::mallocAlign(sizeof(AgentConf));
    MiscTool::bzero(conf, sizeof(AgentConf));
    
    return conf;
}

void AgentCenter::freeConf(AgentConf* conf) {
    CacheUtil::freeAlign(conf);
}

AgentBase* AgentCenter::allocAgent(const char ip[],
    int port, int link_type) {
    AgentBase* data = NULL;
 
    data = (AgentBase*)CacheUtil::mallocAlign(sizeof(AgentBase));
    if (NULL != data) {
        MiscTool::bzero(data, sizeof(*data));
        
        data->m_timestamp = MiscTool::getTimeSec();

        data->m_link_type = link_type;
        data->m_port = port;
        strncpy(data->m_ip, ip, sizeof(data->m_ip)-1); 
    }
    
    return data;
}

void AgentCenter::freeAgent(AgentBase* data) {
    CacheUtil::freeAlign(data);
}

void AgentCenter::onClose(int hd) {
    AgentBase* data = NULL;

    data = (AgentBase*)m_frame->getExtra(hd);

    sendCloseTunnelMsg(data); 
    m_cli->unregSid(data->m_cli_uid, data->m_session);
    freeAgent(data);
}

void AgentCenter::onConnFail(AgentBase* data) {
    data->m_reason = -1;
    sendCloseTunnelMsg(data); 
    freeAgent(data);
}

int AgentCenter::onConnOK(int hd) {
    AgentBase* data = NULL;
    int ret = 0;
    unsigned sid = 0;

    data = (AgentBase*)m_frame->getExtra(hd);
    
    sid = m_cli->regSid(data->m_cli_uid, hd);
    if (0 < sid) { 
        data->m_reason = 0;
        data->m_self_fd = hd;
        data->m_session = sid;
        data->m_stat = ENUM_ROUTER_AUTH_OK;

        sendTunnelAckMsg(data);
    } else {
        ret = -1;
        data->m_reason = ret;
        sendCloseTunnelMsg(data); 
        freeAgent(data);
    }
    
    return ret;
}

void AgentCenter::closeAgent(AgentBase* data) {
    if (0 < data->m_self_fd) {
        m_frame->closeData(data->m_self_fd);
    }
}

int AgentCenter::parseData(int fd, const char* buf, int len) {
    AgentBase* data = NULL;
    NodeMsg* pMsg = NULL;
    int ret = 0;

    data = (AgentBase*)m_frame->getExtra(fd);
    pMsg = m_center->genPlainMsg(data->m_link_no, buf, len);

    /* dispatch a completed msg */
    MsgCenter::setDirection(pMsg, ENUM_MSG_NORTH);
    ret = m_frame->dispatch(fd, pMsg); 
    if (0 != ret) {
        LOG_ERROR("parse_data| ret=%|", ret);
    }
    
    return ret;
}

int AgentCenter::process(int hd, NodeMsg* msg) {
    int ret = 0;
    int enDir = 0;

    ret = MsgCenter::chkHeadMsg(msg);
    if (0 == ret) {
        enDir = MsgCenter::getDirection(msg); 
        switch (enDir) {
        case ENUM_MSG_NORTH:
            ret = procNorth(hd, msg);
            break;

        case ENUM_MSG_SOUTH:
            ret = procSouth(hd, msg);
            break;

        default:
            LOG_ERROR("invalid direction agent|");
            ret = -1;
            break;
        } 
    }

    if (0 != ret) {
        MsgCenter::dspMsgHead("proc_agent", msg);
    }
        
    return ret;
}

int AgentCenter::procNorth(int hd, NodeMsg* msg) {
    AgentBase* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (AgentBase*)m_frame->getExtra(hd); 

    LOG_DEBUG("proc_agent_north_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_stat);
 
    for (pHd = m_north_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            ret = (this->*(pHd->m_fn))(data, hd, msg);
            if (0 != ret) {
                LOG_ERROR("proc_agent_north_msg| ret=%d|"
                    " cmd=0x%x| stat=%d|" 
                    " msg=deal error|",
                    ret, cmd, data->m_stat);
            }
            break;
        }
    } 

    return ret;
}

int AgentCenter::procSouth(int hd, NodeMsg* msg) {
    AgentBase* data = NULL;
    const Worker* pHd = NULL;
    int ret = 0;
    unsigned cmd = 0;

    cmd = MsgCenter::getCmd(msg); 
    data = (AgentBase*)m_frame->getExtra(hd); 

    LOG_DEBUG("proc_agent_south_msg| cmd=0x%x| stat=%d|", 
        cmd, data->m_stat); 
    
    for (pHd = m_south_works; NULL != pHd->m_fn; ++pHd) {
        if (cmd == pHd->m_cmd) { 
            ret = (this->*(pHd->m_fn))(data, hd, msg);
            if (0 != ret) {
                LOG_ERROR("proc_agent_south_msg| ret=%d|"
                    " cmd=0x%x| stat=%d|" 
                    " msg=deal error|",
                    ret, cmd, data->m_stat);
            }
            break;
        }
    } 

    return ret;
}

int AgentCenter::chkTunnelReq(RouterCliData* cli,
    const MsgTunnel* body) {
    int ret = 0;

    if (0 >= body->m_link_type ||
        0 == body->m_uid1 ||
        0 == body->m_sid1 ||
        0 == body->m_link_no ||
        0 >= body->m_port ||
        0xffff <= body->m_port) {
        return -1;
    }

    /* here check link_no */
    if (body->m_link_no != cli->m_login.m_uid1 + body->m_uid1) {
        return -1;
    }

    return ret;
}

int AgentCenter::chkTunnelAck(AgentBase* data,
    MsgTunnelAck* body) {
    int ret = 0;

    if (0 == body->m_link_no ||
        0 == body->m_uid2 ||
        0 == body->m_sid2) {
        return -1;
    }

    if (ENUM_ROUTER_INIT == data->m_stat
        && body->m_sid1 == data->m_session
        && body->m_uid2 == data->m_peer_uid
        && body->m_link_no == data->m_link_no) {
        return ret;
    } else {
        return -1;
    }
}

int AgentCenter::procTunnelAck(AgentBase* data, 
    int, NodeMsg* msg) { 
    MsgTunnelAck* body = NULL;
    int ret = 0;

    body = MsgCenter::getBody<MsgTunnelAck>(msg);

    LOG_DEBUG("proc_tunnel_ack| stat=%d|"
        " my_sid=%u| my_link_no=%u| sid1=%u|"
        " uid2=%u| sid2=%u| link_no=%u|",
        data->m_stat, data->m_session,
        data->m_link_no, body->m_sid1,
        body->m_uid2, body->m_sid2,
        body->m_link_no);

    ret = chkTunnelAck(data, body);
    if (0 == ret) {
        data->m_peer_session = body->m_sid2;
            
        data->m_stat = ENUM_ROUTER_AUTH_OK;
        data->m_peer_stat = ENUM_ROUTER_AUTH_OK;

        /* here we start to read data */
        m_frame->undelayRead(data->m_self_fd);
    }
    
    return ret;
}

int AgentCenter::procPeerUsrExit(AgentBase* data, 
    int, NodeMsg* msg) { 
    MsgUsrExitBroadcast* body = NULL;
    unsigned peerUid = 0;
    int ret = 0; 
    int reason = 0;

    body = MsgCenter::getBody<MsgUsrExitBroadcast>(msg);
    peerUid = body->m_uid;
    reason = body->m_reason; 

    if (peerUid == data->m_peer_uid) {
        (void)reason;
        LOG_DEBUG("proc_user_exit| stat=%d|"
            " my_sid=%u| peer_uid=%u| reason=%d|",
            data->m_stat, data->m_session, 
            peerUid, reason);
        
        _invalidPeer(data);
        closeAgent(data);
    }
    
    return ret;
}

int AgentCenter::sendTunnelMsg(AgentBase* data,
    const LinkAddr* addr) {
    NodeMsg* msg = NULL;
    int ret = 0;
    
    msg = m_center->genTunnelMsg(data->m_cli_uid, 
        data->m_session, data->m_link_no, 
        data->m_link_type, 
        addr->m_ip, addr->m_port);

    ret = _forward(data, msg); 
    
    return ret;    
}

int AgentCenter::sendCloseTunnelMsg(AgentBase* data) {
    NodeMsg* msg = NULL;
    int ret = 0;

    if (ENUM_ROUTER_AUTH_OK == data->m_peer_stat) {
        _invalidPeer(data);

        msg = m_center->genCloseTunnelMsg(
            data->m_peer_uid, data->m_peer_session,
            data->m_reason);

        ret = _forward(data, msg); 
    }
    
    return ret;    
}

int AgentCenter::sendTunnelAckMsg(AgentBase* data) {
    NodeMsg* msg = NULL;
    unsigned uid2 = 0;
    unsigned sid2 = 0;
    unsigned sid1 = 0;
    unsigned link_no = 0;
    int ret = 0;

    sid1 = data->m_peer_session;
    uid2 = data->m_cli_uid;
    sid2 = data->m_session;
    link_no = data->m_link_no;

    msg = m_center->genTunnelAckMsg(sid1, uid2, sid2, link_no);
    ret = _forward(data, msg);
    
    return ret;    
}

int AgentCenter::procTunnelClose(AgentBase* data, 
    int, NodeMsg* msg) { 
    MsgCloseTunnel* body = NULL;
    unsigned uid = 0;
    unsigned sid = 0;
    int reason = 0;
    int ret = 0;

    body = MsgCenter::getBody<MsgCloseTunnel>(msg);
    uid = body->m_uid;
    sid = body->m_sid;
    reason = body->m_reason;

    (void)reason;
    LOG_DEBUG("proc_tunnel_close| stat=%d|"
        " my_sid=%u| sid=%u| reason=%d|",
        data->m_stat, data->m_session, 
        sid, reason);

    if (uid == data->m_cli_uid &&
        sid == data->m_session) {
        _invalidPeer(data);
        closeAgent(data);
    }
    
    return ret;
}

int AgentCenter::procUnreach(AgentBase* data, 
    int, NodeMsg* msg) { 
    MsgUnreach* body = NULL;
    int ret = 0; 

    body = MsgCenter::getBody<MsgUnreach>(msg); 
    if (body->m_uid == data->m_peer_uid &&
        (body->m_sid == data->m_peer_session)) {
        _invalidPeer(data);
        closeAgent(data);
    } 
    
    return ret;
}

int AgentCenter::procSend(AgentBase* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 
    int offset = 0;
        
    outMsg = MsgCenter::refNodeMsg(msg);

    offset = MsgCenter::getPlainOffset();
    MsgCenter::skipMsgPos(outMsg, offset); 
    
    ret = _sendMsg(data, outMsg); 
    return ret;
}
  
int AgentCenter::procForward(AgentBase* data, 
    int, NodeMsg* msg) { 
    NodeMsg* outMsg = NULL;
    int ret = 0; 

    if (ENUM_ROUTER_AUTH_OK == data->m_stat) {
        outMsg = MsgCenter::refNodeMsg(msg);
        ret = _forward(data, outMsg);
    } else {
        ret = -1;
    }
    
    return ret;
}

void AgentCenter::_invalidPeer(AgentBase* data) {
    data->m_peer_stat = ENUM_ROUTER_CLOSING;
}

int AgentCenter::_forward(AgentBase* data, NodeMsg* msg) {
    int ret = 0;

    MsgCenter::setDirection(msg, ENUM_MSG_NORTH); 
    MsgCenter::setSrc(msg, data->m_cli_uid, data->m_session);
    MsgCenter::setDst(msg, data->m_peer_uid, data->m_peer_session);
    
    ret = m_cli->collect(data->m_cli_uid, msg); 
    if (0 != ret) {
        /* if cannot link with cli, then peer is broken down */
        _invalidPeer(data);
    }
    return ret;
}

int AgentCenter::_sendMsg(AgentBase* data, NodeMsg* msg) {
    int fd = data->m_self_fd;
    int ret = 0;

    MsgCenter::setDirection(msg, ENUM_MSG_SOUTH);
    
    ret = m_frame->sendMsg(fd, msg);
    if (0 != ret) {
        LOG_ERROR("agent_send| fd=%d| ret=%d| msg=error|", fd, ret);
    }
    
    return ret;
}

