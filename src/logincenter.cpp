#include<cstdlib>
#include<cstring>
#include"logincenter.h"
#include"msgcenter.h"
#include"misc.h"
#include"cache.h"


LoginCenter::LoginCenter() {
    m_seq = 0;
}

int LoginCenter::init() {
    int ret = 0;

    return ret;
}

void LoginCenter::finish() {
}

unsigned long LoginCenter::next() {
    if (0 != ++m_seq) {
        return m_seq;
    } else {
        return ++m_seq;
    }
}

NodeMsg* LoginCenter::genHeloMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgHelo* body = NULL;
    const unsigned& timestamp = login.m_timestamp;

    msg = MsgCenter::creatMsg<MsgHelo>(ENUM_MSG_HELO);
    
    body = MsgCenter::getBody<MsgHelo>(msg);
    body->m_timestamp = timestamp;

    return msg;
}
    
NodeMsg* LoginCenter::genErrorMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgErrAck* body = NULL;
    const unsigned long& taskid = login.m_taskid;
    const int& reason = login.m_reason;
    
    msg = MsgCenter::creatMsg<MsgErrAck>(ENUM_MSG_ERROR_ACK);
    
    body = MsgCenter::getBody<MsgErrAck>(msg);
    body->m_taskid = taskid;
    body->m_reason = reason;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
} 

NodeMsg* LoginCenter::genLogoutMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgLogout* body = NULL;
    const unsigned long& taskid = login.m_taskid;
    const unsigned& uid1 = login.m_uid1;
    const unsigned& uid2 = login.m_uid2;
    
    msg = MsgCenter::creatMsg<MsgLogout>(ENUM_MSG_LOGOUT);
    
    body = MsgCenter::getBody<MsgLogout>(msg);
    body->m_taskid = taskid;
    body->m_uid1 = uid1;
    body->m_uid2 = uid2;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
}

NodeMsg* LoginCenter::genLoginReqMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgLoginReq* body = NULL;
    const unsigned long& passwd1 = login.m_passwd1;
    const unsigned& timestamp = login.m_timestamp;
    const unsigned& uid1 = login.m_uid1;
    const unsigned& random1 = login.m_random1;
    unsigned long token = 0;

    creatTokenReq(token, passwd1, uid1, random1); 

    msg = MsgCenter::creatMsg<MsgLoginReq>(ENUM_MSG_LOGIN_REQ);
    
    body = MsgCenter::getBody<MsgLoginReq>(msg);
    body->m_timestamp = timestamp;
    body->m_uid1 = uid1;
    body->m_random1 = random1;
    body->m_token = ~token;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
} 

NodeMsg* LoginCenter::genLoginEndMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgLoginEnd* body = NULL;
    const unsigned char* secret = login.m_secret;
    const unsigned long& taskid = login.m_taskid;
    const unsigned& timestamp = login.m_timestamp;
    const unsigned& uid1 = login.m_uid1;
    const unsigned& uid2 = login.m_uid2;
    unsigned long token = 0;

    creatTokenEnd(token, secret, 
        taskid, timestamp, uid1);

    msg = MsgCenter::creatMsg<MsgLoginEnd>(ENUM_MSG_LOGIN_END);
    
    body = MsgCenter::getBody<MsgLoginEnd>(msg);
    body->m_taskid = taskid;
    body->m_uid1 = uid1;
    body->m_uid2 = uid2;
    body->m_token = ~token;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
} 

NodeMsg* LoginCenter::genExchKeyMsg(
    const LoginData& login) {
    NodeMsg* msg = NULL;
    MsgExchKey* body = NULL;
    const unsigned long& taskid = login.m_taskid;
    const unsigned long& passwd1 = login.m_passwd1;
    const unsigned long& passwd2 = login.m_passwd2;
    const unsigned& uid1 = login.m_uid1;
    const unsigned& uid2 = login.m_uid2;
    const unsigned& random1 = login.m_random1;
    const unsigned& random2 = login.m_random2;
    unsigned long token = 0;

    creatTokenExch(token, passwd1, passwd2,
        random1, random2);

    msg = MsgCenter::creatMsg<MsgExchKey>(ENUM_MSG_EXCH_KEY);
    
    body = MsgCenter::getBody<MsgExchKey>(msg);
    body->m_taskid = taskid;
    body->m_uid1 = uid1;
    body->m_uid2 = uid2; 
    body->m_random2 = random2; 
    body->m_token = ~token;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
}

