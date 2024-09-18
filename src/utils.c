#include "../include/ft_ping.h"


/*

Контрольная сумма используется для проверки целостности данных при их передаче по сети. 
Перед отправкой пакета его данные передаются через эту функцию, и вычисленная 
контрольная сумма включается в заголовок пакета. Когда получатель получает этот пакет, 
он снова вычисляет контрольную сумму для полученных данных. Если обе контрольные суммы 
совпадают, значит, данные не были искажены при передаче.

Этот алгоритм используется, например, для протоколов ICMP (в утилитах ping), 
TCP и UDP для проверки целостности пакетов.

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
    printf("\nResolving DNS...\n");
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

// Функция для преобразования IP-адреса в доменное имя
char *reverse_dns_lookup(char *ip_addr) {
    struct sockaddr_in temp_addr;
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;

    // Инициализация структуры sockaddr_in для хранения IP-адреса
    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);

    // Выполнение обратного разрешения с помощью getnameinfo
    if (getnameinfo((struct sockaddr *)&temp_addr, len, buf, sizeof(buf), NULL, 0, NI_NAMEREQD)) {
        printf("Could not resolve reverse lookup of hostname\n");
        return NULL;
    }

    // Копирование результата в новый буфер и возвращение его
    ret_buf = (char *)malloc((strlen(buf) + 1) * sizeof(char));
    strcpy(ret_buf, buf);
    return ret_buf;
}
