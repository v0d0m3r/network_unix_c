/*
 * TCP-сервер, который реализует понятие "Астролог"
 *
 * Функции "Астролога":
 * - общение с клиентом "Звезда"
 * - общение с клиентом "Человек"
 *
 * Формат общения с "Звездой":
 * - запрос от "Звезды": "STARS SAY ZZ\n",
 * где ZZ - знак зодиака, длина строки 22б (добивается пробелами)
 * После заголовка запроса: прогноз длиной 80б (включая '\n')
 * - ответ к "Звезде": "THANKS\n"
 *
 * Формат общения с "Человеком":
 * - запрос от "Человека": "HOROSCOPE ZZ\n" (аналогично "Звезде")
 * - ответ "Человеку": прогноз длиной 80б (включая '\n')
 *
 * Ответ на неправильные запросы - закрытие соединения
*/

#include "../../my_lib.h"

#define COMM_ASTR_SZ 10
#define HS_NAME_SZ 12
#define HS_DATA_SZ 80

//------------------------------------------------------------------------
// Horoscope_str хранит гороскопы в формате:
// hs_name_1hs_data_1...hs_name_nhs_data_n
typedef struct {
    size_t size;  // Рамер строки
    char* buf;    // Указатель на строку c гороскопами
} Horoscope_str;

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
// Чтение данных нужной длины из сокета
ssize_t recv_all(int fd, char* buf, size_t len)
{
    ssize_t received = 0;
    ssize_t rem = len;
    while (len) {
        received = recv(fd, buf, len, 0);
        if (received == -1) return -1;
        if (received == 0) break;
        buf += received;
        len -= received;
    }
    return rem - len;
}

//------------------------------------------------------------------------
// Отправка данных нужной длины из сокета
ssize_t send_all(int fd, char* buf, size_t len)
{
    ssize_t sended = 0;
    ssize_t rem = len;
    while (len) {
        sended = send(fd, buf, len, 0);
        if (sended == -1) return -1;
        buf += sended;
        len -= sended;
    }
    return rem - len;
}

//------------------------------------------------------------------------
// Инициализировать строку в формате:
// "space1space2space3...spaceN-2\0"
int prepare_str(char* str, size_t str_sz)
{
    if (!str_sz) return -1;
    memset(str, ' ', str_sz);
    str[str_sz-1] = '\0';
    return 0;
}

//------------------------------------------------------------------------
// Изменить сокету таймаут на прием/передачу данных
void set_sendrecv_timeout(int cl_sockfd)
{
    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    socklen_t optlen = sizeof(timeout);

    if (setsockopt(cl_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (char*)&timeout, optlen) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");
    if (setsockopt(cl_sockfd, SOL_SOCKET, SO_SNDTIMEO,
                   (char*)&timeout, optlen) < 0)
        printf("Ошибка: Невозможно задать настройки сокету\n");    
}

//------------------------------------------------------------------------
// true, если входная строка является
// именем одного из гороскопов
bool is_horoscope_name(const char* name)
{   
    if (strlen(name) != HS_NAME_SZ) return false;
    static char* hs_tb[] = {
        "Aries", "Taurus", "Gemini", "Cancer", "Leo",
        "Virgo", "Libra", "Scorpio", "Sagittarius",
        "Capricorn", "Aquarius", "Pisces"
    };

    char tmp_str[HS_NAME_SZ+1];
    static const size_t hs_tb_sz = 12;   // Количество гороскопов
    for (size_t i=0; i < hs_tb_sz; ++i) {        
        prepare_str(tmp_str, HS_NAME_SZ+1);
        tmp_str[HS_NAME_SZ-1] = '\n';

        strncpy(tmp_str, hs_tb[i], strlen(hs_tb[i]));
        if (!strncmp(tmp_str, name, HS_NAME_SZ))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------
// Коды команд к астрологу
enum Code_comm_astr
{
    stars_say, horoscope
};

//------------------------------------------------------------------------
// Получить код входной команды
int get_code_comm_astr(const char* comm_astr)
{
    if (strlen(comm_astr) != COMM_ASTR_SZ) return -1;

    static char* comm_astr_tb[] = {
        "STARS SAY ", "HOROSCOPE "
    };

    if (!strncmp(comm_astr, comm_astr_tb[stars_say], COMM_ASTR_SZ))
        return stars_say;

    if (!strncmp(comm_astr, comm_astr_tb[horoscope], COMM_ASTR_SZ))
        return horoscope;

    return -2;
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

    char thanks[] = "THANKS\n";
    sz = send_all(cl_sockfd, thanks, strlen(thanks));
    if (sz != (ssize_t)strlen(thanks)) {
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
        char sorry[] = "SORRY\n";
        sz = send_all(cl_sockfd, sorry, strlen(sorry));
        if (sz != (ssize_t)strlen(sorry)) {
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
void do_client(int cl_sockfd, struct sockaddr_in* client_addr,
               Horoscope_str* hs)
{
    char comm_astr[COMM_ASTR_SZ+1];
    char hs_name  [HS_NAME_SZ+1];
    char hs_data  [HS_DATA_SZ+1];

    prepare_str(comm_astr, COMM_ASTR_SZ+1);
    prepare_str(hs_name,   HS_NAME_SZ+1);
    prepare_str(hs_data,   HS_DATA_SZ+1);

    set_sendrecv_timeout(cl_sockfd);

    int answ = 0;
    ssize_t sz = 0;
    int code_comm_astr = 0;
    while (1) {
        // Получаем данные от клиента
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
        if (code_comm_astr < 0) {
            printf("Неизвестная команда закрываем соединение:\n");
            printf("ip %s, порт номер %d\n",
                   inet_ntoa(client_addr->sin_addr),
                   ntohs(client_addr->sin_port));
            return;
        }

        sz = recv_all(cl_sockfd, hs_name, HS_NAME_SZ);
        if (sz != HS_NAME_SZ || !is_horoscope_name(hs_name)) {
            printf("Некорректное название гороскопа!"
                   "Закрываем соединение!\n");
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

void work_server()
{
    Horoscope_str hs;
    hs.buf = NULL;
    hs.size = 0;
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
        printf("***********new client***************\n");
        do_client(cl_sockfd, &client_addr, &hs); // Работаем с клиентом
        print_current_hs(&hs);
        close(cl_sockfd);                   // Закрываем сокет клиента
        memset(&client_addr, 0, client_sz); // Обнуляем память
                                            // адресов клиента
    }
    if (hs.buf) free(hs.buf);
    close(lsockfd);
}

//------------------------------------------------------------------------

int main()
{
    work_server();
    return 0;
}

//------------------------------------------------------------------------