void LoginCenter::creatTokenReq(unsigned long& token,
    const unsigned long& passwd1,
    const unsigned uid1,
    const unsigned random1) {
    token = random1 ^ uid1;
    token = ~(passwd1 + (token << 32)) + 0xFFFFFFFF;
}

void LoginCenter::creatTokenExch(unsigned long& token,
    const unsigned long& passwd1, 
    const unsigned long& passwd2,
    const unsigned random1,
    const unsigned random2) {
    token = (passwd1 + random1 + 0xFFFFFFFF);
    token += ~(passwd2 + random2); 
}

int LoginCenter::_encData(const unsigned char secret[],
    const void* in, int inlen, void* out) {
    const unsigned long* pilot = NULL;
    const unsigned long* pin = NULL;
    unsigned long* pout = NULL;
    unsigned long tmp = 0;
    int cnt = (inlen >> 3);

    pilot = (const unsigned long*)secret;
    pin = (const unsigned long*)in;
    pout = (unsigned long*)out;
    for (int i=0; i<cnt; ++i) {
        *pout = (*pin) ^ *pilot;
        ++pin;
        ++pout;
    }

    cnt = (inlen & 0x7);
    if (0 < cnt) {
        CacheUtil::bcopy(&tmp, pin, cnt);
        tmp ^= *pilot;
        CacheUtil::bcopy(pout, &tmp, cnt);
    }

    return inlen;
}

void LoginCenter::creatTokenEnd(unsigned long& token,
    const unsigned char secret[], 
    const unsigned long& taskid,
    const unsigned timestamp,
    const unsigned uid1) {
    char tmp[8] = {0};

    CacheUtil::bcopy(tmp, &timestamp, 4);
    CacheUtil::bcopy(tmp + 4, &uid1, 4);

    _encData(secret, tmp, 8, &token);
    token ^= taskid;
    _encData(secret, &token, 8, &token);
}

void LoginCenter::creatSecret(unsigned char secret[],
    const unsigned long& passwd1, 
    const unsigned long& passwd2,
    const unsigned random1,
    const unsigned random2,
    const unsigned uid1) {
    unsigned long* pu = NULL;

    pu = (unsigned long*)secret;
    
    *pu = (random1 ^ random2); 
    *pu <<= 16;
    *pu += uid1;
    *pu <<= 16;
    *pu += (passwd1 ^ passwd2); 
}

int LoginCenter::chkLoginReqMsg(
    const LoginData& login,
    const MsgLoginReq* body) {
    const unsigned long& passwd1 = login.m_passwd1;
    const unsigned& timestamp = login.m_timestamp;
    unsigned long token = 0;
    unsigned reqTm = 0;
    unsigned now = 0;

    reqTm = body->m_timestamp;
    now = MiscTool::getTimeSec();
    
    /* check time stamp */
    if (reqTm != timestamp ||
        now - reqTm >= DEF_REQ_TIMEOUT_SEC) { 
        return -1;
    }


    creatTokenReq(token, passwd1, body->m_uid1,
        body->m_random1);
    if (~token != body->m_token) {
        return -1;
    }
    
    return 0;
}

int LoginCenter::chkLoginEndMsg(
    const LoginData& login,
    const MsgLoginEnd* body) {
    unsigned long token = 0;
    
    if (login.m_uid1 != body->m_uid1 ||
        login.m_uid2 != body->m_uid2 ||
        login.m_taskid != body->m_taskid) {
        return -1;
    }

    creatTokenEnd(token, login.m_secret,
        login.m_taskid,
        login.m_timestamp,
        login.m_uid1);

    if (~token != body->m_token) {
        return -1;
    }

    return 0;
}

int LoginCenter::parseLogoutMsg(
    LoginData& login, NodeMsg* msg) {
    MsgLogout* body = NULL;
    int ret = 0;

    body = MsgCenter::getBody<MsgLogout>(msg);
    
    if (login.m_uid1 == body->m_uid1 &&
        login.m_uid2 == body->m_uid2 &&
        login.m_taskid == body->m_taskid) {
        /* ok */
        login.m_stat = ENUM_ROUTER_LOGOUT;
    } else {
        ret = -1;
    }

    return ret;
}

int LoginCenter::parseErrorAck(LoginData& login,
    NodeMsg* msg) {
    MsgErrAck* body = NULL;
    int ret = 0;
    
    body = MsgCenter::getBody<MsgErrAck>(msg);
    if (login.m_taskid == body->m_taskid) {
        
        login.m_reason = body->m_reason; 
        login.m_stat = ENUM_ROUTER_ERROR;
    } else {
        ret = -1;
    }
    
    return ret;
}

