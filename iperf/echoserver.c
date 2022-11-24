#include <stdio.h>
#include "lwip/tcp.h"
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

/* ECHO protocol states */
enum tcp_echoserver_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* maintaing connection infos to be passed as argument to LwIP callbacks*/
struct tcp_echoserver_struct
{
  u8_t state;             /* current connection state */
  struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
  struct pbuf *p;         /* pointer on the received/to be transmitted pbuf */
};

uint8_t remoteIp[4];
static struct tcp_pcb *tcp_echoserver_pcb;

static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_echoserver_error(void *arg, err_t err);
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);

/* Init tcp echo server */
void tcp_echoserver_init(int port)
{
    /* create new tcp pcb */
    tcp_echoserver_pcb = tcp_new();
    if (tcp_echoserver_pcb != NULL) {
        err_t err;
        /* binding local ip and port */
        err = tcp_bind(tcp_echoserver_pcb, IP_ADDR_ANY, port);
        if (err == ERR_OK) {
            /* tcp server listening and limit climit number */
            tcp_echoserver_pcb = tcp_listen_with_backlog(tcp_echoserver_pcb, 1);
            /* Init Lwip tcp accept callback */
            tcp_accept(tcp_echoserver_pcb, tcp_echoserver_accept);
        }
        else {
            memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
        }
    }
}

void tcp_echoserver_deinit(void)
{
    tcp_arg(tcp_echoserver_pcb, NULL);
    tcp_accept(tcp_echoserver_pcb, NULL);
    err_t err = tcp_close(tcp_echoserver_pcb);
    LWIP_ASSERT("tcp echoserver close err", err == ERR_OK);
}

 /* The implementation of tcp_accept LwIP callback */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    err_t ret_err;
    struct tcp_echoserver_struct *es;

    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(err);
    /* for tcp client set priority */
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    es = (struct tcp_echoserver_struct *)mem_malloc(sizeof(struct tcp_echoserver_struct));
    if(es == NULL) {
        tcp_echoserver_connection_close(newpcb, es);
        ret_err = ERR_MEM;
    }
    else {
        /* client pcb is established */
        es->state = ES_ACCEPTED;
        es->pcb = newpcb;
        es->p = NULL;

        tcp_arg(newpcb, es);
        tcp_recv(newpcb, tcp_echoserver_recv);
        tcp_err(newpcb, tcp_echoserver_error);
        tcp_poll(newpcb, tcp_echoserver_poll, 0);

        remoteIp[0] = newpcb->remote_ip.addr & 0xff;
        remoteIp[1] = (newpcb->remote_ip.addr >> 8) & 0xff;
        remoteIp[2] = (newpcb->remote_ip.addr >> 16) & 0xff;
        remoteIp[3] = (newpcb->remote_ip.addr >> 24) & 0xff;
        printf("Client IP:%d-%d-%d-%d Port: %d connect server\n",
                remoteIp[0],remoteIp[1],remoteIp[2],remoteIp[3],newpcb->remote_port);

        ret_err = ERR_OK;
    }
    return ret_err;  
}

/* The implementation for tcp_recv LwIP callback */
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct tcp_echoserver_struct *es;
    err_t ret_err;
    unsigned char *recv_data = NULL;

    LWIP_ASSERT("arg != NULL",arg != NULL);
    es = (struct tcp_echoserver_struct *)arg;

    /* recv empty data from client */
    if (p == NULL) {
        es->state = ES_CLOSING;
        if(es->p == NULL) {
            tcp_echoserver_connection_close(tpcb, es);
        }
        else {
            /* data remaining but not sent */
            tcp_sent(tpcb, tcp_echoserver_sent);
            tcp_echoserver_send(tpcb, es);
        }
        ret_err = ERR_OK;
    }   
    /* recv data but unkown error */
    else if(err != ERR_OK) {
        if (p != NULL) {
            es->p = NULL;
            pbuf_free(p);
        }
        ret_err = err;
    }
    else if(es->state == ES_ACCEPTED) {
        /* change server state recvived */
        es->state = ES_RECEIVED;
        es->p = p;
        /* echo function */
        tcp_sent(tpcb, tcp_echoserver_sent);
        recv_data = (unsigned char *)malloc(p->len*sizeof(char));
        if(recv_data != NULL) {
            memcpy(recv_data, p->payload, p->len);
            printf("Tcp server recv: %s\n", recv_data);
        }
        free(recv_data);
        tcp_echoserver_send(tpcb, es);
        ret_err = ERR_OK;
    }
    /* data remain from provious data has been already sent */
    else if (es->state == ES_RECEIVED) {
        if(es->p == NULL) {
            es->p = p;
            recv_data = (unsigned char *)malloc(p->len*sizeof(char)+1);
            if(recv_data != NULL) {
                memcpy(recv_data, p->payload, p->len);
                recv_data[p->len] = 0x0;
                printf("Tcp server recv: %s\n", recv_data);
            }
            free(recv_data);
            tcp_echoserver_send(tpcb, es);
        }
        else {
            struct pbuf *ptr;
            /* chain pbufs to the end of what we recved previrously */
            ptr = es->p;
            pbuf_chain(ptr,p);
        }
        ret_err = ERR_OK;
    }
    /* es->state == ES_CLOSING or other state */
    else {
        tcp_recved(tpcb, p->tot_len);
        es->p = NULL;
        pbuf_free(p);
        ret_err = ERR_OK;
    }
    return ret_err;
}

