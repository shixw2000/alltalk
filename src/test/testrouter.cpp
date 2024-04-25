#include<cstdlib>
#include"service.h"
#include"shareheader.h"


const int link_type = 300;
const unsigned agent_uid = 1000;

extern void armSigs();

int serviceRouter(const char ip[], int port) {
    int ret = 0;
    unsigned uid = 100;
    Service* svr = NULL;

    LOG_INFO("start service router|");

    svr = new Service;

    do { 
        ret = svr->init();
        if (0 != ret) {
            break;
        }

        svr->addSvr(ip, port, ++uid);

        svr->start();
        svr->wait();
    } while (0);

    svr->finish();
    delete svr;

    LOG_INFO("ret=%d| end service router|", ret);
    
    return ret;
}

int agentRouter(const char ip[], int port,
    const char ip2[], int port2, unsigned uid) {
    int ret = 0;
    Service* cli = NULL;

    LOG_INFO("start agent router|");

    cli = new Service;

    do { 
        ret = cli->init();
        if (0 != ret) {
            break;
        }

        cli->addCli(ip, port, agent_uid);
        cli->addLink(ip2, port2, uid, link_type);
        cli->addAgentListener(ip, port+1, agent_uid, link_type);

        cli->start();
        cli->wait();
    } while (0);

    cli->finish();
    delete cli;

    LOG_INFO("ret=%d| end agent router|", ret);
    
    return ret;
}

int assetAgent(const char ip[], int port, unsigned uid) {
    int ret = 0;
    Service* agent = NULL;

    LOG_INFO("start asset agent|");

    agent = new Service;

    do { 
        ret = agent->init();
        if (0 != ret) {
            break;
        }

        agent->addCli(ip, port, uid);

        agent->start();
        agent->wait();
    } while (0);

    agent->finish();
    delete agent;

    LOG_INFO("ret=%d| end asset agent|", ret);
    
    return ret;
}

int routerConf(const char* file) {
    int ret = 0;
    Service* agent = NULL;

    LOG_INFO("start asset agent|");

    agent = new Service;

    do { 
        ret = agent->parseInit(file);
        if (0 != ret) {
            break;
        }

        agent->start();
        agent->wait();
    } while (0);

    agent->finish();
    delete agent;

    LOG_INFO("ret=%d| end asset agent|", ret);
    
    return ret;
}

void usage(const char* prog) {
    LOG_ERROR("usage: %s <is_svr> <ip> <port>, examples:\n"
        "router_svr: %s 1 192.168.1.200 8080\n"
        "router_cli: %s 2 192.168.1.200 8080 <ip2> <port2> <uid>\n"
        "asset_agent: %s 3 192.168.1.200 8080 <uid>\n"
        "asset_agent: %s 4 <conf_file>\n", 
        prog, prog, prog, prog, prog);
}

int testRouter(int argc, char* argv[]) {
    const char* ip = NULL;
    const char* ip2 = NULL;
    const char* file = NULL;
    int opt = 0;
    int port2 = 0;
    int port = 0;
    int ret = 0;
    unsigned uid = 0;

    armSigs();

    if (2 <= argc) {
        opt = atoi(argv[1]);

        if (3 == argc && 4 == opt) {
            file = argv[2];
            ret = routerConf(file);
        } else if (4 <= argc) {
            ip = argv[2];
            port = atoi(argv[3]);

            if (1 == opt) {
                ret = serviceRouter(ip, port); 
            } else if (2 == opt && 7 == argc) {
                ip2 = argv[4];
                port2 = atoi(argv[5]);
                uid = atoi(argv[6]);
                ret = agentRouter(ip, port, ip2, port2, uid);
            } else if (3 == opt && 5 == argc) { 
                uid = atoi(argv[4]);
                ret = assetAgent(ip, port, uid);
            } else {
                usage(argv[0]);
                return -1;
            }
        } else {
            usage(argv[0]);
            return -1;
        }
        
        return ret;
    } else { 
        usage(argv[0]);
        return -1;
    }
}

