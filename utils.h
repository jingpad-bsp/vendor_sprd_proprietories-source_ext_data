/* Copyright (C) 2016 Spreadtrum Communications Inc. */
#ifndef EXTDATA_UTILS_H_
#define EXTDATA_UTILS_H_

#ifdef __cplusplus
    extern "C" {
#endif

#define EXT_DATA_SOCKNAME   "hidl_net_ext_data"
#define FW_DAEMON_SOCKNAME   "oemDaemonSrv"
#define LOG_TAG EXT_DATA_SOCKNAME
#include <cutils/log.h>

#define SSLEN(str)  (sizeof(str) - 1)

enum cmd_type {
    CMD_TYPE_PREIFUP,
    CMD_TYPE_IFUP,
    CMD_TYPE_IFDOWN,
    CMD_TYPE_DATAOFF_ENABLE,
    CMD_TYPE_DATAOFF_DISABLE,
    CMD_TYPE_PPPUP,
    CMD_TYPE_PPPD_START,
    CMD_TYPE_PPPD_STOP,
    CMD_TYPE_END
};

#define PDP_ACTIVE_IPV4    0x0001
#define PDP_ACTIVE_IPV6    0x0002

struct command {
    enum cmd_type cmdtype;
    char *ifname;
    unsigned int pdp_type;
    int is_autotest;
    int slotIndex; //for dataoff
    int sPort; //for dataoff
    char ipv4addr[32];//for ppp call
    char *ttyName; //for ppp call
    char local[32];//for ppp call
    char remote[32];//for ppp call
    char dns1[32];//form ppp call
    char dns2[32];//for ppp call
};

int ipv6_need_disable(const unsigned int pdp_type);
int read_cmd(int fd, char *cmd, size_t len);
int parse_cmd(char *cmd, struct command *c);
int write_result(int fd, void *buf, int len);
int exec_cmd(const char *cmd_fmt, ...) \
             __attribute__((__format__(printf, 1, 2)));
int write_file(const char *path, const char *value);

#ifdef __cplusplus
    }
#endif

#endif
