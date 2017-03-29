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

int recv_human_query()
{

}

//------------------------------------------------------------------------

int send_answ_to_human()
{

}

//------------------------------------------------------------------------
// do_client() обрабатывает соединение с сокетом клиента
void do_client(int cl_sockfd, struct sockaddr_in* client_addr)
{
    char comm_astr[COMM_ASTR_SZ+1];
    char hs_name  [HS_NAME_SZ+1];
    char hs_data  [HS_DATA_SZ+1];

    prepare_str(comm_astr, COMM_ASTR_SZ+1);
    prepare_str(hs_name,   HS_NAME_SZ+1);
    prepare_str(hs_data,   HS_DATA_SZ+1);

    int answ = set_sendrecv_timeout(sockfd);
    if (answ) printf("Ошибка: невозможно задать настройки сокету\n");

    ssize_t sz = 0;
    int code_comm_astr = 0;

    // Pseudo code
    // recv_hquery
    // send_hquery_to_astrolog (coonnect to astrolog)
    // recv_astrolog_answ
    // send_astrolog_answ_to_human

    // Получаем запрос от Человека
    sz = recv_all(cl_sockfd, comm_astr, COMM_ASTR_SZ);
    if (sz != COMM_ASTR_SZ) {
        printf("Не получили данные необходимой длины, "
               "закрываем соединение:\n");
        printf("ip %s, порт номер %d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        return;
    }
    printf("comm_astr: %s\n", comm_astr);

    code_comm_astr = get_code_comm_astr(comm_astr);
    if (code_comm_astr == stars_say) {
        send_all(cl_sockfd, "DENIED!", strlen("DENIED!"));
        return;
    }
    if (code_comm_astr < 0) {
        printf("Неизвестная команда закрываем соединение:\n");
        printf("ip %s, порт номер %d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        return;
    }

}

//------------------------------------------------------------------------

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

void work_secretary()
{    
    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);  // Слушащий
    if(lsockfd < 0) {
        printf("Ошибка: Невозможно создать сокет\n");
        return;
    }

    // Разрешаем сокету использовать адрес сразу
    int one = 1;
    if (setsockopt(lsockfd, SOL_SOCKET, SO_REUSEADDR,
                   &one, sizeof(one)) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");
    // Структура адресов для сокета сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    unsigned short portno = 0;
    if (get_port(&portno, "Введите порт сервера:\n") < 0) {
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
    listen(lsockfd, 5); // Делаем сокет слушающим

    if (reap_zombie() == -1) {
        printf("Ошибка: Не игнорируем зомби!\n");
        close(lsockfd);
        return;
    }

    printf("parrent pid: =%d\n", getpid());
    int cl_sockfd = 0;
    pid_t pid = 0;
    while(1) { // Обработка соединений
        // Получаем сокет нового клиента
        cl_sockfd = accept(lsockfd, (struct sockaddr*)&client_addr,
                           &client_sz);
        if (cl_sockfd < 0) {
            printf("Ошибка: %s\n", strerror(errno));
            break;
        }
        pid = fork();
        if (!pid) {           // Дочерний
            close(lsockfd);   // Завершаем слушающий сокет
            // При завершении родительского процесса
            // завершать дочернии
            if(prctl(PR_SET_PDEATHSIG, SIGHUP) < 0)
                printf("Ошибка prctl()");
            printf("child pid: =%d\n", getpid());

            do_human(cl_sockfd); // Работаем с клиентом

            close(cl_sockfd);                   // Закрываем сокет клиента
            exit(0);
        }
        else if (pid > 0)     // Родительский
            close(cl_sockfd); // Закрываем сокет клиента
        else if (pid < 0) {
            printf("Ошибка в создании процесса!");
            break;
        }
    }
    close(lsockfd);
}

//------------------------------------------------------------------------

int main()
{
    work_server();
    return 0;
}

//------------------------------------------------------------------------
