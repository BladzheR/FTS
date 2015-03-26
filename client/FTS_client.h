//
// Created by ksergey on 23.03.15.
//

#ifndef _FILETRANSFERSYSTEM_FTS_CLIENT_H_
#define _FILETRANSFERSYSTEM_FTS_CLIENT_H_

#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#define sizeName 256
#define BUF_SIZE 1024

#define pathToList "list.xml"
#define pathToLoad "Загрузки/"
//#define Limit "0123456"

char message[1024];

enum {
    commandZero, commandOne, commandTwo, commandTree, commandFour, commandFive, commandSix, commandSeven
};

int sendall(int s, char *buf, int len, int flags) {
    int total = 0;
    int n = 0;
    while (total < len) {
        n = (int) send(s, buf + total, (size_t) (len - total), flags);
        if (n == -1) break;
        total += n;
    }
    return (n == -1 ? -1 : total);
}

int deleteFile(int sock) {

    int number, i = 0;

    printf("Введите номер файла для удаления:");
    scanf("%d", &number);

    printf("%d\nОтправляю сообщение серверу.\n", number);

    if ((sendall(sock, (char *) &number, 4, 0)) < 0) {
        perror("send[3]");
    }

    recv(sock, &i, 4, 0);
    if (i != 0) {
        printf("В данный момент файла под данным номером не существует.Файл не удален!\n");
    } else {
        recv(sock, message, sizeof(message), 0);
        printf("%s\n", message);
        //displayListFiles(sock);
    }
    return 0;
}

int downloadFile(int sock) {

    int k = 0, i = 0, number;
    char pathToFile[] = pathToLoad;
    char fileName[sizeName];

    printf("Введите номер файла который желаете скачать:");
    scanf("%d", &number);

    if ((sendall(sock, (char *) &number, 4, 0)) < 0) {
        perror("send[4]");
    }

    recv(sock, &i, sizeof(i), 0);
    if (i != 0) {
        printf("В данный момент файла под данным номером не существует.Файл не скачен!\n");
    } else {
        FILE *f0;
        if (!(f0 = fopen(pathToList, "rb"))) {
            perror("fopen:");
            return 1;
        }

        while ((fileName[i] = (char) fgetc(f0)) != EOF) {

            if (fileName[i] == '\n') {
                fileName[i] = '\0';
                i = 0;
                if (k == number) {break;}
                k++;
            }
            else {
                i++;
            }
        }

        fileName[i] = '\0';

        for (k = 1, i = 0; fileName[i] != ')'; i++) {
            k++;
        }

        strcpy(fileName, &fileName[k]);
        strcat(pathToFile, fileName);

        fclose(f0);

        long buffer[BUF_SIZE], rcv_len = 0;

        FILE *f;
        if (!(f = fopen(pathToFile, "wb"))) {
            perror("fopen");
            return 1;
        }

        long fsize = 0;
        if ((recv(sock, &fsize, sizeof(fsize), 0)) < 0) {
            perror("recv[10]");
        }

        sleep(1);

        int rc;
        fd_set fdr;
        FD_ZERO(&fdr);
        FD_SET(sock, &fdr);
        struct timeval timeout;
        timeout.tv_sec = 1;   ///зададим  структуру времени со значением 1 сек
        timeout.tv_usec = 0;

        do {
            rcv_len = recv(sock, buffer, BUF_SIZE, 0);
            fwrite(buffer, 1, (size_t) rcv_len, f);
            rc = select(sock + 1, &fdr, NULL, NULL, &timeout);    ///ждём данные для чтения в потоке 1 сек.
        } while (rc);     ///проверяем результат

        fclose(f);
        printf("\nФайл успешно скачен!\n");
    }
    return 0;
}