int LoginCenter::parseHeloMsg(LoginData& login,
    NodeMsg* msg) {
    MsgHelo* body = NULL;
    int ret = 0; 

    body = MsgCenter::getBody<MsgHelo>(msg); 

    if (ENUM_ROUTER_INIT != login.m_stat) {
        ret = -1;
        login.m_reason = ret;
        return ret;
    }

    login.m_timestamp = body->m_timestamp;
    MiscTool::getRand(&login.m_random1, 
        sizeof(login.m_random1));

    login.m_stat = ENUM_ROUTER_CLI_REQ;
    return ret;
}

int LoginCenter::parseLoginReqMsg(
    LoginData& login, NodeMsg* msg) {
    MsgLoginReq* body = NULL;
    int ret = 0;

    body = MsgCenter::getBody<MsgLoginReq>(msg);
    
    if (ENUM_ROUTER_SVR_HELO != login.m_stat) {
        ret = -1;
        login.m_reason = ret;
        return ret;
    } 
    
    ret = chkLoginReqMsg(login, body);
    if (0 != ret) {
        login.m_reason = ret;
        return ret;
    }

    login.m_uid1 = body->m_uid1;
    login.m_random1 = body->m_random1;
    login.m_taskid = next();
    MiscTool::getRand(&login.m_random2, sizeof(login.m_random2));

    creatSecret(login.m_secret, 
        login.m_passwd1,
        login.m_passwd2,
        login.m_random1,
        login.m_random2,
        login.m_uid1);

    login.m_stat = ENUM_ROUTER_SVR_EXCH_KEY; 
    return ret;
}

int LoginCenter::parseExchKeyMsg(
    LoginData& login, NodeMsg* msg) {
    MsgExchKey* body = NULL;
    unsigned long token = 0;
    int ret = 0;

    body = MsgCenter::getBody<MsgExchKey>(msg); 
   
    if (ENUM_ROUTER_CLI_REQ != login.m_stat) {
        ret = -1;
        login.m_reason = ret;
        return ret;
    }
    
    if (login.m_uid1 != body->m_uid1) {
        ret = -1;
        login.m_reason = ret;
        return ret;
    }

    creatTokenExch(token, 
        login.m_passwd1,
        login.m_passwd2,
        login.m_random1,
        body->m_random2); 
    if (~token != body->m_token) {
        ret = -1;
        login.m_reason = ret;
        return ret;
    }

    login.m_taskid = body->m_taskid;
    login.m_uid2 = body->m_uid2;
    login.m_random2 = body->m_random2;

    creatSecret(login.m_secret, 
        login.m_passwd1,
        login.m_passwd2,
        login.m_random1,
        login.m_random2,
        login.m_uid1);

    login.m_stat = ENUM_ROUTER_AUTH_OK; 
    return ret;
}

int LoginCenter::parseLoginEnd(
    LoginData& login, NodeMsg* msg) {
    MsgLoginEnd* body = NULL;
    int ret = 0;

    body = MsgCenter::getBody<MsgLoginEnd>(msg);

    if (ENUM_ROUTER_SVR_EXCH_KEY == login.m_stat) {
        ret = chkLoginEndMsg(login, body);
        if (0 == ret) { 
            login.m_stat = ENUM_ROUTER_AUTH_OK;
        } else {
            ret = -2;
            login.m_reason = ret;
        }
    } else {
        ret = -1;
        login.m_reason = ret;
    } 
    
    return ret;
} 

int LoginCenter::parseCipherMsg(
    LoginData& login, NodeMsg* msg) {
    MsgCipherTxt* body = NULL;
    int offset = MsgCenter::getCipherOffset();
    int ret = 0; 
    int hsize = 0;
    int msize = 0;

    msize = MsgCenter::getMsgSize(msg);
    hsize = MsgCenter::getHeadSize(msg);
    body = MsgCenter::getBody<MsgCipherTxt>(msg); 

    if (msize != hsize || hsize !=
        (body->m_cipher_len + offset)) {
        LOG_ERROR("parse_cipher| msize=%d|"
            " hsize=%d| clen=%d| offset=%d|"
            " msg=invalid len|",
            msize, hsize, body->m_cipher_len, offset);
        
        ret = -1;
        login.m_reason = ret;
        return ret;
    }

    if (!hasAuthed(login)) {
        LOG_ERROR("parse_cipher| msize=%d|"
            " hsize=%d| clen=%d| offset=%d|"
            " msg=unauthticated|",
            msize, hsize, body->m_cipher_len, offset);
        
        ret = -2;
        login.m_reason = ret;
        return ret;
    } 

    return 0;
}

