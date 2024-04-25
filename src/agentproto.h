#ifndef __AGENTPROTO_H__
#define __AGENTPROTO_H__
#include"isockapi.h" 


struct SockBuffer;
class SockFrame; 

class AgentProto {
public:
    AgentProto(SockFrame* frame);
    ~AgentProto(); 

    int parseData(int direction, int fd, 
        SockBuffer* cache, const char* buf, 
        int size);

private: 
    int dispatch(int fd, NodeMsg* pMsg);
    
    int parseBody(int direction, int fd,
        SockBuffer* buffer, 
        const char* input, int len);

    int parseHead(SockBuffer* buffer,
        const char* input, int len);

    bool analyseHead(SockBuffer* buffer);

private:
    SockFrame* m_frame;
}; 

#endif

