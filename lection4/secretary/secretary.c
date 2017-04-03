/*
 * Реализована модель fork-per-requrest
 *
 * TCP-сервер для "Человека" и -клиент для "Астролога",
 * реализует понятие "Секретарь" (Прокси)
 *
 * Функции "Секретаря":
 * - общение с сервером "Астролог"
 * - общение с клиентом "Человек"
 *
 * Формат общения с "Астрологом":
 * - передача "Человеческого запроса" к "Астрологу":
 * "HOROSCOPE ZZ\n", где ZZ - знак зодиака;
 * длина строки запроса 22б (добивается пробелами)
 * - ответ от "Астролога":
 * прогноз длиной 80б (включая '\n' в конце строки),
 * если нет гороскопа ответ(8б): "SORRY\n"
 * - если в ответе придет "THANKS!\n",
 * то выводим сообщение - "DENIED!\n"
 *
 * Формат общения с "Человеком":
 * - получение "Человеческого" запроса
 * - отправка ответа от "Астролога"
 *
 * Если придет запрос от "Звезды",
 * то выводим сообщение - "DENIED!\n" и закрываем содинение
 *
 * Неправильные запросы и ответы - закрытие соединения
*/

//------------------------------------------------------------------------

#include "../../my_lib.h"

//------------------------------------------------------------------------
// Получить человеческий запрос
int recv_hquery(int hum_sockfd, char* comm_astr,
                char* hs_name)
{
    char title[] = "recv_hquery";
    if (       strlen(comm_astr) != COMM_ASTR_SZ
            || strlen(hs_name) != HS_NAME_SZ) {
        merror(title, "Некорректные аргументы");
        return -1;
    }

    ssize_t sz = 0;
    sz = recv_all(hum_sockfd, comm_astr, COMM_ASTR_SZ);
    if (sz != COMM_ASTR_SZ) {
        merror(title, "Нет данных необходимой длины");
        return -1;
    }
    printf("comm_astr: %s\n", comm_astr);

    int code_comm_astr = 0;
    code_comm_astr = get_code_comm_astr(comm_astr);
    switch (code_comm_astr) {
    case stars_say:      
        merror(title, "DENIED!");
        return -1;
    case horoscope:
        break;
    default:
        merror(title, "Неизвестная команда");
        return -1;
    }

    sz = recv_all(hum_sockfd, hs_name, HS_NAME_SZ);
    if (sz != HS_NAME_SZ || !is_horoscope_name(hs_name)) {
        merror(title, "Некорректное название гороскопа");
        return -1;
    }
    printf("hs_name: %s", hs_name);

    return 0;
}

