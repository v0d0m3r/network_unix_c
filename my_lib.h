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
