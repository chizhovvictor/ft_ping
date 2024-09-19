#include "../include/ft_ping.h"

/*

Контрольная сумма используется для проверки целостности данных при их передаче по сети. 
Перед отправкой пакета его данные передаются через эту функцию, и вычисленная 
контрольная сумма включается в заголовок пакета. Когда получатель получает этот пакет, 
он снова вычисляет контрольную сумму для полученных данных. Если обе контрольные суммы 
совпадают, значит, данные не были искажены при передаче.

*/
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
void intHandler() { 
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