int LoginCenter::parsePlainMsg(
    LoginData& login, NodeMsg* msg) {
    MsgPlainTxt* body = NULL;
    int offset = MsgCenter::getPlainOffset();
    int ret = 0; 
    int hsize = 0;
    int msize = 0;

    msize = MsgCenter::getMsgSize(msg);
    hsize = MsgCenter::getHeadSize(msg);
    body = MsgCenter::getBody<MsgPlainTxt>(msg); 

    if (msize != hsize || hsize !=
        (body->m_txt_len + offset)) {
        LOG_ERROR("parse_plain| msize=%d|"
            " hsize=%d| txtlen=%d| offset=%d|"
            " msg=invalid len|",
            msize, hsize, body->m_txt_len, offset);
        
        ret = -1;
        login.m_reason = ret;
        return ret;
    }

    if (!hasAuthed(login)) {
        LOG_ERROR("parse_plain| msize=%d|"
            " hsize=%d| txtlen=%d| offset=%d|"
            " msg=unauthticated|",
            msize, hsize, body->m_txt_len, offset);
        
        ret = -1;
        login.m_reason = ret;
        return ret;
    } 

    return 0;
}

NodeMsg* LoginCenter::genUnreachMsg(
    int reason, unsigned from_uid, 
    unsigned from_sid, NodeMsg* origin) {
    NodeMsg* errMsg = NULL;
    MsgUnreach* body = NULL; 
    unsigned seq = 0;
    unsigned suid = 0;
    unsigned ssid = 0;
    unsigned duid = 0;
    unsigned dsid = 0; 

    seq = MsgCenter::getSeq(origin);
    suid = MsgCenter::getSuid(origin);
    ssid = MsgCenter::getSsid(origin);
    duid = MsgCenter::getDuid(origin);
    dsid = MsgCenter::getDsid(origin);
    

    errMsg = MsgCenter::creatMsg<MsgUnreach>(
        ENUM_MSG_UNREACH); 
    body = MsgCenter::getBody<MsgUnreach>(errMsg); 
    body->m_reason = reason;
    body->m_uid = duid;
    body->m_sid = dsid;

    MsgCenter::setSeq(errMsg, seq);
    MsgCenter::setDst(errMsg, suid, ssid);
    MsgCenter::setSrc(errMsg, from_uid, from_sid);
    
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return errMsg;
}

NodeMsg* LoginCenter::genPlainMsg(unsigned link_no,
    const char* txt, int len) {
    NodeMsg* msg = NULL;
    MsgPlainTxt* body = NULL;

    msg = MsgCenter::creatMsg<MsgPlainTxt>(ENUM_MSG_FRAME_PLAIN, len);
    
    body = MsgCenter::getBody<MsgPlainTxt>(msg);
    CacheUtil::bcopy(body->m_txt, txt, len);

    body->m_link_no = link_no;
    body->m_txt_len = len;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return msg;
}

NodeMsg* LoginCenter::genTunnelMsg(unsigned uid,
    unsigned sid, unsigned link_no, 
    int link_type, const char ip[], int port) {
    NodeMsg* msg = NULL;
    MsgTunnel* body = NULL;

    msg = MsgCenter::creatMsg<MsgTunnel>(ENUM_MSG_TUNNEL_REQ);
    
    body = MsgCenter::getBody<MsgTunnel>(msg);
    
    body->m_uid1 = uid;
    body->m_sid1 = sid;
    body->m_link_no = link_no;
    body->m_link_type = link_type;
    body->m_port = port;
    strncpy(body->m_ip, ip, sizeof(body->m_ip)-1);
    
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return msg;
}

NodeMsg* LoginCenter::genTunnelAckMsg(unsigned sid1, 
    unsigned uid2, unsigned sid2, unsigned link_no) {
    NodeMsg* msg = NULL;
    MsgTunnelAck* body = NULL;

    msg = MsgCenter::creatMsg<MsgTunnelAck>(ENUM_MSG_TUNNEL_ACK);
    
    body = MsgCenter::getBody<MsgTunnelAck>(msg);
    
    body->m_sid1 = sid1;
    body->m_uid2 = uid2;
    body->m_sid2 = sid2;
    body->m_link_no = link_no;
    
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return msg;
}

