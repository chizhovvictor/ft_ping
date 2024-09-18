#include "../include/ft_ping.h"

void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_dom, char *ping_ip, char *rev_host)
{
    int ttl_val = 64;
    int msg_count = 0;
    int msg_received_count = 0;
    char buffer[128];
    struct timespec tfs, time_start, time_end, tfe;
    int flag;
    long double rtt_msec = 0, total_msec = 0;

    // используется для получения текущего значения времени на монотонных часах.
    clock_gettime(CLOCK_MONOTONIC, &tfs);

    /*
        Функция setsockopt используется для настройки параметров сокета. Она позволяет установить
        различные опции, которые определяют поведение сокета.
    */

    if (setsockopt(ping_sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
    {
        printf("\nSetting socket options to TTL failed!\n");
        return;
    }
    else
        printf("\nSocket set to TTL...\n");

    // структура для хранения времени отправки пакета
    struct timeval tv_out;
    tv_out.tv_sec = 1; // Устанавливаем таймаут в 1 секунду
    tv_out.tv_usec = 0;

    // Устанавливаем таймаут для получения пакета, чтобы программа не висла в ожидании, а возвращала ошибку
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv_out, sizeof tv_out);

    struct ping_pkt pckt;

    struct sockaddr_in r_addr; // Структура для хранения адреса получателя
    socklen_t addr_len = sizeof(r_addr);

    while (pingloop)
    {
        flag = 1;
        bzero(&pckt, sizeof(pckt)); // Заполняем пакет нулями
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid(); // Получаем идентификатор процесса (при отправке пакетов с разных процессов)

        int i;

        for (i = 0; i < sizeof(pckt.msg) - 1; i++)
            pckt.msg[i] = i + '0'; // Заполняем пакет данными

        pckt.msg[i] = 0;                                   // Завершаем строку нулем
        pckt.hdr.un.echo.sequence = msg_count++;           // Увеличиваем счетчик пакетов
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt)); // Вычисляем контрольную сумму пакета
        usleep(1000000);                                   // Задержка в 1 секунду перед отправкой следующего пакета чтобы не перегружать сеть

        clock_gettime(CLOCK_MONOTONIC, &time_start); // Получаем текущее время

        // Отправляем пакет
        if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr *)ping_addr, sizeof(*ping_addr)) <= 0)
        {
            printf("\nPacket Sending Failed!\n");
            flag = 0;
        }

        // Получаем пакет
        if (recvfrom(ping_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&r_addr, &addr_len) <= 0 && msg_count > 1)
        {
            printf("\nPacket receive failed!\n");
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_end);                                          // Получаем время получения пакета
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000.0; // Вычисляем время прохождения пакета
            rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000.0 + timeElapsed;            // Хранит Round-Trip Time (RTT), который является временем, необходимым для пакета, чтобы отправиться к целевому хосту и вернуться обратно

            // Если пакет получен
            if (flag)
            {
                // Получаем заголовок ICMP
                struct icmphdr *recv_hdr = (struct icmphdr *)buffer;
                if (!(recv_hdr->type == 0 && recv_hdr->code == 0))
                {
                    printf("Error... Packet received with ICMP type %d code %d\n", recv_hdr->type, recv_hdr->code);
                }
                else
                {
                    printf("%d bytes from %s (h: %s) (ip: %s) msg_seq = %d ttl = %d rtt = %Lf ms.\n", PING_PKT_S, ping_dom, rev_host, ping_ip, msg_count, ttl_val, rtt_msec);
                    msg_received_count++;
                }
            }
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec)) / 1000000.0;
    total_msec = (tfe.tv_sec - tfs.tv_sec) * 1000.0 + timeElapsed;

    printf("\n=== %s ping statistics ===\n", ping_ip);
    printf("%d packets sent, %d packets received, %f%% packet loss. Total time: %Lf ms.\n\n", msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100.0, total_msec);
}

int main(int argc, char *argv[])
{
    int sockfd;
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    char net_buf[NI_MAXHOST];

    if (argc != 2) {
        printf("\nFormat %s <address>\n", argv[0]);
        return 0;
    }

    ip_addr = dns_lookup(argv[1], &addr_con);
    if (ip_addr == NULL) {
        printf("\nDNS lookup failed! Could not resolve hostname!\n");
        return 0;
    }

    reverse_hostname = reverse_dns_lookup(ip_addr);
    printf("\nTrying to connect to '%s' IP: %s\n", argv[1], ip_addr);
    printf("\nReverse Lookup domain: %s\n", reverse_hostname);

    // Create a raw socket
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        printf("\nSocket file descriptor not received!\n");
        return 0;
    } else {
        printf("\nSocket file descriptor %d received\n", sockfd);
    }

    signal(SIGINT, intHandler); // Catching interrupt

    // Send pings continuously
    send_ping(sockfd, &addr_con, reverse_hostname, ip_addr, argv[1]);

    return 0;
}