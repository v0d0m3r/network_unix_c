//------------------------------------------------------------------------

#include "my_lib.h"

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
// Получить код входной команды
int get_code_comm_astr(const char* comm_astr)
{
    if (strlen(comm_astr) != COMM_ASTR_SZ) return -1;

    static char* comm_astr_tb[] = {
        "STARS SAY ", "HOROSCOPE "
    };

    if (!strncmp(comm_astr, comm_astr_tb[stars_say], COMM_ASTR_SZ))
        return stars_say;

    if (!strncmp(comm_astr, comm_astr_tb[horoscope], COMM_ASTR_SZ))
        return horoscope;

    return -2;
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
// Изменить сокету таймаут на прием/передачу данных
int set_sendrecv_timeout(int cl_sockfd)
{
    struct timeval timeout;
    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    socklen_t optlen = sizeof(timeout);

    if (setsockopt(cl_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (char*)&timeout, optlen) < 0)
        return -1;

    if (setsockopt(cl_sockfd, SOL_SOCKET, SO_SNDTIMEO,
                   (char*)&timeout, optlen) < 0)
        return -1;

    return 0;
}

//------------------------------------------------------------------------
// Инициализировать строку в формате:
// "space1space2space3...spaceN-2\0"
int prepare_str(char* str, size_t str_sz)
{
    if (!str_sz) return -1;
    memset(str, ' ', str_sz);
    str[str_sz-1] = '\0';
    return 0;
}

//------------------------------------------------------------------------

int randint(int max) { return rand()%max; }

//------------------------------------------------------------------------

void clear_input()
{
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

//------------------------------------------------------------------------

bool is_horoscope_name(const char* name)
{
    static const size_t hs_tb_sz = 12;   // Количество имен гороскопов
    static char* hs_tb[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo",
        "Virgo", "Libra", "Scorpio", "Sagittarius",
        "Capricorn", "Aquarius", "Pisces"
    };

    if (strlen(name) != HS_NAME_SZ) {   // Для имени нефиксированного
                                        // размера
        for (size_t i=0; i < hs_tb_sz; ++i)
            if (!strncmp(hs_tb[i], name, strlen(name)))
                return true;
        return false;
    }

    char tmp_str[HS_NAME_SZ+1];

    for (size_t i=0; i < hs_tb_sz; ++i) {
        prepare_str(tmp_str, HS_NAME_SZ+1);
        tmp_str[HS_NAME_SZ-1] = '\n';

        strncpy(tmp_str, hs_tb[i], strlen(hs_tb[i]));
        if (!strncmp(tmp_str, name, HS_NAME_SZ))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------

void merror(const char* title, const char* err)
{
    printf("error: %s: %s\n", title, err);
}

//------------------------------------------------------------------------