NodeMsg* LoginCenter::genCloseTunnelMsg(
    unsigned uid, unsigned sid, int reason) {
    NodeMsg* msg = NULL;
    MsgCloseTunnel* body = NULL;

    msg = MsgCenter::creatMsg<MsgCloseTunnel>(ENUM_MSG_TUNNEL_CLOSE);
    
    body = MsgCenter::getBody<MsgCloseTunnel>(msg);
    
    body->m_uid = uid;
    body->m_sid = sid;
    body->m_reason = reason;
    
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return msg;
}

NodeMsg* LoginCenter::genUserExitMsg(unsigned uid, int reason) {
    NodeMsg* msg = NULL;
    MsgUsrExitBroadcast* body = NULL;

    msg = MsgCenter::creatMsg<MsgUsrExitBroadcast>(
        ENUM_MSG_UID_EXIT_BROADCAST);
    
    body = MsgCenter::getBody<MsgUsrExitBroadcast>(msg);
    
    body->m_uid = uid;
    body->m_reason = reason;
    
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));
    return msg;
}

NodeMsg* LoginCenter::cipher2PlainMsg(
    const unsigned char secret[], 
    NodeMsg* inMsg) {
    const char* cipher = NULL;
    NodeMsg* msg = NULL;
    MsgCipherTxt* inBody = NULL;
    MsgPlainTxt* body = NULL;
    int max = 0;
    int total = 0;
    int outlen = 0;
    int clen = 0;

    inBody = MsgCenter::getBody<MsgCipherTxt>(inMsg);
    cipher = inBody->m_cipher;
    clen = inBody->m_cipher_len;

    max = getDecLen(secret, clen);

    msg = MsgCenter::creatMsg<MsgPlainTxt>(ENUM_MSG_FRAME_PLAIN, max);

    /* clone src and dst */
    MsgCenter::cloneAddr(msg, inMsg);
    
    body = MsgCenter::getBody<MsgPlainTxt>(msg);
    outlen = decData(secret, cipher, clen, body->m_txt);
    if (outlen != inBody->m_txt_len) {
        MsgCenter::freeMsg(msg);

        return NULL;
    }

    total = MsgCenter::getPlainOffset() + outlen; 
    MsgCenter::setMsgSize(msg, total);
    MsgCenter::setHeadSize(msg, total);
    
    body->m_taskid = inBody->m_taskid; 
    body->m_link_no = inBody->m_link_no; 
    body->m_txt_len = outlen;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
}

NodeMsg* LoginCenter::plain2CipherMsg(
    const unsigned char secret[],
    NodeMsg* inMsg) {
    const char* txt = NULL;
    NodeMsg* msg = NULL;
    MsgPlainTxt* inBody = NULL;
    MsgCipherTxt* body = NULL;
    int max = 0;
    int total = 0;
    int inlen = 0;
    int outlen = 0;

    inBody = MsgCenter::getBody<MsgPlainTxt>(inMsg);
    txt = inBody->m_txt;
    inlen = inBody->m_txt_len;
    
    max = getEncLen(secret, inlen); 
    msg = MsgCenter::creatMsg<MsgCipherTxt>(ENUM_MSG_FRAME_CIPHER, max);

    /* clone src and dst */
    MsgCenter::cloneAddr(msg, inMsg);
    
    body = MsgCenter::getBody<MsgCipherTxt>(msg);
    outlen = encData(secret, txt, inlen, body->m_cipher); 

    total = MsgCenter::getCipherOffset() + outlen; 
    MsgCenter::setMsgSize(msg, total);
    MsgCenter::setHeadSize(msg, total); 

    body->m_taskid = inBody->m_taskid; 
    body->m_link_no = inBody->m_link_no;
    body->m_txt_len = inlen;
    body->m_cipher_len = outlen;
    body->m_mac = MsgCenter::calcMac(body, sizeof(*body));

    return msg;
}

bool LoginCenter::hasAuthed(const LoginData& login) {
    return login.m_stat == ENUM_ROUTER_AUTH_OK;
}

int LoginCenter::getEncLen(const unsigned char[], int inlen) {
    return inlen;
}

int LoginCenter::encData(const unsigned char secret[],
    const void* in, int inlen, void* out) {
    int len = 0;

    len = _encData(secret, in, inlen, out);
    return len;
} 

int LoginCenter::getDecLen(const unsigned char[], int inlen) {
    return inlen;
}

int LoginCenter::decData(const unsigned char secret[],
    const void* in, int inlen, void* out) {
    int len = 0;

    len = _encData(secret, in, inlen, out);
    return len;
} 


