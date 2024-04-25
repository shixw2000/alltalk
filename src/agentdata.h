#ifndef __AGENTDATA_H__
#define __AGENTDATA_H__
#include<vector>
#include<map>
#include"shareheader.h"
#include"isockmsg.h"


static const int DEF_SECRET_SIZE = 8;
static const int MAX_SVR_DATA_CAP = 1000;
static const int MAX_CLI_DATA_CAP = 1000;
static const int MAX_AGENT_DATA_CAP = 1000;
static const int MAX_ASSET_DATA_CAP = 1000;
static const unsigned DEF_REQ_TIMEOUT_SEC = 10;


typedef std::map<unsigned, int> typeSids;
typedef typeSids::iterator typeSidsItr;
typedef typeSids::const_iterator typeSidsItrConst;

enum EnumMsgDir {
    ENUM_MSG_NORTH = 1,
    ENUM_MSG_SOUTH = 2    
};

enum EnumRouterStat {
    ENUM_ROUTER_INVALID = 0,
    ENUM_ROUTER_INIT,
    ENUM_ROUTER_CLI_REQ,
    ENUM_ROUTER_SVR_HELO,
    ENUM_ROUTER_SVR_EXCH_KEY,
    
    ENUM_ROUTER_AUTH_OK,

    ENUM_ROUTER_LOGOUT,
    
    ENUM_ROUTER_CLOSING,
    ENUM_ROUTER_TIMEOUT,
    ENUM_ROUTER_ERROR
};

struct LoginData {
    unsigned long m_passwd1;
    unsigned long m_passwd2;
    unsigned long m_taskid;
    unsigned m_timestamp; 
    unsigned m_uid1;
    unsigned m_uid2;
    unsigned m_random1;
    unsigned m_random2;
    int m_stat;
    int m_reason;
    unsigned char m_secret[DEF_SECRET_SIZE];
};

struct RouterSvrConf {
    unsigned long m_passwd1;
    unsigned long m_passwd2;
    unsigned m_uid2;
    int m_port;
    char m_ip[DEF_IP_SIZE];
};

struct RouterSvrData {
    RouterSvrConf* m_conf;
    LoginData m_login;
    int m_self_fd;
    SockBuffer m_cache;
};

struct RouterCliConf {
    unsigned long m_passwd1;
    unsigned long m_passwd2;
    unsigned m_uid1;
    int m_port;
    char m_ip[DEF_IP_SIZE];
};

struct RouterCliData {
    RouterCliConf* m_conf;
    LoginData m_login; 
    int m_self_fd; 
    int m_conn_delay;
    SockBuffer m_cache;
    typeSids m_sids;
};

struct LinkAddr {
    unsigned m_uid;
    int m_link_type;
    int m_port;
    char m_ip[DEF_IP_SIZE];
};

struct AgentConf {
    unsigned m_uid;
    int m_link_type;
    int m_port;
    char m_ip[DEF_IP_SIZE];
}; 
    
struct AgentBase {
    unsigned m_timestamp; 
    unsigned m_cli_uid;
    unsigned m_session;
    unsigned m_peer_uid;
    unsigned m_peer_session;
    unsigned m_link_no;
    int m_link_type;
    int m_stat;
    int m_peer_stat;
    int m_reason;
    int m_self_fd;
    int m_port;
    char m_ip[DEF_IP_SIZE];
};

#endif

