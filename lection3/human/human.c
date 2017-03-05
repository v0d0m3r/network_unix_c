/*
 * TCP-клиент, который реализует понятие "Человек" и Звезда
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
 * если нет гороскопа ответ(8б): "SORRY\n"
 * - если в ответе придет "THANKS!\n",
 * то выводим сообщение - "DENIED!\n"
 *
 * Функции "Звезды":
 * - общение с сервером "Астролог"
 *
 * Формат общения с "Астрологом":
 * - запрос к "Астрологу": "STARS SAY ZZ\n"
 * (аналогично "Человеку")
 * - ответ от "Астролога"(8б):
 * "THANKS!\n", если придет "SORRY\n",
 * то выводим сообщение - "DENIED!\n"
 *
 * Неправильные ответы - закрытие соединения
*/

//------------------------------------------------------------------------

#include "../../my_lib.h"

//------------------------------------------------------------------------

void clear_input()
{
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

//------------------------------------------------------------------------

int get_int()
{
    int n = 0;
    while(true) {
        if (scanf("%d", &n) != 1) {
            printf("Это не число; попробуйте ещё раз\n");
            clear_input();  // Очищаем ввод
        }
    }
}

//------------------------------------------------------------------------
// get_hs_code() отвечает за консольный ввод код гороскопа
// Возвращаемые значения:
// - код гороскопа в диапазоне [0;11];
// - "-1" - код выхода
int get_hs_code(const char* client_name)
{
    const int low = 1;
    const int hight = 12;
    const int exit = -1;
    printf("***********************************\n"
           "%s: введите код гороскопа от %d до %d\n "
           "или %d для выхода:\n", client_name, low, hight, exit);
    int hs_code = 0;
    while(true) {
        hs_code = get_int();
        if (hs_code == exit) return exit;  // Код выхода
        if (hs_code < low || hs_code > hight)
            printf("Вне диапазона [%d, %d], "
                   "повторите ввод\n", low, hight);
    }
    // Приводим код к нашему диапазону: [0, 11]
    --hs_code;
    return hs_code;
}

//------------------------------------------------------------------------
// get_hs_query() формирует запрос к астрологу
int get_hs_query(int hs_code, char* hs_query)
{
    if (strlen(hs_query) != COMM_ASTR_SZ+HS_NAME_SZ) return -1;
    if (hs_code < 0 && hs_code > HS_COUNT-1) return -1;

    char tmp[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(tmp, COMM_ASTR_SZ+HS_NAME_SZ+1);

    const char hs_comm[] = "HOROSCOPE ";
    strncpy(tmp, hs_comm, strlen(hs_comm)); // Заполняем команду

    tmp[COMM_ASTR_SZ] = '\0';               // Урезаем strlen строки

    static const char* hs_tb[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo",
        "Virgo", "Libra", "Scorpio", "Sagittarius",
        "Capricorn", "Aquarius", "Pisces"
    };
    // Вставляем название гороскопа после команды
    strncat(tmp, hs_tb[hs_code], strlen(hs_tb[hs_code]));
    strncpy(hs_query, tmp, strlen(tmp));    // Формируем запрос
    return 0;
}

//------------------------------------------------------------------------
// is_protocol_answ() проверяет:
// входная строка - это ответ по протоколу?
bool is_protocol_answ(char* hs_data)
{
    if (!strncmp(hs_data, "SORRY! \n", PROTOCOL_ANSWER_SZ)) {
        printf("%s", hs_data);
        return true;
    }

    if (!strncmp(hs_data, "THANKS!\n", PROTOCOL_ANSWER_SZ)) {
        printf("%s", "DENIED!\n");
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
// get_hs_data() получает данные гороскопа от астролога
int get_hs_data(int sockfd)
{    
    char hs_data [HS_DATA_SZ+1];    // Строка данных гороскопа
    prepare_str(hs_data, HS_DATA_SZ+1);

    // Получаем часть гороскопа
    ssize_t sz = recv_all(sockfd, hs_data, PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        printf("Ошибка: приняты некорректные данные! "
               "Закрываем соединение!\n");
        return -1;
    }

    // Данные являются ответом по протоколу?
    if (is_protocol_answ(hs_data)) return 0;

    hs_data[PROTOCOL_ANSWER_SZ] = '\0';

    char tmp[HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ];
    prepare_str(tmp, HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ);

    // Дополучение данных гороскопа
    sz = recv_all(sockfd, tmp, HS_DATA_SZ-PROTOCOL_ANSWER_SZ);
    if (sz != (ssize_t)strlen(tmp)) {
        printf("Ошибка: приняты некорректные данные! "
               "Закрываем соединение!\n");
        return -1;
    }
    strncat(hs_data, tmp, strlen(tmp));
    printf(hs_data);
    return 0;
}

//------------------------------------------------------------------------
// do_human() отправляет "человеческий" запрос
// и получает данные гороскопа
int do_human(int sockfd, int hs_code)
{
    char hs_query[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(hs_query, COMM_ASTR_SZ+HS_NAME_SZ+1);
    hs_query[COMM_ASTR_SZ+HS_NAME_SZ-1] = '\n';
    // Формирование запроса гороскопа
    if (get_hs_query(hs_code, hs_query)) {
        printf("Ошибка: не удалось сформировать запрос!\n");
        return -1;
    }
    printf("%s", hs_query);
    // Запрос гороскопа
    ssize_t sz = send_all(sockfd, hs_query, COMM_ASTR_SZ+HS_NAME_SZ);
    if (sz != COMM_ASTR_SZ+HS_NAME_SZ) {
        printf("Ошибка: плохая отправка данных! "
               "Закрываем соединение!\n");
        return -1;
    }
    return get_hs_data(sockfd); // Получение данных гороскопа
}

//------------------------------------------------------------------------
// do_star() отправляет "звездный" запрос
// и отправляет данные гороскопа
int do_star(int sockfd, int hs_code)
{
    return 0;
}

//------------------------------------------------------------------------

int is_star(bool* star)
{
    /*const int low = 1;
    const int hight = 12;
    const int exit = -1;
    printf("Выберите режим\n "
           "или %d для выхода:\n", client_name, low, hight, exit);
    int hs_code = 0;
    while(true) {
        hs_code = get_int();
        if (hs_code == exit) return exit;  // Код выхода
        if (hs_code < low || hs_code > hight)
            printf("Вне диапазона [%d, %d], "
                   "повторите ввод\n", low, hight);
    }
    // Приводим код к нашему диапазону: [0, 11]
    --hs_code;
    return hs_code;*/
    return 0;
}

//------------------------------------------------------------------------
// work_client() обрабатывает соединение с астрологом
void work_client()
{
    // Создание сокета
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("Ошибка: невозможно создать сокет\n");
        return;
    }

    int answ = set_sendrecv_timeout(sockfd);
    if (answ) printf("Ошибка: невозможно задать настройки сокету\n");

    // Структура адресов для сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    socklen_t serv_sz = sizeof(serv_addr);

    unsigned short portno = 0;
    if (get_port(&portno, "Введите номер порта:\n") < 0) {
        printf("Ошибка: неправильный порт: %d\n", portno);
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
        printf("Ошибка: некорректный ip-адрес: %s\n",
               strerror(errno));
        close(sockfd);
        return;
    }
    if(connect(sockfd, (struct sockaddr *)&serv_addr, serv_sz)<0)
        if(errno != EINPROGRESS) {
            close(sockfd);
            printf("Ошибка: %s\n", strerror(errno));
            return;
        }

    int hs_code = 0;
    bool star = false;
    int exit = -1;
    while (true) {
        answ = is_star(&star);
        if (answ == exit) break;

        hs_code = (star) ? get_hs_code("Клиент-звезда")
                         : get_hs_code("Клиент-человек");
        if (hs_code == exit) break;    // Получен код завершения

        answ = (star) ? do_star (sockfd, hs_code)
                      : do_human(sockfd, hs_code);
        if (answ) break;
    }
    close(sockfd);
}

//------------------------------------------------------------------------

void test()
{
    int hs_code = 0;
    char hs_query[COMM_ASTR_SZ+HS_NAME_SZ+1];
    while (1) {
        prepare_str(hs_query, COMM_ASTR_SZ+HS_NAME_SZ+1);
        hs_query[COMM_ASTR_SZ+HS_NAME_SZ-1] = '\n';

        hs_code = get_hs_code("star");
        if (hs_code == -1) break;    // Получен код завершения
        if (hs_code < 0)
            printf("Ошибка: неправильный код гороскопа!\n");

        if (get_hs_query(hs_code, hs_query))
            printf("Ошибка: неправильный код гороскопа!\n");
        else
            printf("%s", hs_query);
    }
}

//------------------------------------------------------------------------

int main()
{
    work_client();
    return 0;
}

//------------------------------------------------------------------------
