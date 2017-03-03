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

void clear_input()
{
    char ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

//------------------------------------------------------------------------

int get_hs_code()
{
    printf("***********************************\n"
           "Введите номер гороскопа клиента "
           "от 1 до 12\n или -1 для выхода.\n");
    static const int hs_min_num = 1;
    int hs_code = 0;

    if (scanf("%d", &hs_code) != 1) { // Очищаем ввод
        clear_input();
        return -2;
    }

    if (hs_code == -1) return hs_code; // Выход
    if (hs_code < hs_min_num || hs_code > HS_COUNT) return -2;

    --hs_code;  // Приводим код к нашему формату: [0, 11]
    return hs_code;
}

//------------------------------------------------------------------------

int get_hs_query(int hs_code, char* hs_query)
{
    if (strlen(hs_query) != COMM_ASTR_SZ+HS_NAME_SZ) return -1;
    if (hs_code < 0 && hs_code > HS_COUNT-1) return -1;

    char tmp[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(tmp, COMM_ASTR_SZ+HS_NAME_SZ+1);

    static const char hs_comm[] = "HOROSCOPE ";
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

bool is_protocol_answ(char* hs_data)
{
    static const size_t answ_tb_sz = 3;
    static const char* answ_tb[] = {
        "SORRY! \n", "THANKS!\n", "DENIED!\n"
    };

    for (size_t i=0; i < answ_tb_sz; ++i) {
        if (!strncmp(hs_data, answ_tb[i], strlen(answ_tb[i]))) {
            printf("%s", answ_tb[i]);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------

int get_hs_data(int sockfd)
{
    // Ответ - данные
    char hs_data [HS_DATA_SZ+1];
    prepare_str(hs_data, HS_DATA_SZ+1);

    ssize_t sz = recv_all(sockfd, hs_data, PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        printf("Плохая отправка данных, "
               "закрываем соединение!\n");
        return -1;
    }

    if (is_protocol_answ(hs_data)) return 0;

    hs_data[PROTOCOL_ANSWER_SZ] = '\0';

    char tmp[HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ];
    prepare_str(tmp, HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ);

    sz = recv_all(sockfd, tmp, HS_DATA_SZ-PROTOCOL_ANSWER_SZ);
    if (sz != (ssize_t)strlen(tmp)) {
        printf("Плохая отправка данных, "
               "закрываем соединение!\n");
        return -1;
    }
    strncat(hs_data, tmp, strlen(tmp));
    printf("%d\n", strlen(hs_data));
    printf(hs_data);

    return 0;
}

//------------------------------------------------------------------------

int do_human(int sockfd, int hs_code)
{
    char hs_query[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(hs_query, COMM_ASTR_SZ+HS_NAME_SZ+1);
    hs_query[COMM_ASTR_SZ+HS_NAME_SZ-1] = '\n';

    if (get_hs_query(hs_code, hs_query)) {
        printf("Ошибка: не удалось сформировать запрос!\n");
        return -1;
    }
    printf("%s", hs_query);
    // Запрос гороскопа
    ssize_t sz = send_all(sockfd, hs_query, COMM_ASTR_SZ+HS_NAME_SZ);
    if (sz != COMM_ASTR_SZ+HS_NAME_SZ) {
        printf("Плохая отправка данных\n");
        return -1;
    }
    return get_hs_data(sockfd);
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

    int answ = set_sendrecv_timeout(sockfd);
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

    int hs_code = 0;
    while (true) {
        hs_code = get_hs_code();
        if (hs_code == -1) break;    // Получен код завершения
        if (hs_code < 0)
            printf("Ошибка: неправильный код гороскопа!\n");
        else {
            answ = do_human(sockfd, hs_code);
            if (answ) break;
        }
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

        hs_code = get_hs_code();
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
