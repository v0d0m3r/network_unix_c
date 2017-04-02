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

#define COMM_ASTR_SZ        10
#define HS_NAME_SZ          12
#define HS_DATA_SZ          80
#define HS_COUNT            12
#define PROTOCOL_ANSWER_SZ  8

//------------------------------------------------------------------------
// Horoscope_str хранит гороскопы в формате:
// hs_name_1hs_data_1...hs_name_nhs_data_n
typedef struct {
    size_t size;  // Рамер строки
    char* buf;    // Указатель на строку c гороскопами
} Horoscope_str;

//------------------------------------------------------------------------
// Коды команд к астрологу
enum Code_comm_astr
{
    stars_say, horoscope
};

//------------------------------------------------------------------------

int get_code_comm_astr(const char* comm_astr);

//------------------------------------------------------------------------
// Получаем значение порта в аргумент
// Возвращаемое значение: 0 успех, -1 неправильный порт
int get_port(unsigned short* portno, char msg[]);

//------------------------------------------------------------------------
// Простое получение ip из строки, проверка в вызывающем коде
void get_ip(char* ip_num, char msg[]);

//------------------------------------------------------------------------
// Чтение данных нужной длины из сокета
ssize_t recv_all(int fd, char* buf, size_t len);

//------------------------------------------------------------------------
// Отправка данных нужной длины из сокета
ssize_t send_all(int fd, char* buf, size_t len);

//------------------------------------------------------------------------
// Изменить сокету таймаут на прием/передачу данных
int set_sendrecv_timeout(int cl_sockfd);

//------------------------------------------------------------------------
// Инициализировать строку в формате:
// "space1space2space3...spaceN-2\0"
int prepare_str(char* str, size_t str_sz);

//------------------------------------------------------------------------

int randint(int max);

//------------------------------------------------------------------------

void clear_input();

//------------------------------------------------------------------------
// true, если входная строка является
// именем одного из гороскопов
bool is_horoscope_name(const char* name);

//------------------------------------------------------------------------

void merror(const char* title, const char* err);

//------------------------------------------------------------------------