//------------------------------------------------------------------------
// Передать человеческий запрос астрологу
int send_hquery_to_astr(int astr_sockfd, const char* comm_astr,
                        const char* hs_name)
{
    char title[] = "send_hquery_to_astr";
    if (       strlen(comm_astr) != COMM_ASTR_SZ
            || strlen(hs_name) != HS_NAME_SZ) {
        merror(title, "Некорректные аргументы");
        return -1;
    }

    char hs_query[COMM_ASTR_SZ+HS_NAME_SZ+1];
    prepare_str(hs_query, COMM_ASTR_SZ+HS_NAME_SZ+1);

    strncpy(hs_query, comm_astr, strlen(comm_astr));

    hs_query[COMM_ASTR_SZ] = '\0';
    strncat(hs_query, hs_name, strlen(hs_name));

    ssize_t sz = 0;
    sz = send_all(astr_sockfd, hs_query, COMM_ASTR_SZ+HS_NAME_SZ);
    if (sz != COMM_ASTR_SZ+HS_NAME_SZ) {
        merror(title, "Плохая отправка данных");
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------
// Получить ответ от астролога
int recv_astr_data(int astr_sockfd, char* hs_data)
{
    char title[] = "recv_astr_data";
    if (strlen(hs_data) != HS_DATA_SZ) {
        merror(title, "Некорректный аргумент");
        return -1;
    }

    // Получаем часть гороскопа
    ssize_t sz = 0;
    sz = recv_all(astr_sockfd, hs_data, PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        merror("recv_astr_data", "Приняты некорректные данные");
        return -1;
    }

    hs_data[PROTOCOL_ANSWER_SZ] = '\0';

    // Ответ по протоколу?
    if (       !strncmp(hs_data, "SORRY! \n", PROTOCOL_ANSWER_SZ)
            || !strncmp(hs_data, "THANKS!\n", PROTOCOL_ANSWER_SZ)
            || !strncmp(hs_data, "DENIED!\n", PROTOCOL_ANSWER_SZ))
        return 0;

    hs_data[PROTOCOL_ANSWER_SZ] = '\0';

    char tmp[HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ];
    prepare_str(tmp, HS_DATA_SZ+1 - PROTOCOL_ANSWER_SZ);

    // Дополучение данных гороскопа
    sz = recv_all(astr_sockfd, tmp, HS_DATA_SZ-PROTOCOL_ANSWER_SZ);
    if (sz != (ssize_t)strlen(tmp)) {
        merror(title, "Приняты некорректные данные");
        return -1;
    }
    strncat(hs_data, tmp, strlen(tmp));
    printf(hs_data);

    return 0;
}

//------------------------------------------------------------------------
// Отправить ответ астролога человеку
int send_astr_data_to_human(int hum_sockfd, char* hs_data)
{
    char title[] = "send_astr_data_to_human";

    ssize_t hs_data_sz = strlen(hs_data);
    if (    hs_data_sz != PROTOCOL_ANSWER_SZ &&
            hs_data_sz != HS_DATA_SZ) {
        merror(title, "Некорректный аргумент");
        return -1;
    }

    ssize_t sz = send_all(hum_sockfd, hs_data, hs_data_sz);
    if (sz != hs_data_sz) {
        merror(title, "Плохая отправка данных");
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------
// Присоединиться к астрологу
int connect_to_astr(int astr_sockfd, struct sockaddr_in* astr_addr)
{    
    char title[] = "connect_to_astr";

    int answ = set_sendrecv_timeout(astr_sockfd);
    if (answ) merror(title, "Невозможно задать настройки сокету");

    socklen_t serv_sz = sizeof(*astr_addr);
    if(connect(astr_sockfd, (struct sockaddr *)astr_addr, serv_sz)<0)
        if(errno != EINPROGRESS) {
            merror(title, strerror(errno));
            return -1;
        }
    return 0;
}

//------------------------------------------------------------------------
// Разговор с астрологом и человеком
int talk(int hum_sockfd, int astr_sockfd)
{
    char comm_astr[COMM_ASTR_SZ+1];
    char hs_name  [HS_NAME_SZ+1];
    char hs_data  [HS_DATA_SZ+1];

    prepare_str(comm_astr, COMM_ASTR_SZ+1);
    prepare_str(hs_name,   HS_NAME_SZ+1);
    prepare_str(hs_data,   HS_DATA_SZ+1);

    int answ = 0;

    answ = recv_hquery(hum_sockfd, comm_astr, hs_name);
    if (answ) return answ;

    answ = send_hquery_to_astr(astr_sockfd, comm_astr, hs_name);
    if (answ) return answ;

    answ = recv_astr_data(astr_sockfd, hs_data);
    if (answ) return answ;

    answ = send_astr_data_to_human(hum_sockfd, hs_data);
    if (answ) return answ;

    return 0;
}

//------------------------------------------------------------------------
// Игнорировать зомби
int reap_zombie()
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;    // Игнорируем SIGCHLD
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, 0) == -1) return -1;
    return 0;
}

//------------------------------------------------------------------------
// Подготавливать соединение с человеком
int prepare_human_sockfd(int lsockfd, int* hum_sockfd,
                         struct sockaddr_in* human_addr)
{
    char title[] = "prepare_human_sockfd";

    socklen_t human_sz = sizeof(*human_addr);
    memset(human_addr, 0, human_sz);

    // Получаем сокет для человека
    *hum_sockfd = accept(lsockfd, (struct sockaddr*)human_addr,
                         &human_sz);
    if (*hum_sockfd < 0) {
        merror(title, strerror(errno));
        return -1;
    }

    int answ = set_sendrecv_timeout(*hum_sockfd);
    if (answ) merror(title, "Невозможно задать настройки сокету");

    return 0;
}

//------------------------------------------------------------------------
// Создавать дополнительные процессы для клиентов
int split_process(int lsockfd, struct sockaddr_in* astr_addr)
{
    char title[] = "split_process";

    int hum_sockfd = 0;
    struct sockaddr_in human_addr;
    if (prepare_human_sockfd(lsockfd, &hum_sockfd, &human_addr))
        return -1;

    pid_t pid = fork();
    if (!pid) {            // Дочерний
        close(lsockfd);
        // Если завершен родитель, то завершен потомок
        if(prctl(PR_SET_PDEATHSIG, SIGHUP) < 0)
            merror(title, strerror(errno));

        printf("child pid: =%d\n", getpid());

        int astr_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (astr_sockfd < 0) {  // Сокет астролога не создан
            close(hum_sockfd);  // Закрываем сокет человека
            merror(title, "Невозможно создать сокет");
            return -1;
        }

        if (connect_to_astr(astr_sockfd, astr_addr)) {
            merror(title, "Невозможно присоединиться к астрологу\n"
                          "Закрываем содинение");
            close(hum_sockfd);
            close(astr_sockfd);
            return -1;
        }

        while(!talk(hum_sockfd, astr_sockfd));

        printf("Закрываем соединения:\n"
               "1) Человек - ip %s, порт номер %d\n"
               "2) Астролог - ip %s, порт номер %d\n",
               inet_ntoa(human_addr.sin_addr),
               ntohs(human_addr.sin_port),
               inet_ntoa(astr_addr->sin_addr),
               ntohs(astr_addr->sin_port));

        close(astr_sockfd);
        close(hum_sockfd); // Закрываем сокет клиента
        exit(0);
    }
    else if (pid > 0)      // Родительский
        close(hum_sockfd); // Закрываем сокет клиента
    else if (pid < 0) {
        merror(title, "Не удалось создать процесс");
        return -1;
    }

    return 0;
}

//------------------------------------------------------------------------
// Подготавливать слушающий сокет
int prepare_lsockfd(int lsockfd)
{
    char title[] = "prepare_lsockfd";
    // Разрешаем сокету использовать адрес сразу
    int one = 1;
    if (setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR,
                   &one, sizeof(one)) < 0)
        merror(title, "Невозможно задать настройки сокету");

    unsigned short portno = 0;
    if (get_port(&portno, "Введите порт секретаря:\n") < 0) {
        merror(title, "Неправильный порт");
        return -1;
    }

    // Структура адресов для секретаря
    struct sockaddr_in secr_addr;
    memset(&secr_addr, 0, sizeof(secr_addr));
    secr_addr.sin_family = AF_INET;
    secr_addr.sin_addr.s_addr = INADDR_ANY;
    secr_addr.sin_port = htons(portno);

    // Задаем локальный адрес сокету
    bind(lsockfd, (struct sockaddr*)&secr_addr, sizeof(secr_addr));
    listen(lsockfd, 5); // Делаем сокет слушающим

    return 0;
}

