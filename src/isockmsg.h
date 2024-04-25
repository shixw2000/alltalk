#ifndef __ISOCKMSG_H__
#define __ISOCKMSG_H__
#include"shareheader.h"
#include"socktool.h"


/* message from here alignment with 1 byte */
#pragma pack(push, 1)


struct MsgHead_t {
    int m_size;
    unsigned m_version;
    unsigned m_crc;
    unsigned m_seq;
    unsigned m_cmd;
    unsigned m_uid1;
    unsigned m_sid1;
    unsigned m_uid2;
    unsigned m_sid2;
    char m_body[0];
};

struct MsgHelo {
    unsigned m_timestamp;
};

struct MsgErrAck {
    unsigned long m_taskid;
    int m_reason;
    unsigned m_mac;
};

struct MsgLoginReq {
    unsigned long m_token;
    unsigned m_timestamp;
    unsigned m_uid1;
    unsigned m_random1;
    unsigned m_mac;
};

struct MsgExchKey {
    unsigned long m_token;
    unsigned long m_taskid;
    unsigned m_uid1;
    unsigned m_uid2; 
    unsigned m_random2;
    unsigned m_mac;
};

struct MsgLoginEnd {
    unsigned long m_token;
    unsigned long m_taskid;
    unsigned m_uid1;
    unsigned m_uid2;
    unsigned m_mac;
};

struct MsgUnreach {
    int m_reason;
    unsigned m_uid;
    unsigned m_sid;
    unsigned m_mac;
};

struct MsgTunnel {
    unsigned long m_taskid;
    unsigned m_uid1;
    unsigned m_sid1;
    unsigned m_link_no;
    int m_link_type;
    int m_port;
    unsigned m_mac;
    char m_ip[DEF_IP_SIZE];
};

struct MsgTunnelAck {
    unsigned long m_taskid;
    unsigned m_sid1;
    unsigned m_uid2;
    unsigned m_sid2;
    unsigned m_link_no;
    unsigned m_mac;
};

struct MsgCloseTunnel {
    unsigned long m_taskid;
    unsigned m_uid;
    unsigned m_sid;
    int m_reason;
    unsigned m_mac;
};

struct MsgLogout {
    unsigned long m_taskid;
    unsigned m_uid1;
    unsigned m_uid2;
    unsigned m_mac;
};

struct MsgPlainTxt {
    unsigned long m_taskid;
    unsigned m_link_no;
    int m_txt_len;
    unsigned m_mac;
    char m_txt[0];
};

struct MsgCipherTxt {
    unsigned long m_taskid;
    unsigned m_link_no;
    int m_cipher_len;
    int m_txt_len;
    unsigned m_mac;
    char m_cipher[0];
};

struct MsgUsrExitBroadcast {
    unsigned m_uid;
    int m_reason;
    unsigned m_mac;
};


#pragma pack(pop)

static const int DEF_MSG_HEAD_SIZE = sizeof(MsgHead_t);
static const int MAX_MSG_SIZE = 1024 * 1024 * 2;
static const unsigned CURR_VERSION = 0x20240424;

struct NodeMsg;

struct SockBuffer {
    NodeMsg* m_msg;
    unsigned m_pos;
    char m_head[DEF_MSG_HEAD_SIZE];
};

struct PreHeadData {
    int m_direction;
} __attribute__((aligned(8)));

enum EnumMsgCode {
    ENUM_MSG_SYSTEM_MIN = 0x1000, // begin system cmds 
    ENUM_MSG_HELO,
    ENUM_MSG_ERROR_ACK,
    ENUM_MSG_LOGIN_REQ,
    ENUM_MSG_EXCH_KEY,
    
    ENUM_MSG_LOGIN_END, 
    ENUM_MSG_LOGOUT, 
    ENUM_MSG_FRAME_PLAIN,
    ENUM_MSG_FRAME_CIPHER, 
    ENUM_MSG_UNREACH,
    
    ENUM_MSG_TUNNEL_CLOSE,
    ENUM_MSG_TUNNEL_REQ,
    ENUM_MSG_TUNNEL_ACK, 
    ENUM_MSG_UID_EXIT_BROADCAST,
    
    ENUM_MSG_CODE_END
};

enum EnumErrReason {
    ENUM_ERR_NO_USER = 1000,
    ENUM_ERR_NO_SID,
    ENUM_ERR_TUNNEL_PARAM,
    ENUM_ERR_TUNNEL_RESOURCE,
    
    ENUM_ERR_OTHER,
};

#endif

