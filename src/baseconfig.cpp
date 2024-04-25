#include<cstring>
#include<cstdlib>
#include<cstdio>
#include"baseconfig.h"
#include"shareheader.h"


using namespace std;

int BaseConfig::parse(const char* file) {
    int ret = 0;

    do {
        clear();
        
        ret = m_parser.parseFile(file);
        if (0 != ret) {
            break;
        }

        ret = parseLog();
        if (0 != ret) {
            break;
        }
    
        ret = parseParam();
        if (0 != ret) {
            break;
        }

        ret = parseConf();
        if (0 != ret) {
            break;
        } 

        dspConf();
    } while (0);

    return ret;
}

const GlobalConf* BaseConfig::getGlobal() const {
    return &m_conf;
}

int BaseConfig::parseServer(ServerInfo& info, const typeStr& sec) {
    int ret = 0;
    int num = 0;
    typeStr token; 

    ret = m_parser.getNum(sec, KEY_UID, num);
    CHK_RET(ret);

    info.m_uid = num;

    ret = m_parser.getToken(sec, KEY_LISTEN_ADDR, token);
    CHK_RET(ret);

    ret = m_parser.getAddr(info.m_addr, token);
    CHK_RET(ret);
    
    return ret;
}

int BaseConfig::parseClient(ClientInfo& info, const typeStr& sec) {
    int ret = 0;
    int num = 0;
    typeStr token; 

    ret = m_parser.getNum(sec, KEY_UID, num);
    CHK_RET(ret);

    info.m_uid = num;

    ret = m_parser.getToken(sec, KEY_CONN_ADDR, token);
    CHK_RET(ret);

    ret = m_parser.getAddr(info.m_addr, token);
    CHK_RET(ret);
    
    return ret;
}

int BaseConfig::parseAgent(AgentInfo& info, const typeStr& sec) {
    int ret = 0;
    int num = 0;
    typeStr token; 

    ret = m_parser.getNum(sec, KEY_UID, num);
    CHK_RET(ret);

    info.m_uid = num;

    ret = m_parser.getNum(sec, KEY_LINK_TYPE, num);
    CHK_RET(ret);

    info.m_link_type = num;
    
    ret = m_parser.getToken(sec, KEY_AGENT_ADDR, token);
    CHK_RET(ret);

    ret = m_parser.getAddr(info.m_addr, token);
    CHK_RET(ret);
    
    return ret;
}

int BaseConfig::parseLink(LinkInfo& info, const typeStr& sec) {
    int ret = 0;
    int num = 0;
    typeStr token; 

    ret = m_parser.getNum(sec, KEY_UID, num);
    CHK_RET(ret);

    info.m_uid = num;

    ret = m_parser.getNum(sec, KEY_LINK_TYPE, num);
    CHK_RET(ret);

    info.m_link_type = num;
    
    ret = m_parser.getToken(sec, KEY_LINK_ADDR, token);
    CHK_RET(ret);

    ret = m_parser.getAddr(info.m_addr, token);
    CHK_RET(ret);
    
    return ret;
}


void BaseConfig::clear() {
    m_conf.m_log_level = 2;
    m_conf.m_log_stdin = 1;
    m_conf.m_log_size = 10;
    
    m_conf.m_log_dir.clear();
    
    m_conf.m_rd_thresh = 0;
    m_conf.m_wr_thresh = 0;
    m_conf.m_rd_timeout = 0;
    m_conf.m_wr_timeout = 0;
    
    m_conf.m_servers.clear();
    m_conf.m_clients.clear();
    m_conf.m_agents.clear();
    m_conf.m_links.clear();
}


int BaseConfig::parseLog() {
    int ret = 0;
    int n = 0;
    typeStr logDir;

    ret = m_parser.getToken(SEC_LOG, DEF_KEY_LOG_DIR, logDir);
    if (0 == ret) { 
        ret = ConfLog(logDir.c_str());
    } else {
        ret = ConfLog(NULL);
    }
    
    CHK_RET(ret);

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_LEVEL_NAME, n);
    CHK_RET(ret); 
    m_conf.m_log_level = n;

    SetLogLevel(m_conf.m_log_level);

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_STDIN_NAME, n);
    CHK_RET(ret); 
    m_conf.m_log_stdin = n;

    SetLogScreen(m_conf.m_log_stdin);

    ret = m_parser.getNum(SEC_LOG, DEF_KEY_LOG_FILE_SIZE, n);
    CHK_RET(ret); 
    m_conf.m_log_size = n;

    SetMaxLogSize(m_conf.m_log_size);

    return ret;
}


