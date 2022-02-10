/*
 * Convert shell script(ext_data.sh) to source code. The basic functions such
 * as configure ip address, up/down seth netcard... are moved to phoneserver.
 * And filter misc packets, data transmission test... are implemented here.
 *
 * Liping Zhang <liping.zhang@spreadtrum.com>
 * Copyright (C) 2016 Spreadtrum Communications Inc.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <linux/rtnetlink.h>
#include <unistd.h>

#include "utils.h"
#include "auto_test.h"

static void do_pre_ifup(struct command *c) {
    char path[128];

    /* For ZTE lab test, do not drop RA from local IP */
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/accept_ra_from_local",
             c->ifname);
    write_file(path, "1");

    /* Reduce the time cost of getting the ipv6's global address */
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/router_solicitation_interval",
             c->ifname);
    write_file(path, "1");
    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/router_solicitations",
             c->ifname);
    write_file(path, "-1");

    snprintf(path, sizeof path,
             "/proc/sys/net/ipv6/conf/%s/accept_dad",
             c->ifname);
    write_file(path, "-1");

    /* For CTS and BIP test pass! */
    exec_cmd("ip6tables -w -D OUTPUT -o %s -p icmpv6 -j DROP", c->ifname);
    if (ipv6_need_disable(c->pdp_type))
        exec_cmd("ip6tables -w -I OUTPUT -o %s -p icmpv6 -j DROP", c->ifname);

    return;
}

static void do_ifup(struct command *c) {
    if (c->is_autotest)
        start_autotest(c);
    return;
}

static void do_ifdown(struct command *c) {
    if (c->is_autotest)
        stop_autotest(c);

    return;
}

int send_string_toNetd(char *cmd)
{
    int localfd;
    int i;
    int len,nlen;
    int ret;

    for ( i=0; i<3; i++) {
    	localfd = socket_local_client(FW_DAEMON_SOCKNAME,
				 ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	if (localfd >= 0)
	   break;
	usleep(10000);//wait for 10ms
    }

    if ( localfd < 0 ) {
        ALOGE("Fail to connect socket server: %s (%d)\n", FW_DAEMON_SOCKNAME, localfd);
	return 1;
    }
    ALOGE("Send cmd [%s] connect socket server.\n", cmd);
    len = strlen(cmd);
    nlen = htonl(len);
    ret = send(localfd, (void*)&nlen, sizeof(nlen), 0);
    if (ret==-1) {
	 ALOGE("Fail to send cmd to socket server: %s\n", FW_DAEMON_SOCKNAME);
    } else {
    	ret = send(localfd, cmd, len, 0);
    }
    usleep(1000);//wait for 1ms
    close(localfd);
    return ret;
}

static void send_cmd_toNetd(struct command *c) {
    char cmd[256];

    memset(cmd,0,sizeof(cmd));
    switch (c->cmdtype) {
	case CMD_TYPE_PREIFUP:
		sprintf(cmd, "ext_data<preifup>%s;%d;%d",c->ifname, c->pdp_type, c->is_autotest);
		send_string_toNetd(cmd);
		break;
	case CMD_TYPE_IFUP:
		sprintf(cmd, "ext_data<ifup>%s;%d;%d",c->ifname, c->pdp_type, c->is_autotest);
		send_string_toNetd(cmd);
		break;
	case CMD_TYPE_IFDOWN:
		sprintf(cmd, "ext_data<ifdown>%s;%d;%d",c->ifname, c->pdp_type, c->is_autotest);
		send_string_toNetd(cmd);
		break;
    case CMD_TYPE_DATAOFF_DISABLE:
        sprintf(cmd, "ext_data<dataOffDisable>%d;%d",c->slotIndex, c->sPort);
		send_string_toNetd(cmd);
		break;
    case CMD_TYPE_DATAOFF_ENABLE:
        sprintf(cmd, "ext_data<dataOffEnable>%d;%d",c->slotIndex, c->sPort);
		send_string_toNetd(cmd);
		break;
    case CMD_TYPE_PPPUP:
        sprintf(cmd, "ext_data<pppup>%s;%s",c->ifname, c->ipv4addr);
        send_string_toNetd(cmd);
        break;
    case CMD_TYPE_PPPD_START:
        sprintf(cmd, "ext_data<startpppd>%s;%s;%s;%s;%s",c->ttyName, c->local, c->remote, c->dns1, c->dns2);
        send_string_toNetd(cmd);
        break;
    case CMD_TYPE_PPPD_STOP:
        sprintf(cmd, "ext_data<stoppppd>");
        send_string_toNetd(cmd);
        break;

	default:
		break;
    }
    return;
}

int process_cmd(struct command *c) {
    switch (c->cmdtype) {
    case CMD_TYPE_PREIFUP:
        do_pre_ifup(c);
        send_cmd_toNetd(c);
        break;

    case CMD_TYPE_IFUP:
        do_ifup(c);
        send_cmd_toNetd(c);
        break;

    case CMD_TYPE_IFDOWN:
        do_ifdown(c);
        send_cmd_toNetd(c);
        break;

    case CMD_TYPE_DATAOFF_ENABLE:
    case CMD_TYPE_DATAOFF_DISABLE:
         send_cmd_toNetd(c);
         break;

    case CMD_TYPE_PPPUP:
         send_cmd_toNetd(c);
         break;

    case CMD_TYPE_PPPD_START:
         send_cmd_toNetd(c);
         break;

    case CMD_TYPE_PPPD_STOP:
         send_cmd_toNetd(c);
         break;


    default:
        break;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct timeval tv;
    int srvfd;
    int localfd;

    signal(SIGPIPE, SIG_IGN);

    srvfd = socket_local_server(EXT_DATA_SOCKNAME,
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
    if (srvfd < 0) {
        ALOGE("Fail to create unix socket server: %s\n", strerror(errno));
        return -1;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (1) {
        struct command c;
        char cmd[256];
        int clifd;

        clifd = accept(srvfd, NULL, NULL);
        ALOGD("New client %d connected!\n", clifd);

        /*
          * Prevent some 'error' clients do not read the result, then suspend
          * our write() call.
          */
        (void) setsockopt(clifd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);

        while (read_cmd(clifd, cmd, sizeof cmd) > 0) {
            struct timeval start, end;
            int err = 0;

            gettimeofday(&start, NULL);
            ALOGI("**********Start[%s]**********\n", cmd);

            if (parse_cmd(cmd, &c) == 0)
                err = process_cmd(&c);

            if (write_result(clifd, &err, sizeof(err)) < 0)
                break;

            gettimeofday(&end, NULL);
            ALOGI("**********End[%lds, %ldus]**********\n",
                  end.tv_sec - start.tv_sec, end.tv_usec - start.tv_usec);
        }

        ALOGD("Client %d bye!\n", clifd);
        close(clifd);
    }

    close(srvfd);
    return 0;
}
