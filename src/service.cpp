#include<cstdlib>
#include<cstring>
#include"service.h"
#include"baseconfig.h"
#include"usercenter.h"
#include"sockframe.h"
#include"misc.h"
#include"msgcenter.h"
#include"logincenter.h"
#include"agentproto.h"
#include"clicenter.h"
#include"routeragent.h"
#include"cache.h"


Service::Service() {
    m_frame = NULL;
    m_center = NULL;
    m_proto = NULL;
    m_svr = NULL;
    m_cli = NULL;
    m_agent = NULL;

    m_rd_timeout = 0;
    m_wr_timeout = 0;
    MiscTool::bzero(m_log_dir, sizeof(m_log_dir));
}

Service::~Service() {
}

int Service::init() {
    int ret = 0; 

    do { 
        ret = ConfLog(m_log_dir);
        if (0 != ret) {
            break;
        }
        
        SockFrame::creat(); 

        m_frame = SockFrame::instance(); 
        m_proto = new AgentProto(m_frame);

        m_center = new LoginCenter;
        ret = m_center->init();
        if (0 != ret) {
            break;
        }

        m_svr = new SvrCenter(m_frame, m_center, m_proto);
        ret = m_svr->init();
        if (0 != ret) {
            break;
        }
        
        m_cli = new CliCenter(m_frame, m_center, m_proto);
        ret = m_cli->init();
        if (0 != ret) {
            break;
        }
        
        m_agent = new AgentCenter(m_frame, m_center, m_proto, m_cli);
        ret = m_agent->init();
        if (0 != ret) {
            break;
        }
    } while (0);

    return ret;
}

int Service::parseInit(const char* conf_file) {
    const GlobalConf* global = NULL;
    int ret = 0;
    BaseConfig conf;

    do {
        ret = conf.parse(conf_file);
        if (0 == ret) {
            global = conf.getGlobal();
            if (!global->m_log_dir.empty()) {
                global->m_log_dir.copy(m_log_dir, 
                    sizeof(m_log_dir)-1);
            }
        } else {
            return ret;
        }

        ret = init();
        if (0 != ret) {
            return ret;
        }

        ret = prepare(conf.getGlobal());
        if (0 != ret) {
            return ret;
        }
    } while (0);

    return ret;
}

void Service::finish() {
    if (NULL != m_agent) {
        m_agent->finish();

        delete m_agent;
        m_agent = NULL;
    }
    
    if (NULL != m_cli) {
        m_cli->finish();

        delete m_cli;
        m_cli = NULL;
    }
    
    if (NULL != m_svr) {
        m_svr->finish();

        delete m_svr;
        m_svr = NULL;
    }
    
    if (NULL != m_center) {
        m_center->finish();

        delete m_center;
        m_center = NULL;
    }

    if (NULL != m_proto) {
        delete m_proto;
        m_proto = NULL;
    }

    if (NULL != m_frame) {
        SockFrame::destroy(m_frame);
        m_frame = NULL;
    }
}

int Service::prepare(const GlobalConf* conf) {
    int ret = 0;

    m_rd_timeout = conf->m_rd_timeout;
    m_wr_timeout = conf->m_wr_timeout;
    
    if (!conf->m_servers.empty()) {
        for (int i=0; 0 == ret && i<(int)conf->m_servers.size(); ++i) {
            const ServerInfo& info = conf->m_servers[i];
            
            ret = addSvr(info.m_addr.m_ip.c_str(), 
                info.m_addr.m_port, info.m_uid);
        }
    }

    if (!conf->m_clients.empty()) {
        for (int i=0; 0 == ret && i<(int)conf->m_clients.size(); ++i) {
            const ClientInfo& info = conf->m_clients[i];

            addCli(info.m_addr.m_ip.c_str(), 
                info.m_addr.m_port, info.m_uid);
        }
    }

    if (!conf->m_agents.empty()) {
        for (int i=0; 0 == ret && i<(int)conf->m_agents.size(); ++i) {
            const AgentInfo& info = conf->m_agents[i];

            addAgentListener(info.m_addr.m_ip.c_str(), 
                info.m_addr.m_port, 
                info.m_uid, info.m_link_type);
        }
    }

    if (!conf->m_links.empty()) {
        for (int i=0; 0 == ret && i<(int)conf->m_links.size(); ++i) {
            const LinkInfo& info = conf->m_links[i];

            addLink(info.m_addr.m_ip.c_str(), 
                info.m_addr.m_port, 
                info.m_uid, info.m_link_type);
        }
    }

    return ret;
}

int Service::addSvr(const char ip[], int port, unsigned uid) {
    return m_svr->addListener(ip, port, uid);
}

int Service::addCli(const char ip[], int port, unsigned uid) {
    return m_cli->addCli(ip, port, uid);
}

int Service::addAgentListener(const char ip[], int port, 
    unsigned uid, int link_type) {
    return m_agent->addListener(ip, port, uid, link_type);
}

int Service::addLink(const char ip[], int port, 
    unsigned uid, int link_type) {
    return m_agent->addLink(ip, port, uid, link_type);
}

void Service::start() { 
    m_frame->setTimeout(m_rd_timeout, m_wr_timeout);
    
    m_svr->start();
    m_cli->start();
    m_agent->start();
    m_frame->start(); 
}

void Service::wait() {
    m_frame->wait();
}

void Service::stop() {
    m_frame->stop();
}

