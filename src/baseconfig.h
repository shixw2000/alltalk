#ifndef __BASECONFIG_H__
#define __BASECONFIG_H__
#include"config.h"


struct ServerInfo {
    AddrInfo m_addr;
    unsigned m_uid;
};

struct ClientInfo {
    AddrInfo m_addr;
    unsigned m_uid;
};

struct AgentInfo {
    AddrInfo m_addr;
    unsigned m_uid;
    int m_link_type;
};

struct LinkInfo {
    AddrInfo m_addr;
    unsigned m_uid;
    int m_link_type;
};

struct GlobalConf {
    unsigned m_rd_thresh; 
    unsigned m_wr_thresh;
    unsigned m_rd_timeout;
    unsigned m_wr_timeout;
    int m_log_level;
    int m_log_stdin;
    int m_log_size;
    typeStr m_log_dir;
    std::vector<ServerInfo> m_servers;
    std::vector<ClientInfo> m_clients;
    std::vector<AgentInfo> m_agents;
    std::vector<LinkInfo> m_links;
};

static const char SEC_LOG[] = "log";
static const char SEC_SERVER[] = "server";
static const char SEC_ROUTER[] = "router";
static const char SEC_CLIENT[] = "client";
static const char SEC_AGENT[] = "agent";
static const char SEC_LINK[] = "link";
static const char KEY_LISTEN_ADDR[] = "listen_addr";
static const char KEY_CONN_ADDR[] = "conn_addr";
static const char KEY_AGENT_ADDR[] = "agent_addr";
static const char KEY_LINK_ADDR[] = "link_addr";
static const char KEY_UID[] = "uid";
static const char KEY_LINK_TYPE[] = "link_type";

class BaseConfig { 
public: 
    int parse(const char* file);
    
    const GlobalConf* getGlobal() const;

    int parseConf(); 

    void clear();
    void dspConf();
    void dspServer(const ServerInfo& info);
    void dspClient(const ClientInfo& info);
    void dspAgent(const AgentInfo& info);
    void dspLink(const LinkInfo& info);

private:
    int parseServer(ServerInfo& info, const typeStr& sec);
    int parseClient(ClientInfo& info, const typeStr& sec);
    int parseAgent(AgentInfo& info, const typeStr& sec);
    int parseLink(LinkInfo& info, const typeStr& sec);

    int parseLog();
    int parseParam();

private:
    Config m_parser;
    GlobalConf m_conf;
};

#endif

