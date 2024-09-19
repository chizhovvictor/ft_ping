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

int pingloop;

//для макоса (потом закоментить и протестить на линуксе)
struct icmphdr {
    uint8_t type;       // Тип ICMP-сообщения
    uint8_t code;       // Код ICMP-сообщения
    uint16_t checksum;  // Контрольная сумма
    union {
        struct {
            uint16_t id;        // Идентификатор сообщения
            uint16_t sequence;  // Номер последовательности сообщения
        } echo;
        uint32_t gateway;       // Используется для Destination Unreachable
    } un;
};


struct ping_pkt {
    struct icmphdr hdr;  // Заголовок ICMP-пакета
    char msg[PING_PKT_S - sizeof(struct icmphdr)]; // Данные пакета
};

struct ping_stats {
    double min;
    double avg;
    double max;
    double stddev;
    double value[];
};

unsigned short checksum(void *b, int len);
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con);
void intHandler();
void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip, char *host);
void put_stats(long double time, struct ping_stats *ping_stat);
void get_stddev(struct ping_stats *ping_stat, int count);