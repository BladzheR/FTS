/**
* @author: Sergey Kudryavtsev <bladzher@yandex.ru>
*/
/*  HELP:
* PATH files:
* ...........PC name: /ksergey/
* ...........Ultrabook name: /bladzher/
*
*******************************************************************************
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "FTS_server.h"

#define textMenu "\n========================\nМЕНЮ:\n0)Отобразить список файлов.\n1)Добавить файл.\n2)Удалить файл.\n3)Скачать файл.\n4)Выход.\n5)Выключить сервер.\n6)Обновить список файлов.\n7)Очистить экран\n========================\n"

int main() {

    int sock, listener;      // дескрипторы сокетов
    int check, yes = 1, counter = 0;

    int number = 0, *getCommand;
    getCommand = &number;

    //pid_t pid;

    if ((listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {// создаем сокет для входных подключений
        perror("socket");
        exit(1);
    }

    /*     Указываем параметры сервера    */
    struct sockaddr_in addr; // структура с адресом
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /*    END block    */

    if ((check = inet_pton(addr.sin_family, "127.0.0.1", &addr.sin_addr)) < 0) {
        perror("Error: первый параметр не относится к категории корректных адресов.");
    } else if (!check) {
        perror("Error: второй параметр не содержит корректного IP-адреса.");
    }

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind(listener, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) { // связываемся с сетевым устройство. Сейчас это устройство lo - "петля", которое используется для отладки сетевых приложений
        perror("bind");
        close(listener);
        exit(2);
    }

    if ((listen(listener, 1)) == -1) { // очередь входных подключений
        perror("listen");
        close(listener);
        exit(0);
    }

    /*  демон - запуск отдельно от терминала...
    pid_t pid = daemon(0,0);
    if ( pid == -1 ) {
        //close(ls);
        perror( "demonize error!\n");
        //return;
    }
    */

    printf("Сервер запущен!\n\nОжидание подключения:\n");
//    while (1){ //для подкл.клиентов.

    if ((sock = accept(listener, NULL, NULL)) < 0) {// принимаем входные подключение и создаем отделный сокет для каждого нового подключившегося клиента
        perror("accept");
        close(listener);
        exit(3);
    }

    //pid = fork();

//    if (pid < 0){
//        syslog(LOG_ERR, " fork abort");
//    }
//    if (pid == 0) {
//        close(listener);

    printf("К серверу подключился клиент!\n");

    while (1) {
        if (counter == 0) {
            sendall(sock, textMenu, sizeof(textMenu), 0);
            counter++;
        }
        printf("Ожидаем команду:\n");
        if ((recv(sock, (void *) &number, sizeof(int), 0)) < 0) {
            perror("recv[0]");
        }

        printf("Получена команда от клиента: %d\n", *getCommand);

        if (navigation(*getCommand, sock) == 1) {
            break;
        }
    }
//        close(sock);
//    } else {
//        close(sock);
//    }
//
//}
    close(sock);
    close(listener);
    _exit(0);
}