int BaseConfig::parseParam() {
    int ret = 0;
    int n = 0;

    ret = m_parser.getNum(GLOBAL_SEC, "rd_thresh", n);
    CHK_RET(ret); 
    m_conf.m_rd_thresh = n;

    ret = m_parser.getNum(GLOBAL_SEC, "wr_thresh", n);
    CHK_RET(ret); 
    m_conf.m_wr_thresh = n;

    ret = m_parser.getNum(GLOBAL_SEC, "rd_timeout", n);
    CHK_RET(ret); 
    m_conf.m_rd_timeout = n;

    ret = m_parser.getNum(GLOBAL_SEC, "wr_timeout", n);
    CHK_RET(ret); 
    m_conf.m_wr_timeout = n;

    return ret;
}

int BaseConfig::parseConf() {
    int ret = 0;
    int cnt = 0;
    typeStr key;
    typeStr subkey; 

    ret = m_parser.getItemCnt(GLOBAL_SEC, SEC_ROUTER, cnt);
    CHK_RET(ret);

    for (int i=1; i<=cnt; ++i) {
        ServerInfo info;
        
        subkey = m_parser.getIndexName(SEC_ROUTER, i);
        
        ret = parseServer(info, subkey);
        CHK_RET(ret);

        m_conf.m_servers.push_back(info);
    }

    ret = m_parser.getItemCnt(GLOBAL_SEC, SEC_CLIENT, cnt);
    CHK_RET(ret);

    for (int i=1; i<=cnt; ++i) {
        ClientInfo info;
        
        subkey = m_parser.getIndexName(SEC_CLIENT, i);
        
        ret = parseClient(info, subkey);
        CHK_RET(ret);

        m_conf.m_clients.push_back(info);
    }

    ret = m_parser.getItemCnt(GLOBAL_SEC, SEC_AGENT, cnt);
    CHK_RET(ret);

    for (int i=1; i<=cnt; ++i) {
        AgentInfo info;
        
        subkey = m_parser.getIndexName(SEC_AGENT, i);
        
        ret = parseAgent(info, subkey);
        CHK_RET(ret);

        m_conf.m_agents.push_back(info);
    }

    ret = m_parser.getItemCnt(GLOBAL_SEC, SEC_LINK, cnt);
    CHK_RET(ret);

    for (int i=1; i<=cnt; ++i) {
        LinkInfo info;
        
        subkey = m_parser.getIndexName(SEC_LINK, i);
        
        ret = parseLink(info, subkey);
        CHK_RET(ret);

        m_conf.m_links.push_back(info);
    }

    return ret;
}

void BaseConfig::dspServer(const ServerInfo& info) {
    LOG_INFO("server_info| addr=%s:%d| uid=%u|",
        info.m_addr.m_ip.c_str(), info.m_addr.m_port,
        info.m_uid);
}

void BaseConfig::dspClient(const ClientInfo& info) {
    LOG_INFO("client_info| addr=%s:%d| uid=%u|",
        info.m_addr.m_ip.c_str(), info.m_addr.m_port,
        info.m_uid);
}

void BaseConfig::dspAgent(const AgentInfo& info) {
    LOG_INFO("agent_info| addr=%s:%d| uid=%u| link_type=%d|",
        info.m_addr.m_ip.c_str(), info.m_addr.m_port,
        info.m_uid, info.m_link_type);
}

void BaseConfig::dspLink(const LinkInfo& info) {
    LOG_INFO("link_info| addr=%s:%d| uid=%u| link_type=%d|",
        info.m_addr.m_ip.c_str(), info.m_addr.m_port,
        info.m_uid, info.m_link_type);
}

void BaseConfig::dspConf() {
    if (!m_conf.m_servers.empty()) {
        for (int i=0; i<(int)m_conf.m_servers.size(); ++i) {
            const ServerInfo& info = m_conf.m_servers[i];

            dspServer(info);
        }
    } else {
        LOG_INFO("empty server|");
    }

    if (!m_conf.m_clients.empty()) {
        for (int i=0; i<(int)m_conf.m_clients.size(); ++i) {
            const ClientInfo& info = m_conf.m_clients[i];

            dspClient(info);
        }
    } else {
        LOG_INFO("empty client|");
    }

    if (!m_conf.m_agents.empty()) {
        for (int i=0; i<(int)m_conf.m_agents.size(); ++i) {
            const AgentInfo& info = m_conf.m_agents[i];

            dspAgent(info);
        }
    } else {
        LOG_INFO("empty agent|");
    }

    if (!m_conf.m_links.empty()) {
        for (int i=0; i<(int)m_conf.m_links.size(); ++i) {
            const LinkInfo& info = m_conf.m_links[i];

            dspLink(info);
        }
    } else {
        LOG_INFO("empty link|");
    }
}

