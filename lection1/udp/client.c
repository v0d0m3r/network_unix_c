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

int get_port(unsigned short* portno, char msg[])
{
    static const int min_p = 1024;
    printf("%s", msg);
    scanf("%hu", portno);
    return (*portno < min_p) ? -1: 0;
}

//------------------------------------------------------------------------

void get_ip(char* ip_num, char msg[])
{
    printf("%s", msg);
    scanf("%s", ip_num);
}

//------------------------------------------------------------------------

void work_client()
{
    // Создание сокета
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0) {
        printf("Ошибка: Невозможно создать сокет\n");
        return;
    }

    // Структура адресов для сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    socklen_t serv_sz = sizeof(serv_addr);

    unsigned short portno = 0;
    if (get_port(&portno, "Введите номер порта:\n") < 0) {
        printf("Ошибка: Неправильный порт: %d\n", portno);
        close(sockfd);
        return;
    }

    const unsigned short serv_ip_sz = 17;
    char serv_ip[serv_ip_sz];
    memset(&serv_ip, 0, serv_ip_sz);
    get_ip(serv_ip, "Введите ip-номер сервера:\n");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    // Переводим ip-строку в структуру sin_addr
    if (inet_aton(serv_ip , &serv_addr.sin_addr) < 0) {
        printf("Ошибка: Некорректный ip-адрес: %s\n", strerror(errno));
        close(sockfd);
        return;
    }
    /*if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
        if(errno != EINPROGRESS) {
            close(sockfd);
            printf("\n Error : Connect Failed \n");
            return;
        }*/

    const unsigned short buff_sz = 20;
    char buff[buff_sz];
    memset(&buff, 0, buff_sz);
    int sz = 0;
    const unsigned int usec_sleep = 100000; // 100 мсек
    for (int i=0; i < 100; ++i) {
        sprintf(buff, "qwersss%d", i);
        // Отправляем пакет серверу
        sz = sendto(sockfd, &buff, buff_sz, MSG_NOSIGNAL,
                    (struct sockaddr*)&serv_addr,
                    serv_sz);
        if (sz < 0) {
            printf("sz: %d\n", sz);
            break;
        }
        memset(&buff, 0, buff_sz);
        usleep(usec_sleep);
    }
}

//------------------------------------------------------------------------

int main()
{
    work_client();
    return 0;
}

//------------------------------------------------------------------------
