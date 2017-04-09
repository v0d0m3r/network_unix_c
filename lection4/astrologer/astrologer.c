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
 * длина строки запроса до символа '\n'
 * После заголовка запроса: прогноз длиной не больше 80б (включая '\n')
 * - ответ к "Звезде": "THANKS\n"
 *
 * Формат общения с "Человеком":
 * - запрос от "Человека": "HOROSCOPE ZZ\n" (аналогично "Звезде")
 * - ответ "Человеку": прогноз длиной не больше 80б (включая '\n')
 *
 * Ответ на неправильные запросы - закрытие соединения
*/

//------------------------------------------------------------------------

#include "../../my_lib.h"

//------------------------------------------------------------------------

char main_buf[HS_DATA_SZ];
ssize_t main_buf_sz = 0;

//------------------------------------------------------------------------
// Получает блок данных до символа ch
// Возвращает размер блока,
// 0, если ничего не получил,
// -1, в случае сбоя
ssize_t get_block(int fd, char* buf, char ch)
{
    char recv_buf[HS_DATA_SZ];
    memset(recv_buf, 0, HS_DATA_SZ);

    ssize_t received = 0;
    ssize_t copy_sz = 0;

    char* find_ch = (char*) memchr(main_buf, ch, main_buf_sz);
    if (find_ch) {
        copy_sz = (ssize_t)(find_ch - main_buf + 1);
        memcpy(buf, main_buf, copy_sz);

        /*memcpy(recv_buf, main_buf+copy_sz, ma-copy_sz);
        memset(main_buf, 0, HS_DATA_SZ);
        memcpy(main_buf, recv_buf, HS_DATA_SZ-copy_sz);*/

        memmove(main_buf, main_buf+copy_sz, main_buf_sz-copy_sz);
        memset(main_buf+copy_sz, 0, copy_sz);

        main_buf_sz -= copy_sz;
        return copy_sz;
    }

    received = recv_all(fd, recv_buf, HS_DATA_SZ);
    if (received <= 0) return received;

    // Получаем максимальный размер, который
    // можно добавить из recv_buf в main_buf
    copy_sz = (main_buf_sz + received > HS_DATA_SZ)
            ? HS_DATA_SZ - main_buf_sz
            : received;

    // Есть ли в подготовленных данных символ ch?
    find_ch = (char*) memchr(recv_buf, ch, copy_sz);
    if (!find_ch) return -1;

    // Получаем размер до символа ch (включительно)
    copy_sz = (ssize_t)(find_ch - recv_buf + 1);
    // Дополняем main_buf из recv_buf
    memcpy(main_buf + main_buf_sz, recv_buf, copy_sz);
    main_buf_sz += copy_sz; // Размер актуальных байт увеличился

    ssize_t block_sz = main_buf_sz;   // Запоминаем размер блока

    memcpy(buf, main_buf, block_sz);

    memset(main_buf, 0, HS_DATA_SZ);  // Сбрасываем главный буфер

    main_buf_sz = received - copy_sz; // Обновляем размер буфера

    // Копируем остатки в главный буфер
    memcpy(main_buf, recv_buf + copy_sz, main_buf_sz);

    return block_sz;
}