//------------------------------------------------------------------------

int prepare_astr_addr(struct sockaddr_in* astr_addr)
{
    char title[] = "prepare_astr_addr";
    // Структура адресов для сервера
    memset(astr_addr, 0, sizeof(*astr_addr));

    unsigned short portno = 0;
    if (get_port(&portno, "Введите номер порта астролога:\n") < 0) {
        merror(title, "Неправильный порт");
        return -1;
    }

    const unsigned short serv_ip_sz = 17;
    char serv_ip[serv_ip_sz];
    memset(&serv_ip, 0, serv_ip_sz);
    get_ip(serv_ip, "Введите ip-номер астролога:\n");

    astr_addr->sin_family = AF_INET;
    astr_addr->sin_port = htons(portno);
    // Переводим ip-строку в структуру sin_addr
    if (!inet_aton(serv_ip , &astr_addr->sin_addr)) {
        merror(title, "Некорректный ip-адрес");
        merror(title, strerror(errno));
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------

void secretary()
{    
    char title[] = "secretary";

    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lsockfd < 0) {
        merror(title, "Невозможно создать сокет");
        return;
    }

    if (prepare_lsockfd(lsockfd)) {
        close(lsockfd);
        return;
    }

    if (reap_zombie()) {
        merror(title, "Не игнорируем зомби");
        close(lsockfd);
        return;
    }

    struct sockaddr_in astr_addr;
    if (prepare_astr_addr(&astr_addr)) {
        close(lsockfd);
        return;
    }

    printf("parrent pid: =%d\n", getpid());

    while(!split_process(lsockfd, &astr_addr));

    close(lsockfd);
}

//------------------------------------------------------------------------

int main()
{
    secretary();
    return 0;
}

//------------------------------------------------------------------------
