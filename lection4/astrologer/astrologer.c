/*
 * TCP-сервер, который реализует понятие "Астролог"
 *
 * Функции "Астролога":
 * - общение с клиентом "Звезда"
 * - общение с клиентом "Человек"
 *
 * Формат общения со "Звездой":
 * - запрос от "Звезды": "STARS SAY ZZ\n",
 * где ZZ - знак зодиака;
 * длина строки запроса 22б (добивается пробелами)
 * После заголовка запроса: прогноз длиной 80б (включая '\n')
 * - ответ к "Звезде": "THANKS\n"
 *
 * Формат общения с "Человеком":
 * - запрос от "Человека": "HOROSCOPE ZZ\n" (аналогично "Звезде")
 * - ответ "Человеку": прогноз длиной 80б (включая '\n')
 *
 * Ответ на неправильные запросы - закрытие соединения
*/

//------------------------------------------------------------------------

#include "../../my_lib.h"

//------------------------------------------------------------------------
// Добавить новый гороскоп
int push_back_horoscope(Horoscope_str* hs, const char* hs_name,
                        const char* hs_data)
{
    if (       strlen(hs_name) != HS_NAME_SZ
            || strlen(hs_data) != HS_DATA_SZ)
        return -1;
    // Выделяем память под старые и новые данные
    size_t tmp_sz = hs->size + HS_NAME_SZ + HS_DATA_SZ;
    char* tmp_str = (char*)malloc(tmp_sz * sizeof(char) + 1);
    if (!tmp_str) return -1;

    char* begin_str = tmp_str;           // Запоминаем указатель на начало
    for (size_t i=0; i < hs->size; ++i ) // Сохраняем предыдущие данные
        tmp_str[i] = hs->buf[i];

    if (hs->buf) free(hs->buf); // Очищаем старые данные

    tmp_str += hs->size;        // Переносим указатель на 1 ячейку
                                // новых данных
    // Заполняем имя гороскопа
    for (size_t i=0; i < HS_NAME_SZ; ++i) {
        *tmp_str = hs_name[i];
        ++tmp_str;
    }
    // Заполняем данные гороскопа
    for (size_t i = 0; i < HS_DATA_SZ; ++i) {
        *tmp_str = hs_data[i];
        ++tmp_str;
    }

    *tmp_str = '\0';

    hs->buf = begin_str;    // Запоминаем "строку" с гороскопами
    hs->size = tmp_sz;      // Запоминаем размер
    return 0;
}

//------------------------------------------------------------------------
// Поиск позиции данных гороскопа по его имени
int find_pos_horoscope(Horoscope_str* hs, const char* hs_name,
                       int* pos)
{
    *pos = -1;
    if (!hs->size) return 0; // Нет гороскопов    

    if (strlen(hs_name) != HS_NAME_SZ) return -1;

    int h = HS_NAME_SZ + HS_DATA_SZ; // Шаг перехода между гороскопами

    for (size_t i=0; i < hs->size; i += h) // Ищем гороскоп
        if (!strncmp(&hs->buf[i], hs_name, HS_NAME_SZ)) {
            *pos = i + HS_NAME_SZ;  // Запоминаем позицию
            return 0;
        }
    return 0;
}

//------------------------------------------------------------------------
// Заменить старый гороскоп на новый по позиции pos
int replace_data_horoscope(Horoscope_str* hs, const char* source_hs_data,
                           int pos)
{
    if (strlen(source_hs_data) != HS_DATA_SZ) return -1;
    if (pos < 0 || (size_t)(pos+HS_DATA_SZ) > hs->size) return -1;

    strncpy(&hs->buf[pos], source_hs_data, HS_DATA_SZ);
    return 0;
}

//------------------------------------------------------------------------
// Получить гороскоп по позиции pos
int get_data_horoscope(Horoscope_str* hs, char* dest_hs_data,
                       int pos)
{

    if (strlen(dest_hs_data) != HS_DATA_SZ) return -1;
    if (pos < 0 || (size_t)(pos+HS_DATA_SZ) > hs->size) return -1;

    strncpy(dest_hs_data, &hs->buf[pos], HS_DATA_SZ);
    return 0;
}

