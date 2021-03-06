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
 * - прогноз длиной 80б (включая '\n' в конце строки)
 * - ответ от "Астролога"(8б):
 * "THANKS!\n", если придет "SORRY\n",
 * то выводим сообщение - "DENIED!\n"
 *
 * Неправильные ответы - закрытие соединения
*/

//------------------------------------------------------------------------

#include "../../my_lib.h"

//------------------------------------------------------------------------

int get_int()
{
    int n = 0;
    while (true) {
        if (scanf("%d", &n) == 1) break;

        printf("Это не число; попробуйте ещё раз\n");
        clear_input();  // Очищаем ввод
    }
    return n;
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
    printf("%s: введите код гороскопа от %d до %d\n "
           "или %d для выхода:\n", client_name, low, hight, exit);
    int hs_code = 0;
    while (true) {
        hs_code = get_int();
        if (hs_code == exit) return exit;  // Код выхода
        if (hs_code < low || hs_code > hight)
            printf("Вне диапазона [%d, %d], "
                   "повторите ввод\n", low, hight);
        else break;
    }
    // Приводим код к нашему диапазону: [0, 11]
    --hs_code;
    return hs_code;
}

//------------------------------------------------------------------------

int get_hs_query(bool star, int hs_code, char* hs_query)
{
    if (strlen(hs_query) != COMM_ASTR_SZ+HS_NAME_SZ) return -1;
    if (hs_code < 0 && hs_code > HS_COUNT-1) return -1;

    char tmp[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(tmp, COMM_ASTR_SZ+HS_NAME_SZ+1);

    char hs_comm[COMM_ASTR_SZ+1];
    prepare_str(hs_comm, COMM_ASTR_SZ+1);
    (star) ? sprintf(hs_comm, "STARS SAY ")
           : sprintf(hs_comm, "HOROSCOPE ");

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
bool is_protocol_answ(const char* hs_data, bool star)
{
    if (!strncmp(hs_data, "SORRY! \n", PROTOCOL_ANSWER_SZ)) {
        (star) ? printf("%s", "DENIED!\n")
               : printf("%s", hs_data);
        return true;
    }

    if (!strncmp(hs_data, "THANKS!\n", PROTOCOL_ANSWER_SZ)) {        
        (star) ? printf("%s", hs_data)
               : printf("%s", "DENIED!\n");
        return true;
    }

    return false;
}

//------------------------------------------------------------------------
// do_human() получает данные от астролога
int do_human(int sockfd)
{    
    char hs_data[HS_DATA_SZ+1];    // Строка данных гороскопа
    prepare_str(hs_data, HS_DATA_SZ+1);

    // Получаем часть гороскопа
    ssize_t sz = recv_all(sockfd, hs_data, PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        printf("Ошибка: приняты некорректные данные! "
               "Закрываем соединение!\n");
        return -1;
    }

    // Данные являются ответом по протоколу?
    if (is_protocol_answ(hs_data, false)) return 0;

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

int get_hs_data(char* dest_hs_data)
{
    if (strlen(dest_hs_data) != HS_DATA_SZ) return -1;

    static const int hs_data_tb_sz = 4;
    static const char* hs_data_tb[] = {
        "don't be fooled by others who aren't "
        "straight with you. If you don't know the  \n",

        "One day, I want to be. When you were little, "
        "how did you finish that sentence  \n",

        "If not, consider what you could do to change "
        "it. Maybe you should lie on your  \n"

        //"bad horoscope\n",
    };

    static time_t t;
    srand((unsigned) time(&t));

    int n = randint(hs_data_tb_sz-1);
    strncpy(dest_hs_data, hs_data_tb[n], HS_DATA_SZ);    

    return 0;
}

//------------------------------------------------------------------------
// do_stars() отдает данные астрологу
int do_stars(int sockfd)
{
    char hs_data[HS_DATA_SZ+1];    // Строка данных гороскопа
    prepare_str(hs_data, HS_DATA_SZ+1);

    if(get_hs_data(hs_data)) {
        printf("Ошибка: get_hs_data(): некорректный "
               "гороскоп!\n");
        return -1;
    }

    // Отправляем данные гороскопа
    ssize_t sz = send_all(sockfd, hs_data, HS_DATA_SZ);
    if (sz != HS_DATA_SZ) {
        printf("Ошибка: приняты некорректные данные! "
               "Закрываем соединение!\n");
        return -1;
    }

    char answ[PROTOCOL_ANSWER_SZ+1];
    prepare_str(answ, PROTOCOL_ANSWER_SZ+1);

    // Получаем ответ от астролога/прокси
    sz = recv_all(sockfd, answ, PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        printf("Ошибка: приняты некорректные данные! "
               "Закрываем соединение!\n");
        return -1;
    }

    if (!is_protocol_answ(answ, true)) return -1;
    return 0;
}

//------------------------------------------------------------------------
// process_horoscope() отправляет человеческий(звездный)
// запрос и получает(отправляет) данные гороскопа
int process_horoscope(bool star, int sockfd, int hs_code)
{
    char hs_query[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(hs_query, COMM_ASTR_SZ+HS_NAME_SZ+1);
    hs_query[COMM_ASTR_SZ+HS_NAME_SZ-1] = '\n';

    // Формирование запроса
    if (get_hs_query(star, hs_code, hs_query)) {
        printf("Ошибка: не удалось сформировать запрос!\n");
        return -1;
    }
    printf("%s", hs_query);
    // Запрос астрологу/прокси
    ssize_t sz = send_all(sockfd, hs_query, COMM_ASTR_SZ+HS_NAME_SZ);
    if (sz != COMM_ASTR_SZ+HS_NAME_SZ) {
        printf("Ошибка: плохая отправка данных! "
               "Закрываем соединение!\n");
        return -1;
    }
    return (star) ? do_stars(sockfd)
                  : do_human(sockfd);
}

//------------------------------------------------------------------------

enum Menu_code {
   mc_exit = -1, mc_star_code = 1, mc_human_code = 2
};

//------------------------------------------------------------------------

int is_star(bool* star)
{    
    printf("***********************************\n"
           "Введите номер пункта меню:\n"
           " %d - режим Звезды\n"
           " %d - режим Человека\n"
           "%d  - выход\n"
           "***********************************\n",
           mc_star_code, mc_human_code, mc_exit);
    int menu_code = 0;
    while (true) {
        menu_code = get_int();
        switch (menu_code) {
        case mc_star_code:
            *star = true;
            return 0;
        case mc_human_code:
            *star = false;
            return 0;
        case mc_exit:
            return mc_exit;  // Код выхода
        default:
            printf("Неправильный пункт меню, "
                   "повторите ввод\n");
            break;
        }
    }   
    return 0;
}

//------------------------------------------------------------------------
// work_client() обрабатывает соединение с астрологом/прокси
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

        answ = process_horoscope(star, sockfd, hs_code);
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

        if (get_hs_query(false, hs_code, hs_query))
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