//------------------------------------------------------------------------
// Добавить новый гороскоп
int push_back_horoscope(Horoscope_str* hs, const char* hs_name,
                        const char* hs_data)
{
    size_t hs_name_sz = strlen(hs_name);
    size_t hs_data_sz = strlen(hs_data);

    // Выделяем память под старые и новые данные
    size_t tmp_sz = hs->size + hs_name_sz + hs_data_sz;
    char* tmp_str = (char*)malloc(tmp_sz * sizeof(char) + 1);
    if (!tmp_str) return -1;

    char* begin_str = tmp_str;           // Запоминаем указатель на начало
    for (size_t i=0; i < hs->size; ++i ) // Сохраняем предыдущие данные
        tmp_str[i] = hs->buf[i];

    if (hs->buf) free(hs->buf); // Очищаем старые данные

    tmp_str += hs->size;        // Переносим указатель на 1 ячейку
                                // новых данных
    // Заполняем имя гороскопа
    for (size_t i=0; i < hs_name_sz; ++i) {
        *tmp_str = hs_name[i];
        ++tmp_str;
    }
    // Заполняем данные гороскопа
    for (size_t i = 0; i < hs_data_sz; ++i) {
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
                           char* hs_name, char* block_buf)
{   
    char title[] = "do_stars_say_comm_astr";
    ssize_t sz = get_block(cl_sockfd, block_buf, '\n');
    if (sz <= 0) {
        merror(title, "не получили данные необходимой длины");
        return -1;
    }

    char hs_data[sz+1];
    prepare_str(hs_data, sz+1);
    memcpy(hs_data, block_buf, sz);

    if (hs_data[sz-1] != '\n') {
        merror(title, "неправильные данные гороскопа");
        return -1;
    }

    int pos = 0;
    int answ = find_pos_horoscope(hs, hs_name, &pos);
    if (answ) {
        merror(title, "некорректное имя гороскопа");
        return -1;
    }
    //printf("pos: %d\n", pos);
    if (pos == -1) { // Нет гороскопа в Horoscope_str
        answ = push_back_horoscope(hs, hs_name, hs_data);
        if (answ) {
            merror(title, "не удалось добавить гороскоп");
            return -1;
        }
    }
    else {
        answ = replace_data_horoscope(hs, hs_data, pos);
        if (answ) {
            merror(title, "не удалось заменить гороскоп");
            return -1;
        }
    }

    sz = send_all(cl_sockfd, "THANKS!\n", sizeof("THANKS!\n"));
    if (sz != sizeof("THANKS!\n")) {
        merror(title, "плохая отправка данных");
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

int choose_speaker(int cl_sockfd, Horoscope_str* hs,
                    char* block_buf, size_t sz)
{
    char title[] = "choose_speaker";

    if (sz < COMM_ASTR_SZ) {
        merror(title, "некорректная длина команды");
        return -1;
    }
    char comm_astr[COMM_ASTR_SZ+1];
    prepare_str(comm_astr, COMM_ASTR_SZ+1);

    memcpy(comm_astr, block_buf, COMM_ASTR_SZ);
    printf("comm_astr: %s\n", comm_astr);

    int code_comm_astr = get_code_comm_astr(comm_astr);
    if (code_comm_astr < 0) {
        merror(title, "неизвестная команда");
        return -1;
    }

    size_t hs_name_sz = sz - COMM_ASTR_SZ;
    char hs_name[hs_name_sz+1];
    prepare_str(hs_name, hs_name_sz+1);

    memcpy(hs_name, block_buf+COMM_ASTR_SZ, hs_name_sz);

    if (!is_horoscope_name(hs_name)) {
        merror(title, "некорректное имя гороскопа");
        return -1;
    }
    printf("hs_name: %s", hs_name);

    memset(block_buf, 0, HS_DATA_SZ);

    int answ = (code_comm_astr == stars_say)
            ? do_stars_say_comm_astr(cl_sockfd, hs, hs_name, block_buf)
            : do_horoscope_comm_astr(cl_sockfd, hs, hs_name, block_buf);

    return answ;

}

//------------------------------------------------------------------------
// do_client() обрабатывает соединение с сокетом клиента
void do_client(int cl_sockfd, Horoscope_str* hs)
{
    char title[] = "do_client";

    int answ = set_sendrecv_timeout(cl_sockfd);
    if (answ)
        merror(title, "невозможно задать настройки сокету");

    char block_buf[HS_DATA_SZ];
    memset(block_buf, 0, HS_DATA_SZ);

    ssize_t sz = 0;
    while (1) {
        // Получаем данные от клиента
        sz = get_block(cl_sockfd, block_buf, '\n');
        if (sz <= 0) {
            printf("%s: %s\n", title, "нет данных");
            return;
        }

        answ = choose_speaker(cl_sockfd, hs, block_buf, sz);
        if(answ) return;

        memset(block_buf, 0, HS_DATA_SZ);
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
        memset(main_buf, 0, HS_DATA_SZ);
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