//------------------------------------------------------------------------
// Обрабатываем соединение со звездой
int do_stars_say_comm_astr(int cl_sockfd, Horoscope_str* hs,
                           char* hs_name, char* hs_data)
{   
    ssize_t sz = recv_all(cl_sockfd, hs_data, HS_DATA_SZ);
    if (sz != HS_DATA_SZ) {
        printf("Не получили данные необходимой длины, "
               "закрываем соединение!\n");
        return -1;
    }
    if (hs_data[HS_DATA_SZ-1] != '\n') {
        printf("Неправильные данные гороскопа!"
               "закрываем соединение!\n");
        return -1;
    }

    int pos = 0;
    int answ = find_pos_horoscope(hs, hs_name, &pos);
    if (answ) {
        printf("Не корректно имя гороскопа, "
               "закрываем соединение!\n");
        return -1;
    }
    //printf("pos: %d\n", pos);
    if (pos == -1) { // Нет гороскопа в Horoscope_str
        answ = push_back_horoscope(hs, hs_name, hs_data);
        if (answ) {
            printf("Не удалось добавить гороскоп, "
                   "закрываем соединение!\n");
            return -1;
        }
    }
    else {
        answ = replace_data_horoscope(hs, hs_data, pos);
        if (answ) {
            printf("Не удалось заменить гороскоп, "
                   "закрываем соединение!\n");
            return -1;
        }
    }

    sz = send_all(cl_sockfd, "THANKS!\n", PROTOCOL_ANSWER_SZ);
    if (sz != PROTOCOL_ANSWER_SZ) {
        printf("Плохая отправка данных\n");
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------
// Обрабатываем соединение с человеком
int do_horoscope_comm_astr(int cl_sockfd, Horoscope_str* hs,
                           char* hs_name, char* hs_data)
{
    int pos = 0;
    ssize_t sz = 0;

    int answ = find_pos_horoscope(hs, hs_name, &pos);
    if (answ) {
        printf("Некорректное имя гороскопа, "
               "закрываем соединение!\n");
        return -1;
    }
    if (pos == -1) { // Нет гороскопа        
        sz = send_all(cl_sockfd, "SORRY! \n", PROTOCOL_ANSWER_SZ);
        if (sz != PROTOCOL_ANSWER_SZ) {
            printf("Плохая отправка данных, "
                   "закрываем соединение!\n");
            return -1;
        }
        return 0;
    }

    answ = get_data_horoscope(hs, hs_data, pos);
    if (answ) {
        printf("Не удалось получить гороскоп, "
               "закрываем соединение!\n");
        return -1;
    }

    sz = send_all(cl_sockfd, hs_data, HS_DATA_SZ);
    if (sz != HS_DATA_SZ) {
        printf("Плохая отправка данных "
               "закрываем соединение!\n");
        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------
// do_client() обрабатывает соединение с сокетом клиента
void do_client(int cl_sockfd, Horoscope_str* hs)
{
    char title[] = "do_client";

    char comm_astr[COMM_ASTR_SZ+1];
    char hs_name  [HS_NAME_SZ+1];
    char hs_data  [HS_DATA_SZ+1];

    prepare_str(comm_astr, COMM_ASTR_SZ+1);
    prepare_str(hs_name,   HS_NAME_SZ+1);
    prepare_str(hs_data,   HS_DATA_SZ+1);

    int answ = set_sendrecv_timeout(cl_sockfd);
    if (answ)
        merror(title, "Невозможно задать настройки сокету");

    ssize_t sz = 0;
    int code_comm_astr = 0;
    while (1) {
        // Получаем данные от клиента
        sz = recv_all(cl_sockfd, comm_astr, COMM_ASTR_SZ);
        if (sz != COMM_ASTR_SZ) {
            merror(title, "Не получили данные необходимой длины");
            return;
        }
        printf("comm_astr: %s\n", comm_astr);

        code_comm_astr = get_code_comm_astr(comm_astr);
        if (code_comm_astr < 0) {
            merror(title, "Неизвестная команда");
            return;
        }

        sz = recv_all(cl_sockfd, hs_name, HS_NAME_SZ);
        if (sz != HS_NAME_SZ || !is_horoscope_name(hs_name)) {
            merror(title, "Некорректное название гороскопа");
            return;
        }
        printf("hs_name: %s", hs_name);

        answ = (code_comm_astr == stars_say)
                ? do_stars_say_comm_astr(cl_sockfd, hs, hs_name, hs_data)
                : do_horoscope_comm_astr(cl_sockfd, hs, hs_name, hs_data);

        if(answ) return;

        prepare_str(comm_astr, COMM_ASTR_SZ+1);
        prepare_str(hs_name,   HS_NAME_SZ+1);
        prepare_str(hs_data,   HS_DATA_SZ+1);
    }
}

//------------------------------------------------------------------------

void print_current_hs(Horoscope_str* hs)
{
    if (!hs->size) return;
    printf("********************\n");
    printf("В буфере:\n");
    printf("%s", hs->buf);
    printf("********************\n");
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

    // Структура адресов для астролога
    struct sockaddr_in astr_addr;
    memset(&astr_addr, 0, sizeof(astr_addr));
    astr_addr.sin_family = AF_INET;
    astr_addr.sin_addr.s_addr = INADDR_ANY;
    astr_addr.sin_port = htons(portno);

    // Задаем локальный адрес сокету
    bind(lsockfd, (struct sockaddr*)&astr_addr, sizeof(astr_addr));
    listen(lsockfd, 5); // Делаем сокет слушающим

    return 0;
}

//------------------------------------------------------------------------

void astrologer()
{
    char title[] = "astrologer";
    Horoscope_str hs;
    hs.buf = NULL;
    hs.size = 0;

    int lsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lsockfd < 0) {
        merror(title, "Невозможно создать сокет");
        return;
    }

    if (prepare_lsockfd(lsockfd)) {
        close(lsockfd);
        return;
    }

    // Структура адресов для сокета клиента
    struct sockaddr_in client_addr;
    socklen_t client_sz = sizeof(client_addr);

    int cl_sockfd = 0;  // Cокет нового клиента
    while (1) {         // Обработка соеднений
        cl_sockfd = accept(lsockfd, (struct sockaddr*)&client_addr,
                           &client_sz);
        if (cl_sockfd < 0) {
            merror(title, strerror(errno));
            break;
        }
        printf("***********new client***********\n");

        do_client(cl_sockfd, &hs);

        printf("Закрываем соединение:\n");
        printf("ip %s, порт номер %d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        print_current_hs(&hs);
        close(cl_sockfd);   // Закрываем сокет клиента
    }
    if (hs.buf) free(hs.buf);
    close(lsockfd);
}

//------------------------------------------------------------------------

int main()
{
    astrologer();
    return 0;
}

//------------------------------------------------------------------------
