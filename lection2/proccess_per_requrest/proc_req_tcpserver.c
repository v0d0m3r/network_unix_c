/*
 * TCP-сервер, который передает строку "Hi there!\n"
 * каждому клиенту 5 раз с интервалом 1 сек
 * Реализована модель process-per-requrest
*/

#include "../../my_lib.h"

//------------------------------------------------------------------------
// do_lient() обрабатывает соединение с сокетом клиента
void do_client(int cl_sockfd, struct sockaddr_in* client_addr)
{
    char str[] = "Hi there!\n";

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    socklen_t optlen = sizeof(timeout);
    // Меняем таймаут на отправку данных
    if (setsockopt(cl_sockfd, SOL_SOCKET, SO_SNDTIMEO,
                   (char *)&timeout, optlen) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");

    const int count_send = 5;
    const unsigned int usec_sleep = 1000000; // 1 сек
    for (int i=0; i < count_send; ++i) { // Посылаем данные клиенту
        if (send(cl_sockfd, &str, strlen(str), MSG_NOSIGNAL) <= 0) {
            printf("Обрыв соединения с ip - %s, порт - %d\n",
                   inet_ntoa(client_addr->sin_addr),
                   ntohs(client_addr->sin_port));
            break;
        }
        printf("Передано клиенту: ip - %s, порт - %d\n",
               inet_ntoa(client_addr->sin_addr),
               ntohs(client_addr->sin_port));
        printf("parent pid: %d, my_pid: %d\n",
               getppid(), getpid());
        usleep(usec_sleep);
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

void work_server()
{        
    // Создание сокета (AF_INET - сетевой(интернет),
    // SOCK_STREAM - потоковый сокет, 0 - по умолчанию TCP)
    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
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
        if (pid == 0) {       // Дочерний
            close(lsockfd);   // Завершаем слушающий сокет
            // При завершении родительского процесса
            // завершать дочернии
            if(prctl(PR_SET_PDEATHSIG, SIGHUP) < 0)
                printf("Ошибка prctl()");
            printf("child pid: =%d\n", getpid());

            do_client(cl_sockfd, &client_addr); // Работаем с клиентом

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
