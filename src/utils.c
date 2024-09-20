#include "../include/ft_ping.h"

unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;
    return result;
}

char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con) {
    struct hostent *host_entity;
    char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));

    if ((host_entity = gethostbyname(addr_host)) == NULL) {
        return NULL;
    }

    strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
    addr_con->sin_family = host_entity->h_addrtype;
    addr_con->sin_port = htons(PORT_NO);
    addr_con->sin_addr.s_addr = *(long *)host_entity->h_addr;

    return ip;
}

void put_stats(long double time, struct ping_stats *ping_stat) {
    if (ping_stat->min > time)
        ping_stat->min = time;
    if (ping_stat->max < time)
        ping_stat->max = time;
    ping_stat->avg += time;    
}

void get_stddev(struct ping_stats *ping_stat, int count) {
    ping_stat->avg = ping_stat->avg / count;

    for (int i = 0; i < count; i++) {
        ping_stat->stddev += pow(ping_stat->value[i] - ping_stat->avg, 2);
    }

    ping_stat->stddev = sqrt(ping_stat->stddev / count);

}