/* error callback func*/
static void tcp_echoserver_error(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(err);
  struct tcp_echoserver_struct *es;
  es = (struct tcp_echoserver_struct *)arg;
  if (es != NULL)   {
      mem_free(es);
  }
}

/* polling callbacks func */
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb)
{
    err_t ret_err;
    struct tcp_echoserver_struct *es;
    es = (struct tcp_echoserver_struct *)arg;
    if (es != NULL) {
        if (es->p != NULL) {
            tcp_sent(tpcb, tcp_echoserver_sent);
            /* there is a remaining pbuf (chain) , try to send data */
            tcp_echoserver_send(tpcb, es);
        }
        else {
            if(es->state == ES_CLOSING) {
                tcp_echoserver_connection_close(tpcb, es);
            }
        }
        ret_err = ERR_OK;
    }
    else {
        tcp_abort(tpcb);
        ret_err = ERR_ABRT;
    }
    return ret_err;
}

/* send finish callback func */
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  LWIP_UNUSED_ARG(len);
  struct tcp_echoserver_struct *es;
  es = (struct tcp_echoserver_struct *)arg;
  if(es->p != NULL) {
      /* still got pbufs to send */ 
      tcp_sent(tpcb, tcp_echoserver_sent);
      tcp_echoserver_send(tpcb, es);
  }
  else {
      if(es->state == ES_CLOSING)
          tcp_echoserver_connection_close(tpcb, es);
  }
  return ERR_OK;
}

/* send data to net card */
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
    struct pbuf *ptr;
    err_t wr_err = ERR_OK;
    /*
     * 1. tcp task no error
     * 2. have will send data
     * 3. len(will send data) < len(tcp send len) 
     */
    while ((wr_err == ERR_OK) &&
            (es->p != NULL) && 
            (es->p->len <= tcp_sndbuf(tpcb)))
    {
        /* get pointer op pbuf */
        ptr = es->p;
        /* send data to net card */
        wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
        if (wr_err == ERR_OK) {
            u16_t plen;
            u8_t freed;
            plen = ptr->len;

            es->p = ptr->next;
            if(es->p != NULL) {
                /* update counter */
                pbuf_ref(es->p);
            }
            do {
                freed = pbuf_free(ptr);
            } while(freed == 0);
            tcp_recved(tpcb, plen);
        }
        else if(wr_err == ERR_MEM) {
            es->p = ptr;
            tcp_output(tpcb);
        }
        else {
            /* other problem */
        }
    }
}

/* close callback func */
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);
    if (es != NULL) {
        mem_free(es);
    }  
    tcp_close(tpcb);
}
static void cmd_echoserver(struct cmd_arg *arg, int argc, char **argv)
{
    LinkStatusTypeDef st;
    wlan_cli_status(&st);
    if(st.conn_state == 0) {
        printf("Iperf Start Failed! Please start wifi first.\n");
        return;
    }
    if(argc < 2) {
        printf("Usage: echoserver <port>\n");
        printf("- port: server port is bwtween 0 and 65535, default 5001\n");
    }
    if(!strcmp(argv[1], "stop"))
        tcp_echoserver_deinit();
    else {
        int port = atoi(argv[1]);
        port_check(port);
        tcp_echoserver_init(port);
    }
}

void init_cmd_echoserver(void)
{
    shell_cmd_register(cmd_echoserver, "echoserver", NULL, "echo server");
}
