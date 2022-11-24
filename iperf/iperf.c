#include <stdio.h>
#include <lwip/apps/lwiperf.h>
#include "module_process.h"
#include "wifi_cli.h"

#define DEFAULT_TCP_SERVER_PORT 5001
#define port_check(n) \
    do {              \
        if(n>65535 || n<0){     \
            n = DEFAULT_TCP_SERVER_PORT;\
            printf("Port non-compliant. Use default port 5001\n");\
        }\
    }while(0);
static void *iperf_handler = NULL;

static void lwiperf_report(void *arg, enum lwiperf_report_type report_type,
  const ip_addr_t* local_addr, u16_t local_port, const ip_addr_t* remote_addr, u16_t remote_port,
  u32_t bytes_transferred, u32_t ms_duration, u32_t bandwidth_kbitpsec)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(local_addr);
    LWIP_UNUSED_ARG(local_port);
    printf("IPERF report: type=%d, remote: %s:%d, total bytes: %"U32_F", duration in ms: %"U32_F", Mbits/s: %.2f\n",
            (int)report_type, ipaddr_ntoa(remote_addr), (int)remote_port, bytes_transferred, ms_duration, (bandwidth_kbitpsec*1.0)/1000);
}

static void cmd_iperf(struct cmd_arg *arg, int argc, char **argv)
{
    LinkStatusTypeDef st;
    wlan_cli_status(&st);
    if(st.conn_state == 0) {
        printf("Iperf Start Failed! Please start wifi first.\n");
        return;
    }
    if(!strcmp(argv[1], "server")) {
        if(iperf_handler != NULL) {
            printf("Iperf server is running\n");
            return ;
        }
        ip_addr_t  server_addr;
        server_addr.addr = IP_ADDR_ANY->addr;
        int server_port = LWIPERF_TCP_PORT_DEFAULT;
        if(argc == 3)
            ipaddr_aton((const char *)argv[2], &server_addr);
        else if(argc == 4) {
            ipaddr_aton((const char *)argv[2], &server_addr);
            server_port = atoi(argv[3]);
            port_check(server_port);
        }
        iperf_handler = lwiperf_start_tcp_server(&server_addr, server_port, lwiperf_report, NULL);
    }
    else if(!strcmp(argv[1], "client")) {
        if(argc < 3) {
            printf("Usage: iperf client remote_addr <remote_port> <client_type>\n\n");
            printf("remote_port: default 5001\n");
            printf("client_type: client / dual / tradeoff, default client\n");
            return;
        }
        int remote_port = LWIPERF_TCP_PORT_DEFAULT;
        int client_type = LWIPERF_CLIENT;
        ip_addr_t  remote_addr;
        ipaddr_aton((const char *)argv[2], &remote_addr);
        if(argc == 4)
            remote_port = atoi(argv[3]);
        else if(argc == 5) {
            remote_port = atoi(argv[3]);
            if(!strcmp(argv[4], "dual"))
                client_type = LWIPERF_DUAL;
            else if(!strcmp(argv[4], "tradeoff"))
                client_type = LWIPERF_TRADEOFF;
        }
        port_check(remote_port);
        lwiperf_start_tcp_client(&remote_addr, remote_port, client_type, lwiperf_report, NULL);
    }
    else if(!strcmp(argv[1], "stop")) {
        lwiperf_stop_tcp_server(iperf_handler);
    }
    else
        printf("Usage : iperf <server/client/stop> <remote_addr> <remote port>\n");
}

void init_cmd_iperf_tool(void)
{
    shell_cmd_register(cmd_iperf, "iperf", NULL, "network performance measurement and tuning");
}
