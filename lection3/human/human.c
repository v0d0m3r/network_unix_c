/*
 * TCP-клиент, который реализует понятие "Человек"
 *
 * Функции "Человека":
 * - общение с сервером "Астролог"/"Прокси"
 *
 * Формат общения с "Астрологом"/"Прокси":
 * - запрос к "Астрологу"/"Прокси": "HOROSCOPE ZZ\n"
 * где ZZ - знак зодиака;
 * длина строки запроса 22б (добивается пробелами)
 * - ответ от "Астролога"/"Прокси":
 * прогноз длиной 80б (включая '\n' в конце строки),
 * если нет гороскопа ответ: "SORRY\n"
 * - если в ответе придет "DENIED!\n", то
 * то выводим сообщение: "Ответ поддерживается только звездой\n"
 *
 * Неправильные ответы - закрытие соединения
*/

#include "../../my_lib.h"

//------------------------------------------------------------------------

void get_hs_query(int hs_id, char* hs_query)
{
    char tmp[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(tmp, COMM_ASTR_SZ+HS_NAME_SZ+1);

    static char* hs_tb[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo",
        "Virgo", "Libra", "Scorpio", "Sagittarius",
        "Capricorn", "Aquarius", "Pisces"
    };
}

//------------------------------------------------------------------------

int do_human(int sockfd)
{

    const unsigned short buff_sz = 20;
    char buff[buff_sz];
    memset(&buff, 0, buff_sz);
    int sz = 0;
    const unsigned int usec_sleep = 100000; // 100 мсек
    for (int i=0; i < 100; ++i) {
        sprintf(buff, "qwersss%d\n", i);
        // Отправляем пакет серверу
        sz = send(sockfd, &buff, buff_sz, MSG_NOSIGNAL);
        if (sz <= 0) {
            printf("sz: %d\n", sz);
            break;
        }
        memset(&buff, 0, buff_sz);
        usleep(usec_sleep);
    }
}

//------------------------------------------------------------------------

void work_client()
{
    // Создание сокета
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("Ошибка: Невозможно создать сокет\n");
        return;
    }

    int answ = set_sendrecv_timeout(cl_sockfd);
    if (answ) printf("Ошибка: Невозможно задать настройки сокету\n");

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
    if (inet_aton(serv_ip , &serv_addr.sin_addr) == 0) {
        printf("Ошибка: Некорректный ip-адрес: %s\n", strerror(errno));
        close(sockfd);
        return;
    }
    if(connect(sockfd, (struct sockaddr *)&serv_addr, serv_sz)<0)
        if(errno != EINPROGRESS) {
            close(sockfd);
            printf("Ошибка: %s\n", strerror(errno));
            return;
        }
    while (true) {
        answ = do_human();
        if (answ) break;
    }

    close(sockfd);
}

//------------------------------------------------------------------------

int main()
{
    work_client();
    return 0;
}

//------------------------------------------------------------------------