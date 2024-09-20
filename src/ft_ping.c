#include "../include/ft_ping.h"

int pingloop;

char* error_message = "usage: ping [-AaDdfnoQqRrv] [-c count] [-G sweepmaxsize]\n"
                      "            [-g sweepminsize] [-h sweepincrsize] [-i wait]\n"
                      "            [-l preload] [-M mask | time] [-m ttl] [-p pattern]\n"
                      "            [-S src_addr] [-s packetsize] [-t timeout][-W waittime]\n"
                      "            [-z tos] host\n"
                      "       ping [-AaDdfLnoQqRrv] [-c count] [-I iface] [-i wait]\n"
                      "            [-l preload] [-M mask | time] [-m ttl] [-p pattern] [-S src_addr]\n"
                      "            [-s packetsize] [-T ttl] [-t timeout] [-W waittime]\n"
                      "            [-z tos] mcast-group\n"
                      "Apple specific options (to be specified before mcast-group or host like all options)\n"
                      "            -b boundif           # bind the socket to the interface\n"
                      "            -k traffic_class     # set traffic class socket option\n"
                      "            -K net_service_type  # set traffic class socket options\n"
                      "            --apple-connect      # call connect(2) in the socket\n"
                      "            --apple-time         # display current time\n";


void intHandler() {
    pingloop = 0;
}    

void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip, char *host)
{
    int ttl_val = 64;
    int msg_count = 0;
    int msg_received_count = 0;
    char buffer[128];
    struct timespec time_start, time_end;
    int flag, print_statistic = 1;
    long double rtt_msec = 0;

    if (setsockopt(ping_sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
    {
        fprintf(stderr, "Setting socket options to TTL failed!\n");
        return;
    }

    int packet_size = PING_PKT_S - sizeof(struct icmphdr);
    printf("PING %s (%s): %d data bytes\n", host, ping_ip, packet_size);


    struct timeval tv_out;
    tv_out.tv_sec = 1;
    tv_out.tv_usec = 0;

    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv_out, sizeof tv_out);

    struct ping_pkt pckt;
    struct ping_stats stats;

    stats.min = 100000;
    stats.max = 0;
    stats.avg = 0;
    stats.stddev = 0;

    struct sockaddr_in r_addr;
    socklen_t addr_len = sizeof(r_addr);

    int j = 0;
    while (pingloop)
    {
        flag = 1;
        bzero(&pckt, sizeof(pckt));
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid();

        unsigned long i;

        for (i = 0; i < sizeof(pckt.msg) - 1; i++)
            pckt.msg[i] = i + '0';

        pckt.msg[i] = 0;
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
        usleep(1000000);

        clock_gettime(CLOCK_MONOTONIC, &time_start);

        if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr *)ping_addr, sizeof(*ping_addr)) <= 0)
        {
            fprintf(stderr, "ping: sendto: No route to host\n");
            flag = 0;
        }

        if (pingloop == 0)
            break;
        
        pckt.hdr.un.echo.sequence = msg_count++;
        if (recvfrom(ping_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&r_addr, &addr_len) > 0)
        {
            clock_gettime(CLOCK_MONOTONIC, &time_end);
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000;
            rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000 + timeElapsed;

            if (flag)
            {
                struct icmphdr *recv_hdr = (struct icmphdr *)buffer;
                if (recv_hdr->type == 0 && recv_hdr->code == 0)
		{
		    printf("statistic = %d\n", print_statistic);
                    print_statistic = 0;
		}
                else
                {
                    printf("%d bytes from %s: icmp_seq = %d ttl = %d time = %.3Lf ms\n", PING_PKT_S, ping_ip, msg_count - 1, ttl_val, rtt_msec);
                    put_stats(rtt_msec, &stats);
                    stats.value[j] = rtt_msec;
                    msg_received_count++;
                    j++;
                }
            }
        }
	else
	    print_statistic = 0;
    }
    get_stddev(&stats, msg_count);
    printf("--- %s ping statistics ---\n", host);
    printf("%d packets transmitted, %d packets received, %d%% packet loss\n", msg_count, msg_received_count, ((msg_count - msg_received_count) / msg_count) * 100);
    
    if (print_statistic)
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", stats.min, stats.avg, stats.max, stats.stddev);

}

int main(int argc, char *argv[])
{
    int sockfd;
    char *ip_addr;
    struct sockaddr_in addr_con;

    pingloop = 1;

    if (argc != 2)
    {
        fprintf(stderr, "...\n");
        exit(EXIT_FAILURE);
    }

    ip_addr = dns_lookup(argv[1], &addr_con);

    if (ip_addr == NULL)
    {
        fprintf(stderr, "ping: cannot resolve %s: Unknown host\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        fprintf(stderr, "Socket file descriptor not received!\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, intHandler); 
    
    send_ping(sockfd, &addr_con, ip_addr, argv[1]);
    close(sockfd);
    free(ip_addr);

    return 0;
}
