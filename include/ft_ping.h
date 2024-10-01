#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#define PING_PKT_S 64
#define PORT_NO 0

struct ping_pkt
{
    struct icmphdr hdr;
    char msg[PING_PKT_S - sizeof(struct icmphdr)];
};

struct ping_stats
{
    double min;
    double avg;
    double max;
    double stddev;
    double value[100000];
};

unsigned short checksum(void *b, int len);
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con);
void intHandler();
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip, char *host, int verbose_mode);
void put_stats(long double time, struct ping_stats *ping_stat);
void get_stddev(struct ping_stats *ping_stat, int count);
