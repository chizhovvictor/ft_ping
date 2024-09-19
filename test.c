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


#define PING_PKT_S 64
#define PORT_NO 0
int pingloop = 1;

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

struct ping_stats {
    double min;
    double avg;
    double max;
    double stddev;
    double value[];
};

struct ping_pkt {
    struct icmphdr hdr;  // Заголовок ICMP-пакета
    char msg[PING_PKT_S - sizeof(struct icmphdr)]; // Данные пакета
};




unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;      // Преобразуем указатель на данные в указатель на массив 16-битных чисел (unsigned short)
    unsigned int sum = 0;         // Инициализируем переменную для хранения суммы
    unsigned short result;        // Переменная для результата контрольной суммы

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;            // Проходим по буферу и добавляем 16-битные слова к сумме
    if (len == 1)                 // Если остался один байт (неполное 16-битное слово)
        sum += *(unsigned char *)buf;  // Добавляем этот байт к сумме

    // Теперь обрабатываем переносы битов (если сумма превышает 16 бит)
    sum = (sum >> 16) + (sum & 0xFFFF);  // Складываем старшие 16 бит с младшими
    sum += (sum >> 16);                  // Повторяем, если снова произошло переполнение

    result = ~sum;                       // Инвертируем биты суммы (получаем контрольную сумму)
    return result;                       // Возвращаем контрольную сумму
}



// Функция для преобразования доменного имени в IP-адрес

char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con) {
    struct hostent *host_entity;
    char *ip = (char *)malloc(NI_MAXHOST * sizeof(char));

    if ((host_entity = gethostbyname(addr_host)) == NULL) {
        return NULL;
    }

    strcpy(ip, inet_ntoa(*(struct in_addr *)host_entity->h_addr));
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = htons(PORT_NO);
    (*addr_con).sin_addr.s_addr = *(long *)host_entity->h_addr;

    return ip;
}

// функция для прерывания бесконечного цикла пинга по нажатию Ctrl+C
void intHandler(int dummy) { 
    pingloop = 0; 
}

// Функция для вычисления статистики
void put_stats(long double time, struct ping_stats *ping_stat) {
    if (ping_stat->min > time)
        ping_stat->min = time;
    if (ping_stat->max < time)
        ping_stat->max = time;
    ping_stat->avg += time;



    
}

// Функция для вычисления стандартного отклонения
void get_stddev(struct ping_stats *ping_stat, int count) {
    ping_stat->avg = ping_stat->avg / count;

    for (int i = 0; i < count; i++) {
        ping_stat->stddev += pow(ping_stat->value[i] - ping_stat->avg, 2);
    }

    ping_stat->stddev = sqrt(ping_stat->stddev / count);

}

void send_ping(int ping_sockfd, struct sockaddr_in *ping_addr, char *ping_ip, char *rev_host)
{
    int ttl_val = 64;
    int msg_count = 0;
    int msg_received_count = 0;
    char buffer[128];
    struct timespec time_start, time_end;
    int flag;
    long double rtt_msec = 0;

    /*
        Функция setsockopt используется для настройки параметров сокета. Она позволяет установить
        различные опции, которые определяют поведение сокета.
    */

    if (setsockopt(ping_sockfd, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0)
    {
        fprintf(stderr, "Setting socket options to TTL failed!\n");
        return;
    }


    // структура для хранения времени отправки пакета
    struct timeval tv_out;
    tv_out.tv_sec = 1; // Устанавливаем таймаут в 1 секунду
    tv_out.tv_usec = 0;

    // Устанавливаем таймаут для получения пакета, чтобы программа не висла в ожидании, а возвращала ошибку
    setsockopt(ping_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv_out, sizeof tv_out);

    struct ping_pkt pckt;
    struct ping_stats stats;

    stats.min = 100000;
    stats.max = 0;
    stats.avg = 0;
    stats.stddev = 0;

    struct sockaddr_in r_addr; // Структура для хранения адреса получателя
    socklen_t addr_len = sizeof(r_addr);

    int j = 0;
    while (pingloop)
    {
        flag = 1;
        bzero(&pckt, sizeof(pckt)); // Заполняем пакет нулями
        pckt.hdr.type = ICMP_ECHO;
        pckt.hdr.un.echo.id = getpid(); // Получаем идентификатор процесса (при отправке пакетов с разных процессов)

        int i;

        for (i = 0; i < sizeof(pckt.msg) - 1; i++)
            pckt.msg[i] = i + '0'; // Заполняем пакет данными

        pckt.msg[i] = 0;                                   // Завершаем строку нулем           // Увеличиваем счетчик пакетов
        pckt.hdr.checksum = checksum(&pckt, sizeof(pckt)); // Вычисляем контрольную сумму пакета
        usleep(1000000);                                   // Задержка в 1 секунду перед отправкой следующего пакета чтобы не перегружать сеть

        clock_gettime(CLOCK_MONOTONIC, &time_start); // Получаем текущее время

        // Отправляем пакет
        if (sendto(ping_sockfd, &pckt, sizeof(pckt), 0, (struct sockaddr *)ping_addr, sizeof(*ping_addr)) <= 0)
        {
            fprintf(stderr, "ping: sendto: No route to host\n");
            flag = 0;
        }

        if (pingloop == 0)
            break;
        pckt.hdr.un.echo.sequence = msg_count++;
        if (recvfrom(ping_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&r_addr, &addr_len) <= 0 && msg_count > 1)
        {
            fprintf(stderr, "Request timeout for icmp_seq %d\n", msg_count);
        }
        else
        {
            clock_gettime(CLOCK_MONOTONIC, &time_end);                                          // Получаем время получения пакета
            double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec)) / 1000000; // Вычисляем время прохождения пакета
            rtt_msec = (time_end.tv_sec - time_start.tv_sec) * 1000 + timeElapsed;            // Хранит Round-Trip Time (RTT), который является временем, необходимым для пакета, чтобы отправиться к целевому хосту и вернуться обратно

            // Если пакет получен
            if (flag)
            {
                // Получаем заголовок ICMP
                struct icmphdr *recv_hdr = (struct icmphdr *)buffer;
                if (recv_hdr->type == 0 && recv_hdr->code == 0)
                {
                    printf("Error... Packet received with ICMP type %d code %d\n", recv_hdr->type, recv_hdr->code);
                }
                else
                {
                    printf("%d bytes from %s: icmp_seq = %d ttl = %d time = %.3Lf ms\n", PING_PKT_S, ping_ip, msg_count, ttl_val, rtt_msec);
                    put_stats(rtt_msec, &stats);
                    stats.value[j] = rtt_msec;
                    msg_received_count++;
                    j++;
                }
            }
        }
    }
    get_stddev(&stats, msg_count);
    printf("\n--- %s ping statistics ---\n", ping_ip);
    printf("%d packets transmitted, %d packets received, %.1f%% packet loss\n", msg_count, msg_received_count, ((msg_count - msg_received_count) / (double)msg_count) * 100);
    printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", stats.min, stats.avg, stats.max, stats.stddev);
    close(ping_sockfd);

}

int main(int argc, char *argv[])
{
    int sockfd;
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    char net_buf[NI_MAXHOST];

    if (argc != 2)
    {
        fprintf(stderr, "%s\n",  error_message);
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

    return 0;
}

// разобраться с таймом
// разобраться как сделать чтобы при нажатии Ctrl+C не отправлялся пакет или не учитывался
// разобраться с таймаутом
// разобраться с выводом ошибок
// разобраться с выводом статистики

