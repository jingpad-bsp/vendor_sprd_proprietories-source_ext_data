/* Copyright (C) 2016 Spreadtrum Communications Inc. */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cutils/properties.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "utils.h"

int read_cmd(int fd, char *cmd, size_t len) {
    char *p = cmd;
    char c;

    while (p < cmd + len) {
        int r = read(fd, &c, 1);

        if (r < 0) {
            if (errno == EINTR)
                continue;

            ALOGE("Read cmd error: %s\n", strerror(errno));
            return r;
        } else if (r == 0) {
            ALOGI("Client %d closed by peer!\n", fd);
            return -1;
        }

        if (p == cmd)
            ALOGD("The first char is %d\n", c);

        if (c == '\0') {
            *p = c;
            break;
        } else {
            *p++ = c;
        }
    }

    if (p == cmd + len) {
        *(p - 1) = '\0';
        ALOGE("Cmd %s is too long, abort!\n", cmd);
        return -1;
    }

    return p - cmd;
}

int cmd2type(char **cmd) {
    if (!strncasecmp(*cmd, "<preifup>", SSLEN("<preifup>"))) {
        *cmd += SSLEN("<preifup>");
        return CMD_TYPE_PREIFUP;
    } else if (!strncasecmp(*cmd, "<ifup>", SSLEN("<ifup>"))) {
        *cmd += SSLEN("<ifup>");
        return CMD_TYPE_IFUP;
    } else if (!strncasecmp(*cmd, "<ifdown>", SSLEN("<ifdown>"))) {
        *cmd += SSLEN("<ifdown>");
        return CMD_TYPE_IFDOWN;
    } else if(!strncasecmp(*cmd, "<dataOffDisable>", SSLEN("<dataOffDisable>"))) {
        *cmd += SSLEN("<dataOffDisable>");
        return CMD_TYPE_DATAOFF_DISABLE;
    } else if(!strncasecmp(*cmd, "<dataOffEnable>", SSLEN("<dataOffEnable>"))) {
        *cmd += SSLEN("<dataOffEnable>");
        return CMD_TYPE_DATAOFF_ENABLE;
    }  else if (!strncasecmp(*cmd, "<pppup>", SSLEN("<pppup>"))) {
        *cmd += SSLEN("<pppup>");
        return CMD_TYPE_PPPUP;
    }else if (!strncasecmp(*cmd, "<startpppd>", SSLEN("<startpppd>"))) {
        *cmd += SSLEN("<startpppd>");
        return CMD_TYPE_PPPD_START;
    } else if (!strncasecmp(*cmd, "<stoppppd>", SSLEN("<stoppppd>"))) {
        *cmd += SSLEN("<stoppppd>");
        return CMD_TYPE_PPPD_STOP;
    }

    return -1;
}

int ipv6_need_disable(const unsigned int pdp_type) {
#if 0
    char prop[PROPERTY_VALUE_MAX] = "";
    char ip_type[64];

    /* 1-ipv4, 2-ipv6, 3-ipv4v6, if not set, suppose v4v6 is on */
    snprintf(ip_type, sizeof ip_type, "net.%s.ip_type", ifname);
    property_get(ip_type, prop, "3");
    ALOGD("%s is %s\n", ip_type, prop);
#endif
    if (pdp_type == 1)
        return 1;
    else
        return 0;
}

