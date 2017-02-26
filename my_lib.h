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
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <sys/prctl.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//------------------------------------------------------------------------
// Получаем значение порта в аргумент
// Возвращаемое значение: 0 успех, -1 неправильный порт
int get_port(unsigned short* portno, char msg[])
{
    static const int min_p = 1024;
    printf("%s", msg);
    scanf("%hu", portno);
    return (*portno < min_p) ? -1: 0;
}

//------------------------------------------------------------------------
// Простое получение ip из строки, проверка в вызывающем коде
void get_ip(char* ip_num, char msg[])
{
    printf("%s", msg);
    scanf("%s", ip_num);
}

//------------------------------------------------------------------------
// Чтение данных нужной длины из сокета
ssize_t recv_all(int fd, char* buf, size_t len)
{
    ssize_t received = 0;
    ssize_t rem = len;
    while (len) {
        received = recv(fd, buf, len, 0);
        if (received == -1) return -1;
        if (received == 0) break;
        buf += received;
        len -= received;
    }
    return rem - len;
}

//------------------------------------------------------------------------
// Отправка данных нужной длины из сокета
ssize_t send_all(int fd, char* buf, size_t len)
{
    ssize_t sended = 0;
    ssize_t rem = len;
    while (len) {
        sended = send(fd, buf, len, 0);
        if (sended == -1) return -1;
        buf += sended;
        len -= sended;
    }
    return rem - len;
}

//------------------------------------------------------------------------
