/*
 * TCP-сервер, который реализует понятие "Астролог"
 *
 * Функции "Астролога":
 * - общение с клиентом "Звезда"
 * - общение с клиентом "Человек"
 *
 * Формат общения с "Звездой":
 * - запрос от "Звезды": "STARS SAY ZZ\n",
 * где ZZ - знак зодиака, длина строки 22б (добивается пробелами)
 * После заголовка запроса: прогноз длиной 80б (включая '\n')
 * - ответ к "Звезде": "THANKS\n"
 *
 * Формат общения с "Человеком":
 * - запрос от "Человека": "HOROSCOPE ZZ\n" (аналогично "Звезде")
 * - ответ "Человеку": прогноз длиной 80б (включая '\n')
 *
 * Ответ на неправильные запросы - закрытие соединение
*/

#include "../../my_lib.h"

//------------------------------------------------------------------------

typedef struct {
    size_t size;  // Рамер строки
    char* buff;   // Указатель на строку
} Horoscope;

//------------------------------------------------------------------------

int add_horoscope(Horoscope *hs, char* name_hor, char* source)
{
    size_t tmp_sz = hs->size + strlen(name_hor) + strlen(source);
    char* tmp_str = (char*)malloc(tmp_sz*sizeof(char));
    if (tmp_str == NULL) return -1;

    size_t start = 0;
    for (size_t i = start; i < hs->size; ++i )
        tmp_str[i] = hs->buff[i];
    free(hs->buff);

    start = hs->size;
    for (size_t i = start; i < (start + strlen(name_hor)); ++i)
        tmp_str[i] = name_hor[i];

    start = hs->size + strlen(name_hor);
    for (size_t i = start; i < (start + strlen(source)); ++i)
        tmp_str[i] = name_hor[i];

    hs->buff = tmp_str;
    hs->size = tmp_sz;
    free(tmp_str);
    return 0;
}

//------------------------------------------------------------------------
// do_client() обрабатывает соединение с сокетом клиента
void do_client(int* cl_sockfd, struct sockaddr_in* client_addr)
{
    const unsigned short buff_sz = 20;  // Буфер входной информации
    char buff[buff_sz];
    memset(&buff, 0, buff_sz);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    socklen_t optlen = sizeof(timeout);
    // Меняем таймаут на прием пакетов
    if (setsockopt(*cl_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (char *)&timeout, optlen) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");

    int sz = 0;
    while(1) {          // Получаем данные от клиента
        sz = recv(*cl_sockfd, &buff, buff_sz, MSG_NOSIGNAL);
        if (sz <= 0) {  // Данные закончились - выходим
            printf("sz: %d\n", sz);
            break;
        }
        printf("Данные: %s", buff);
        printf("от ip: %s, порт номер: %d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
    }
}

//------------------------------------------------------------------------

void work_server()
{
    // Создание сокета (AF_INET - сетевой(интернет),
    // SOCK_STREAM - потоковый сокет, 0 - по умолчанию TCP)
    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lsockfd < 0) {
        printf("Ошибка: Невозможно создать сокет\n");
        return;
    }
    // Структура адресов для сокета сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    unsigned short portno = 0;
    if (get_port(&portno, "Введите ip сервера:\n") < 0) {
        printf("Ошибка: Неправильный порт: %d\n", portno);
        close(lsockfd);
        return;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Структура адресов для сокета клиента
    struct sockaddr_in client_addr;
    socklen_t client_sz = sizeof(client_addr);
    memset(&client_addr, 0, client_sz);

    // Задаем локальный адрес сокету
    bind(lsockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(lsockfd, 5); // Делаем сокет слушащим
    int cl_sockfd = 0;
    while(1) {          // Обработка соеднений
                        // Получаем сокет нового клиента
        cl_sockfd = accept(lsockfd, (struct sockaddr*)&client_addr,
                           &client_sz);
        if (cl_sockfd < 0) {
            printf("Ошибка: %s\n", strerror(errno));
            break;
        }
        do_client(&cl_sockfd, &client_addr); // Работаем с клиентом

        close(cl_sockfd);                   // Закрываем сокет клиента
        memset(&client_addr, 0, client_sz); // Обнуляем память
                                            // адресов клиента
    }
    close(lsockfd);
}

//------------------------------------------------------------------------

int add_space(char* str, size_t startpos, size_t finishpos)
{
    if (startpos >= finishpos) return -1;
    if (strlen(str) <= finishpos) return -1;

    for (size_t i = startpos; i < finishpos; ++i)
        str[i] = ' ';
    str[finishpos] = '\n';
    return 0;
}

//------------------------------------------------------------------------

bool is_hrscp_nm(char* name)
{
    static const size_t hrscp_tb_sz = 12;
    static char* hrscp_tb[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo",
        "Virgo", "Libra", "Scorpio", "Sagittarius",
        "Capricorn", "Aquarius", "Pisces"
    };
    char tmp_str[hrscp_tb_sz];
    int res = 0;
    for (size_t i=0; i < hrscp_tb_sz; ++i) {
        strncpy(tmp_str, hrscp_tb[i], strlen(hrscp_tb[i]));
        res = add_space(tmp_str, strlen(hrscp_tb[i]), hrscp_tb_sz-1);
        if (res) printf("Ошибка: не удалось добавить пробелы");
        if (!strncmp(tmp_str, name, strlen(tmp_str)))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------

int main()
{
    //work_server();
    Horoscope hs;
    hs.size = 0;

    char name_hor[] = "Aries      \n";
    printf("Name sz: %u\n", strlen(name_hor));

    if (is_hrscp_nm(name_hor))
        printf("yeah!\n");


    return 0;
}

//------------------------------------------------------------------------
