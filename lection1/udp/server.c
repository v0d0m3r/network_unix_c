
//------------------------------------------------------------------------

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//------------------------------------------------------------------------
// Получаем значение порта в аргумент
// Возвращаемое значение: 0 успех, -1 неправильный порт
int get_port(unsigned short* portno)
{
    static const int min_p = 1024;
    printf("Введите номер порта:\n");
    scanf("%hu", portno);
    return (*portno < min_p) ? -1: 0;
}

//------------------------------------------------------------------------

void work_server()
{
    // Создание сокета (AF_INET - сетевой(интернет),
    // SOCK_DGRAM - датаграммный, 0 - по умолчанию UDP)
    int serv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(serv_sockfd < 0) {
        printf("Ошибка: Невозможно создать сокет\n");
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    socklen_t optlen = sizeof(timeout);
    // Меняем таймаут на прием пакетов
    if (setsockopt(serv_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (char *)&timeout, optlen) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");

    // Структура адресов для сокета сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    unsigned short portno = 0;
    if (get_port(&portno) < 0) {
        printf("Ошибка: Неправильный порт: %d\n", portno);
        close(serv_sockfd);
        return;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    // Структура адресов для сокета клиента
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_sz = sizeof(client_addr);

    const unsigned short buff_sz = 20;  // Буфер входной информации
    char buff[buff_sz];
    memset(&buff, 0, buff_sz);

    // Задаем локальный адрес сокету
    bind(serv_sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    int sz = 0;
    while(1) {
        // Получаем данные от клиента
        sz = recvfrom(serv_sockfd, &buff, buff_sz, MSG_NOSIGNAL,
                      (struct sockaddr*)&client_addr,
                      &client_sz);
        if (sz < 0) {   // Данные закончились - выходим
            printf("sz: %d\n", sz);
            break;
        }
        printf("Данные: %s\n", buff);
        printf("от ip: %s, порт номер: %d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        memset(&buff, 0, buff_sz);
    }
    close(serv_sockfd);
}

//------------------------------------------------------------------------

int main()
{
    work_server();
    return 0;
}

//------------------------------------------------------------------------
