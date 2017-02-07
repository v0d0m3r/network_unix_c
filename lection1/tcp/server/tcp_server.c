//------------------------------------------------------------------------

#include "../../../my_lib.h"

//------------------------------------------------------------------------
// do_lient() обрабатывает соединение с сокетом клиента
void do_lient(int* cl_sockfd, struct sockaddr_in* client_addr)
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
        do_lient(&cl_sockfd, &client_addr); // Работаем с клиентом

        close(cl_sockfd);                   // Закрываем сокет клиента
        memset(&client_addr, 0, client_sz); // Обнуляем память
                                            // адресов клиента
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
