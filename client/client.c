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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "FTS_client.h"

int main() {
    int sock;                // дескриптор сокета
    int check, counter = 0;
    struct sockaddr_in addr; // структура с адресом

    int number = 0, *getTeam;
    getTeam = &number;

    /*     Указываем параметры сервера    */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET; // домены Internet
    addr.sin_port = htons(3425); // Порт
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    /*END*/

    if ((check = inet_pton(addr.sin_family, "127.0.0.1", &addr.sin_addr)) < 0) {
        perror("Error: первый параметр не относится к категории корректных адресов.");
    } else if (!check) {
        perror("Error: второй параметр не содержит корректного IP-адреса.");
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {// создание TCP-сокета
        perror("socket");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) { // установка соединения с сервером
        perror("connet");
        close(sock);
        exit(2);
    } else {
        printf("Вы успешно подключились к системе передачи файлов!\n");
    }

    while (1) {

        if (counter == 0) {
            recv(sock, message, sizeof(message), 0);
            counter++;
        }

        printf("%s\n", message);

        do {
            printf("Введите команду серверу[0-6]:");
            scanf("%d", &number);
        } while (number > 7 || number < 0);

        printf("Отправляю сообщение серверу.\n");

        if ((sendall(sock, (char *) &number, sizeof(int), 0)) < 0) {
            perror("send[0]");
        }

        printf("%d\n", *getTeam);

        if (navigation(*getTeam, sock) == 1) {
            remove(pathToList);
            break;
        }
    }
    close(sock);
    _exit(0);
}
