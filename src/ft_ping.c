#include "../include/ft_ping.h"


char *question_msg = 	"Usage: ping [OPTION...] HOST ...\n"
			"Send ICMP ECHO_REQUEST packets to network hosts.\n\n"
			" Options controlling ICMP request types:\n"
			"      --address              send ICMP_ADDRESS packets (root only)\n"
			"      --echo                 send ICMP_ECHO packets (default)\n"
			"      --mask                 same as --address\n"
			"      --timestamp            send ICMP_TIMESTAMP packets\n"
			"  -t, --type=TYPE            send TYPE packets\n\n"
			" Options valid for all request types:\n\n"
			"  -c, --count=NUMBER         stop after sending NUMBER packets\n"
			"  -d, --debug                set the SO_DEBUG option\n"
			"  -i, --interval=NUMBER      wait NUMBER seconds between sending each packet\n"
			"  -n, --numeric              do not resolve host addresses\n"
			"  -r, --ignore-routing       send directly to a host on an attached network\n"
			"      --ttl=N                specify N as time-to-live\n"
			"  -T, --tos=NUM              set type of service (TOS) to NUM\n"
			"  -v, --verbose              verbose output\n"
			"  -w, --timeout=N            stop after N seconds\n"
			"  -W, --linger=N             number of seconds to wait for response\n\n"
			" Options valid for --echo requests:\n\n"
			"  -f, --flood                flood ping (root only)\n"
			"      --ip-timestamp=FLAG    IP timestamp of type FLAG, which is one of\n"
			"                             \"tsonly\" and \"tsaddr\"\n"
			"  -l, --preload=NUMBER       send NUMBER packets as fast as possible before\n"
			"                             falling into normal mode of behavior (root only)\n"
			"  -p, --pattern=PATTERN      fill ICMP packet with given pattern (hex)\n"
			"  -q, --quiet                quiet output\n"
			"  -R, --route                record route\n"
			"  -s, --size=NUMBER          send NUMBER data octets\n\n"
			"  -?, --help                 give this help list\n"
			"      --usage                give a short usage message\n"
			"  -V, --version              print program version\n\n"
			"Mandatory or optional arguments to long options are also mandatory or optional\n"
			"for any corresponding short options.\n\n"
			"Options marked with (root only) are available only to superuser.\n\n"
			"Report bugs to <bug-inetutils@gnu.org>.\n";

int pingloop;

void intHandler() {
    pingloop = 0;
}    

void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip, char *host)
{
    int ttl_val = 63;
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
            //fprintf(stderr, "ping: sendto: No route to host\n");
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
                // struct icmphdr *recv_hdr = (struct icmphdr *)buffer;

                struct iphdr *ip_header = (struct iphdr *) buffer;
                struct icmphdr *recv_hdr = (struct icmphdr *) (buffer + ip_header->ihl * 4);

                if (recv_hdr->type == ICMP_ECHOREPLY && recv_hdr->code == 0)
                {
		    // check control sum
		    
		    //unsigned short received_checksum = recv_hdr->checksum;
		    //recv_hdr->checksum = 0;
		    //unsigned short calculated_checksum = checksum(recv_hdr, sizeof(*recv_hdr) + (ip_header->tot_len - ip_header->ihl * 4));

		    //if (received_checksum != calculated_checksum)
        	    //{
            		//printf("Checksum mismatch: received %u, calculated %u\n", received_checksum, calculated_checksum);
            		//continue;
        	    //}
                    
                    printf("%d bytes from %s: icmp_seq = %d ttl = %d time = %.3Lf ms\n", PING_PKT_S, ping_ip, msg_count - 1, ttl_val, rtt_msec);
                    put_stats(rtt_msec, &stats);
                    stats.value[j] = rtt_msec;
                    msg_received_count++;
                    j++;
                }
                else    
                {
                    printf("statistic = %d\n", print_statistic);
                    print_statistic = 0;
                }
            }
        }       
	else
	    print_statistic = 0;
    }
    get_stddev(&stats, msg_count);
    printf("--- %s ping statistics ---\n", host);
    printf("%d packets transmitted, %d packets received, %.0f%% packet loss\n", msg_count, msg_received_count, ((float)(msg_count - msg_received_count) / msg_count) * 100);
    
    if (print_statistic)
	printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", stats.min, stats.avg, stats.max, stats.stddev);

}

int main(int argc, char *argv[])
{
    int sockfd;
    char *ip_addr;
    struct sockaddr_in addr_con;

    int verbose_mode = 0;
    int question_mode = 0;

    for (int i = 1; i < argc; i++)
    {
	if (strcmp(argv[i], "-v") == 0)
	    verbose_mode++;
	else if (strcmp(argv[i], "-?") == 0)
	    question_mode++;
    }

    if (question_mode > 0)
    {
	printf(question_msg);
	return 0;
    }

    pingloop = 1;

    if ((argc == 2 && verbose_mode > 0) || (argc > 2 && verbose_mode == 0) || argc > 3 || verbose_mode > 1)
    {
	fprintf(stderr, "ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n");
	exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++)
    	if (strcmp(argv[i], "-v") != 0)
    	    ip_addr = dns_lookup(argv[i], &addr_con);

    if (ip_addr == NULL)
    {
	fprintf(stderr, "ping: unknown host\n");
	exit(EXIT_FAILURE);
    }

    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        fprintf(stderr, "Socket file descriptor not received!\n");
	free(ip_addr);
	close(sockfd);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, intHandler); 
    
    send_ping(sockfd, &addr_con, ip_addr, argv[1]);
    close(sockfd);
    free(ip_addr);

    return 0;
}


//check control sum
//create verbose mode
