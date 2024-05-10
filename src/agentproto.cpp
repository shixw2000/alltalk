#include<errno.h>
#include<cstring>
#include<cstdlib>
#include"agentproto.h"
#include"sockframe.h"
#include"isockmsg.h"
#include"msgcenter.h"
#include"cache.h"


AgentProto::AgentProto(SockFrame* frame) {
    m_frame = frame;
}

AgentProto::~AgentProto() {
} 

int AgentProto::dispatch(int fd, NodeMsg* pMsg) {
    int ret = 0; 

    ret = m_frame->dispatch(fd, pMsg);
    return ret;
}

int AgentProto::parseData(int direction,
    int fd, SockBuffer* cache, 
    const char* buf, int size) {
    int ret = 0;
    
    while (0 < size) {
        if ((int)cache->m_pos < DEF_MSG_HEAD_SIZE) {
            ret = parseHead(cache, buf, size);
        } else {
            ret = parseBody(direction, fd,
                cache, buf, size);
        }
        
        if (0 <= ret) {
            buf += ret;
            size -= ret;
        } else {
            return ret;
        }
    }

    return 0;
}

bool AgentProto::analyseHead(SockBuffer* buffer) {
    NodeMsg* pMsg = NULL;
    MsgHead_t* ph = (MsgHead_t*)buffer->m_head;
    
    if (MsgCenter::chkValidHead(ph)) {
        pMsg = MsgCenter::allocMsg(ph->m_size);

        MsgCenter::fillMsg(pMsg, 
            buffer->m_head, DEF_MSG_HEAD_SIZE);
        
        buffer->m_msg = pMsg;
        return true;
    } else {
        return false;
    }
}

int AgentProto::parseHead(SockBuffer* buffer,
    const char* input, int len) {
    int cnt = 0;
    int used = 0;
    bool bOk = false;
    char* psz = NULL;
    
    if ((int)buffer->m_pos < DEF_MSG_HEAD_SIZE) {
        cnt = DEF_MSG_HEAD_SIZE - buffer->m_pos;
        psz = &buffer->m_head[buffer->m_pos];
        
        if (cnt <= len) {
            CacheUtil::bcopy(psz, input, cnt);

            used += cnt;
            buffer->m_pos = DEF_MSG_HEAD_SIZE;

            bOk = analyseHead(buffer);
            if (!bOk) { 
                return -1;
            } 
        } else {
            CacheUtil::bcopy(psz, input, len);

            used += len;
            buffer->m_pos += len;
        }
    } 

    return used;
}

int AgentProto::parseBody(int direction,
    int fd, SockBuffer* buffer, 
    const char* input, int len) {
    NodeMsg* pMsg = buffer->m_msg;
    int size = 0;
    int used = 0;
    int left = 0;

    (void)size;
    size = MsgCenter::getMsgSize(pMsg);
    left = MsgCenter::getMsgLeft(pMsg);
    if (left <= len) {
        if (0 < left) {
            /* get a completed msg */
            MsgCenter::fillMsg(pMsg, input, left);
            used += left;
        }

        LOG_DEBUG("parse_msg| fd=%d| len=%d|"
            " size=%d| left=%d|",
            fd, len, size, left);

        /* dispatch a completed msg */
        MsgCenter::setDirection(pMsg, direction);
        dispatch(fd, pMsg);
        buffer->m_msg = NULL;
        buffer->m_pos = 0;
    } else {
        LOG_DEBUG("skip_msg| fd=%d| len=%d|"
            " size=%d| left=%d|",
            fd, len, size, left);
        
        MsgCenter::fillMsg(pMsg, input, len); 
        used += len;
    }

    return used;
}


