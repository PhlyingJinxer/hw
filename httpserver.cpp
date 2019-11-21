#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#define PORT 80

#define MAXTHREADS 10

pthread_mutex_t thread_mutex;
pthread_cond_t thread_condition;

void* handle_connection(void* arg);
void serve(int argc, char const *argv[]);
void create_pool(pthread_t *threads, int *threadIDs, int pool);




/// q
struct Handle {
    int socket;
};

int queue_head = 0;
int queue_tail = 0;
struct Handle queue[4];

// call isFull before you call this
void enqueue(int socket) {
    queue_tail = (queue_tail + 1) % 4;
    struct Handle item;
    item.socket = socket;
    queue[queue_head] = item;
}

int isEmpty() {
    return queue_head == queue_tail;
}

int isFull() {
    return ((queue_tail + 1) % 4 ) == queue_head;
}

// call isEmpty before you call this
int dequeue() {
    int socket = queue[queue_head].socket;
    queue_head = (queue_head + 1) % 4;
    return socket;
}

int main(int argc, char const *argv[]) {
    serve(argc, argv);
}

void create_pool(pthread_t * threads, int * threadIDs, int pool) {
    
    int index, returnVal;
    char message[100];
    
    for (index = 0; index < pool; index++) {
        threadIDs[index] = index;
        
        returnVal = pthread_create(&threads[index], NULL, handle_connection, &threadIDs[index]);
        
        if (returnVal) {
            sprintf(message, "pthread_create() fails with error code %d", returnVal);
            printf("%s\n", message);
        }
    }
    
}

void serve(int argc, char const *argv[]) {
    int server_fd, new_socket;//, valread;
    struct sockaddr_in address;
    int opt = 1;
//    char buffer[1024] = {0};
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        close(server_fd);
        exit(2);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // initialize pool
   int pool = 4;
   pthread_t threads[MAXTHREADS];
   int threadIDs[MAXTHREADS];
   create_pool(threads, threadIDs, pool);
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    pthread_mutex_init(&thread_mutex, NULL);
    pthread_cond_init(&thread_condition, NULL);
    
    for (;;) {
        int addrlen = sizeof(address);
        printf("before accept\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("before enqueue\n");
        enqueue(new_socket);
        
        printf("enqueued: %d\n", new_socket);
        pthread_cond_broadcast(&thread_condition);
        printf("after accept\n");
    }
}

void* handle_connection(void* arg) {
   //open mutex
    pthread_mutex_lock(&thread_mutex);

    pthread_cond_wait(&thread_condition, &thread_mutex);
    int new_socket = dequeue();
    
    int valread;
//    struct sockaddr_in address;
    char buffer[1024] = {0};
    
    valread = (int)recv(new_socket, buffer, 1024, 0);
    char method[1024];
    char socketName[1024];
    char uri[1024];
    sscanf(buffer, "%s %s %s\n", method, uri, socketName);
    printf("client sez:>>>%s<<<\n", buffer);
    printf("method:%s\nuri:%s\nsocketName:%s\n", method, uri, socketName);
//        send(new_socket, hello, strlen(hello), 0);
    
//        sprintf(path, "./%s", uri);
    ssize_t numRead;
//        send(new_socket, socketName, strlen(socketName), 0);
    char* response200 = socketName;
    char* response400 = socketName;
    char* response403 = socketName;
    char* response404 = socketName;
    char* response500 = socketName;
    strcat(response200, "");
    strcat(response400, "");
    strcat(response403, "");
    strcat(response404, "");
    strcat(response500, "");
    
    strcat(response200, "200 OK\r\n");
    strcat(response400, "400 Bad Request\r\n");
    strcat(response403, "403 Forbidden\r\n");
    strcat(response404, "404 Not Found\r\n");
    strcat(response500, "500 Internal Server Error\r\n");
    
    if (!strcasecmp(method, "GET")) {
        char getBuffer[1024];
        int fd = 0;
        if (strlen(uri) > 27) {
            send(new_socket, "400 Bad Request\n", strlen("400 Bad Request\n"), 0);
        }
        else if ((fd = open(uri, O_RDONLY)) == 0) {
            if (errno == 13) {
                send(new_socket, "403 Forbidden\n", strlen("403 Forbidden\n"), 0);
            } else if (errno == 9) {
                send(new_socket, "404 Not Found\n", strlen("404 Not Found\n"), 0);
            } else {
                send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
            }
            perror("open");
            send(new_socket, "", strlen(getBuffer), 0);
        } else {
            do {
                if ((numRead = (int)read(fd, getBuffer, 1024)) < 0) {
                    if (errno == 13) {
                        send(new_socket, "403 Forbidden\n", strlen("403 Forbidden\n"), 0);
                    } else if (errno == 9) {
                        send(new_socket, "404 Not Found\n", strlen("404 Not Found\n"), 0);
                    } else {
                        send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
                    }
                    break;
                }
                if (numRead >= 0) {
                    printf("%s", getBuffer);
                }
                send(new_socket, "200 OK\n", strlen("200 OK\n"), 0);
                send(new_socket, getBuffer, strlen(getBuffer), 0);
            } while (numRead == 1024);
        }
    } else if(!strcasecmp(method, "PUT")) {
        int fd;
        int contentLength = 1024;
        char searchString[] = "Content-Length:";
        char *offset;
        char recvBuffer[1024] = "";
        int cameFromFile = 0;
        ssize_t readTotal = 0;
        if ((offset = strstr(buffer, searchString))) {
            sscanf(offset + strlen(searchString), "%d", &contentLength);
        }
        readTotal = contentLength;
        if (contentLength == 0) {
            if ((fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, 0666)) <= 0) {
                perror("open");
                if (errno == 13) {
                    send(new_socket, "403 Forbidden\n", strlen("403 Forbidden\n"), 0);
                } else if (errno == 9) {
                    send(new_socket, "404 Not Found\n", strlen("404 Not Found\n"), 0);
                } else {
                    send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
                }
            }
            write(fd, recvBuffer, readTotal);
        }
        if((numRead = recv(new_socket , buffer , 1024 , 0)) > 0 ) {
            strcat(recvBuffer, buffer);
            cameFromFile = 1;
//                readTotal += numRead;
        }

        if (cameFromFile == 1) {
            if ((fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, 0666)) <= 0) {
                perror("open");
                if (errno == 13) {
                    send(new_socket, "403 Forbidden\n", strlen("403 Forbidden\n"), 0);
                } else if (errno == 9) {
                    send(new_socket, "404 Not Found\n", strlen("404 Not Found\n"), 0);
                } else {
                    send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
                }
            }
            write(fd, recvBuffer, readTotal);
        } else {
//                if ((offset = strstr(buffer, searchString))) {
//                    sscanf(offset + strlen(searchString), "%d", &contentLength);
//                }
            if (!(offset = strstr(buffer, "\r\n\r\n"))) {
                perror("strstr");
                send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
            }
            
            if ((fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, 0666)) <= 0) {
                perror("open");
                if (errno == 13) {
                    send(new_socket, "403 Forbidden\n", strlen("403 Forbidden\n"), 0);
                } else if (errno == 9) {
                    send(new_socket, "404 Not Found\n", strlen("404 Not Found\n"), 0);
                } else {
                    send(new_socket, "500 Internal Server Error\n", strlen("500 Internal Server Error\n"), 0);
                }
            }
            
            write(fd, offset+4, contentLength);
    }
        send(new_socket, "200 OK\n", strlen("200 OK\n"), 0);
    }
    
    close(new_socket);
    //close mutex
    pthread_mutex_unlock(&thread_mutex);
    return NULL;
}