int parse_cmd(char *cmd, struct command *c) {
    char *ifname, *v4v6, *autotest, *ipv4addr;
    char *saveptr = NULL;
    int type;
    char *slotIndex, *sPort;
    char * ttyName, *local, *remote, *dns1, *dns2;

    if (!strstr(cmd, "ext_data")) {
        ALOGE("wrong cmd: %s\n", cmd);
        return -1;
    }
    cmd += SSLEN("ext_data");

    if ((type=cmd2type(&cmd)) < 0) {
        ALOGE("Parse the cmdtype fail: %s\n", cmd);
        return -1;
    }
    c->cmdtype = type;

    if(CMD_TYPE_DATAOFF_ENABLE == type || CMD_TYPE_DATAOFF_DISABLE == type) {

        slotIndex = strtok_r(cmd, ";", &saveptr);
        sPort = strtok_r(NULL, ";", &saveptr);
        c->slotIndex = atoi(slotIndex);
        c->sPort = atoi(sPort);

        ALOGE("ps data off , Parse the cmdtype :slot =  %d, sport = %d\n", c->slotIndex, c->sPort);
        return 0;
    }

    if(CMD_TYPE_PPPD_START == type) {
        ttyName = strtok_r(cmd, ";", &saveptr);
        local = strtok_r(NULL, ";", &saveptr);
        remote = strtok_r(NULL, ";", &saveptr);
        dns1 = strtok_r(NULL, ";", &saveptr);
        dns2 = strtok_r(NULL, ";", &saveptr);

        if ( NULL == ttyName || NULL == local || NULL ==  remote || NULL == dns1 || NULL == dns2) {
            ALOGE("ppp ! , parse_cmd() there is null !\n" );
            return -2;
        }

        c->ttyName = ttyName;

        snprintf(c->local, sizeof(c->local),"%s",local);
        snprintf(c->remote, sizeof(c->ipv4addr),"%s",remote);
        snprintf(c->dns1, sizeof(c->ipv4addr),"%s",dns1);
        snprintf(c->dns2, sizeof(c->ipv4addr),"%s",dns2);

        ALOGE("ppp ! , ext_data start_pppd =  c->ttyName = %s, c->local = %s, c->remote= %s,  c->dns1 = %s, c->dns2 = %s \n", c->ttyName, c->local, c->remote, c->dns1,c->dns2 );

        return 0;
    }

    if (CMD_TYPE_PPPD_STOP == type) {
        return 0;
    }

    ifname = strtok_r(cmd, ";", &saveptr);
    if (CMD_TYPE_PPPUP == c->cmdtype) {
        /*parse ipv4 address for ppp*/
        ipv4addr = NULL;
        ipv4addr = strtok_r(NULL, ";", &saveptr);
        if (ipv4addr == NULL || ifname == NULL) {
             ALOGE("Parse cmd fail: ifname =%s , ipv4addr=%s\n",
             ifname ? ifname : "", ipv4addr ? ipv4addr : "");
             return -2;
        }
        c->ifname = ifname;
        snprintf(c->ipv4addr, sizeof(c->ipv4addr),"%s",ipv4addr);
        ALOGE("ppp , Parse the cmdtype :ifname =  %s, ipv4addr = %s\n", c->ifname, c->ipv4addr);
        return 0;
    }

    v4v6 = strtok_r(NULL, ";", &saveptr);
    autotest = strtok_r(NULL, ";", &saveptr);

    if (ifname == NULL || v4v6 == NULL || autotest == NULL) {
        ALOGE("Parse cmd fail: ifname=%s, v4v6=%s, auto=%s\n",
              ifname ? ifname : "", v4v6 ? v4v6 : "",
              autotest ? autotest : "");
        return -2;
    }

    c->ifname = ifname;
    if (!strcasecmp(v4v6, "IPV4")) {
        c->pdp_type = PDP_ACTIVE_IPV4;
    } else if (!strcasecmp(v4v6, "IPV6")) {
        c->pdp_type = PDP_ACTIVE_IPV6;
    } else if (!strcasecmp(v4v6, "IPV4V6")) {
        c->pdp_type = PDP_ACTIVE_IPV4 | PDP_ACTIVE_IPV6;
    } else {
        ALOGE("IPV4V6 type is wrong: %s\n", v4v6);
        return -3;
    }

    c->is_autotest = !!atoi(autotest);

    ALOGD("Parse ok: cmd=%d, ifname=%s, ipv6=%s(%x), autotest=%d\n",
          c->cmdtype, c->ifname, v4v6, c->pdp_type, c->is_autotest);
    return 0;
}

int write_result(int fd, void *buf, int len) {
    char *result = buf;
    int n, done;

    for (done = 0; done < len; done += n) {
        n = write(fd, &result[done], len-done);
        if (n == -1) {
            if (errno == EINTR) {
                n = 0;
            } else {
                ALOGE("write result error: %s\n", strerror(errno));
                return -1;
            }
        }
    }

    return len;
}

int exec_cmd(const char *cmd_fmt, ...) {
    char cmd[128];
    va_list va;
    int ecode;

    va_start(va, cmd_fmt);
    vsnprintf(cmd, sizeof cmd, cmd_fmt, va);
    va_end(va);

    ecode = system(cmd);
    ALOGD("%s exit with %d\n", cmd, ecode);
    return ecode;
}

int write_file(const char *path, const char *value) {
    int fd, len;

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        ALOGE("open file %s fail: %s\n", path, strerror(errno));
        return -1;
    }

    len = strlen(value);
    if (write(fd, value, len) != len) {
        ALOGE("write %s to file %s fail: %s\n", value, path, strerror(errno));
        close(fd);
        return -1;
    }

    ALOGD("write %s to file %s ok\n", value, path);
    close(fd);
    return 0;
}
