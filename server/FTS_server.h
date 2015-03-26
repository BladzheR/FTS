//
// Created by ksergey on 23.03.15.
//

#ifndef _FILETRANSFERSYSTEM_HEADER_FILE_H_
#define _FILETRANSFERSYSTEM_HEADER_FILE_H_

#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <dirent.h>
#include <unistd.h>

#define sizeName 256
#define BUF_SIZE 1024

#define pathToFolder "files"
#define pathToFolers "files/"
#define pathToList "list.xml"

int numberOfFiles = 0;

enum {
    commandZero, commandOne, commandTwo, commandTree, commandFour, commandFive, commandSix
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

int fileTransferSend(int sock, char pathToFile[]) {

    FILE *f;
    if (!(f = fopen(pathToFile, "rb"))) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    int fsize = (int) ftell(f);
    rewind(f);

    char buffer[BUF_SIZE];
    long sended = 0, readed = 0;

    if ((sendall(sock, (char *) &fsize, sizeof(int), 0)) < 0) {
        perror("send[10]");
    }

    sleep(1);

    do {
        readed = fread(buffer, 1, BUF_SIZE, f);

        if ((sendall(sock, buffer, (int) readed, 0)) < 0) {
            perror("send[7]");
        }
        sended += readed;
    } while (sended != fsize);

    fclose(f);

    return 0;
}

int fileTransferRecv(int sock, char pathToFile[]) {

    FILE *f;
    if (!(f = fopen(pathToFile, "wb+"))) {
        perror("fopen::");
        return 1;
    }

    long rcv_len = 0;
    char buffer[BUF_SIZE];
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


int loadList() {

    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(pathToFolder))) {
        perror("diropen");
        return 1;
    }

    FILE *f;
    if (!(f = fopen(pathToList, "wb+"))) {
        perror("fopen");
        return 1;
    }

    int i = 0;
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        fprintf(f, "%d)%s\n", i, entry->d_name);
        i++;

    }
    numberOfFiles = i;

    fclose(f);
    closedir(dir);

    //Задержка
    usleep(1000);
    //

    return 0;
}

int addFile(int sock) {

    int i = 0;
    char pathToFile[] = pathToFolers, fileName[sizeName];

    if ((recv(sock, &i, 4, 0)) < 0) {
        perror("recv[1]");
    }
    if (i == 0) {

        if ((recv(sock, fileName, sizeof(fileName), 0)) < 0) {
            perror("recv[2]");
        }
        strcat(pathToFile, fileName);

        fileTransferRecv(sock, pathToFile);

        printf("\n\nКлиент добавил файл на сервер:%s\n\n", pathToFile);
    } else {
        return 1;
    }
    return 0;
}

int deleteFile(int sock) {

    int number = 0, i = 0;

    if ((recv(sock, &number, 4, 0)) < 0) {
        perror("recv[3]");
    }
    printf("Получен номер файла для удаления: %d\n", number);

    if (number < 0 || number >= numberOfFiles) {
        i++;

        if ((sendall(sock, (char *) &i, sizeof(i), 0)) < 0) {
            perror("send[0]");
        }

    } else {

        if ((sendall(sock, (char *) &i, sizeof(i), 0)) < 0) {
            perror("send[1]");
        }

        FILE *f0;
        if (!(f0 = fopen(pathToList, "rb"))) {
            perror("fopen:");
            return 1;
        }
        i = 0;
        int k = 0;
        char fileName[sizeName];
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
        fclose(f0);
        char pathToFile[] = pathToFolers;
        strcpy(fileName, &fileName[k]);
        strcat(pathToFile, fileName);

        if (remove(pathToFile) == -1) {

            if ((sendall(sock, "Ошибка при удалении файла.Сервер не удалил файл!", sizeof("Ошибка при удалении файла.Сервер не удалил файл!"), 0)) < 0) {
                perror("send[2]");
            }

        } else {
            printf("\nС сервера удалён файл:%s\n\n", pathToFile);
            if ((sendall(sock, "Сервер удалил файл!", sizeof("Сервер удалил файл!"), 0)) < 0) {
                perror("send[3]");
            }
        }
        loadList();
        fileTransferRecv(sock, pathToList);
    }
    return 0;
}

int downloadFile(int sock) {

    int i = 0, k = 0, q = 0, number = 0;
    char fileName[sizeName], pathToFile[] = pathToFolers;

    if ((recv(sock, (void *) &number, 4, 0)) < 0) {
        perror("recv[4]");
    }

    if (number < 0 || number > numberOfFiles) { //>=
        i++;

        if ((sendall(sock, (char *) &i, sizeof(i), 0)) < 0) {
            perror("send[4]");
        }
    } else {

        if ((sendall(sock, (char *) &i, sizeof(i), 0)) < 0) {
            perror("send[5]");
        }
        printf("Клиент скачал файл под номером:%d\n", number);
        number--;

        FILE *fo;
        if (!(fo = fopen(pathToList, "r"))) {
            perror("fopen");
            return 1;
        }

        while ((fileName[i] = (char) fgetc(fo)) != EOF) {

            if (fileName[i] == '\n') {
                fileName[i] = '\0';
                i = 0;
                q = k - 1;
                if (q == number) {
                    break;
                }
                k++;
            } else {
                i++;
            }
        }
        fileName[i] = '\0';
        printf("%s\n", fileName);

        for (k = 1, i = 0; fileName[i] != ')'; i++) {
            k++;
        }

        strcpy(fileName, &fileName[k]);
        strcat(pathToFile, fileName);

        fclose(fo);

        fileTransferSend(sock, pathToFile);

        printf("\nКлиент скачал файл:%s\n", pathToFile);
    }
    return 0;
}

int navigation(int getCommand, int sock) {
    if (getCommand == commandZero) {
        if (fileTransferSend(sock, pathToList) != 0) {
            perror("fileTransferSend:");
        }
    }
    else if (getCommand == commandOne) {
        if (addFile(sock) != 0) {
            perror("addFile:");
        }
    }
    else if (getCommand == commandTwo) {
        if (deleteFile(sock) != 0) {
            perror("deleteFile:");
        }
    }
    else if (getCommand == commandTree) {
        if (downloadFile(sock) != 0) {
            perror("downloadFile:");
        }
    }
    else if (getCommand == commandFour) {
        system("clear");
        printf("Клиент отключился!\n");
        printf("\nОжидание подключения:\n");
    }
    else if (getCommand == commandFive) {
        system("clear");
        printf("Получена команда на завершение работы сервера!\nРабота сервера завершена!\n");
        return 1;
    }
    else if (getCommand == commandSix) {
        if (loadList() != 0) {
            perror("loadList:");
        }
    }
    return 0;
}

#endif //_FILETRANSFERSYSTEM_HEADER_FILE_H_