int fileTransferSend(int sock, char pathToFile[]) {

    long sended = 0, readed = 0;
    char buffer[BUF_SIZE];
    FILE *f;
    if (!(f = fopen(pathToFile, "rb"))) {
        perror("fopen:");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    int fsize = (int) ftell(f);
    rewind(f);

    if ((sendall(sock, (char *) &fsize, sizeof(int), 0)) < 0) {
        perror("send[10]");
    }

    sleep(1);

    do {
        readed = fread(buffer, 1, BUF_SIZE, f);

        if ((sendall(sock, buffer, (int) readed, 0)) < 0) {
            perror("send[0]");
        }
        sended += readed;
    } while (sended != fsize);
    fclose(f);

    return 0;
}

int fileTransferRecv(int sock) {

    char buffer[BUF_SIZE];
    long rcv_len = 0;
    memset(buffer, 0, sizeof buffer);

    FILE *f;
    if (!(f = fopen(pathToList, "wb+"))) {
        perror("fopen");
        return 1;
    }

    long fsize = 0;
    if ((recv(sock, &fsize, sizeof(fsize), 0)) < 0) {
        perror("recv[10]");
    }

    sleep(1);

    int rc;
    fd_set fdr;
    FD_ZERO(&fdr);
    FD_SET(sock, &fdr);
    struct timeval timeout;
    timeout.tv_sec = 1;   ///зададим  структуру времени со значением 1 сек
    timeout.tv_usec = 0;

    do {
        rcv_len = recv(sock, buffer, BUF_SIZE, 0);
        fwrite(buffer, 1, (size_t) rcv_len, f);
        rc = select(sock + 1, &fdr, NULL, NULL, &timeout);    ///ждём данные для чтения в потоке 1 сек.
    } while (rc);     ///проверяем результат

    fclose(f);

    return 0;
}

int addFile(int sock) {

    int i = 0;
    char pathToFile[sizeName], fileName[sizeName];

    printf("Введите путь к файлу[/home/bladzher/Загрузки/]:");
    scanf("%s", pathToFile);

    printf("Введите имя файла с расширением(Пробелы замените на '_')[Mr.Probz-Waves(Robin_Schulz_Radio_Edit)(Europa_Plus).mp3]:");
    scanf("%s", fileName);

    strcat(pathToFile, fileName);

    FILE *f;
    if (!(f = fopen(pathToFile, "r"))) {
        printf("Файл не найден!\n");
        i++;
    }

    if ((sendall(sock, (char *) &i, 4, 0)) < 0) {
        perror("send[1]");
    }

    if (i == 0) {

        if ((sendall(sock, fileName, sizeof(fileName), 0)) < 0) {
            perror("send[2]");
        }

        fileTransferSend(sock, pathToFile);

        fclose(f);
        printf("Файл успешно добавлен!\n");
    }
    return 0;
}

int displayListFiles(int sock) {

    fileTransferRecv(sock);

    FILE *f;
    if (!(f = fopen(pathToList, "rb"))) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long lSize = ftell(f);
    rewind(f);

    if (lSize == 0) {
        printf("\nДанные не получены!Пожалуйста,попробуйте еще раз.\n");
    } else {

        char buffer[BUF_SIZE * 4];
        memset(buffer, 0, sizeof buffer);

        long result = fread(buffer, 1, (size_t) lSize, f);
        if (result != lSize) {
            perror("Ошибка чтения");
            return 1;
        }

        printf("\nСписок файлов:\n%s\n", buffer);
    }
    fclose(f);
    return 0;
}

int navigation(int getTeam, int sock) {
    if (getTeam == commandZero) {
        if (displayListFiles(sock) != 0) {
            perror("sendListFiles:");
        }
    }
    else if (getTeam == commandOne) {
        if (addFile(sock) != 0) {
            perror("addFiles:");
        }
    }
    else if (getTeam == commandTwo) {
        if (deleteFile(sock) != 0) {
            perror("deleteFile:");
        }
    }
    else if (getTeam == commandTree) {
        if (downloadFile(sock) != 0) {
            perror("downloadFile");
        }
    }
    else if (getTeam == commandFour) {
        system("clear");
        printf("Вы успешно завершили работу программы клиент!\n");
        return 1;
    }
    else if (getTeam == commandFive) {
        system("clear");
        printf("Вы успешно завершили работу сервера и программы клиент!\n");
        return 1;
    }
    else if (getTeam == commandSix) {

    }
    else if (getTeam == commandSeven) {
        system("clear");
    }
    return 0;
}

#endif //_FILETRANSFERSYSTEM_FTS_CLIENT_H_
