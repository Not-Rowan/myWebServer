#include "web_server.h"

// program path (e.g. /path/to/program)
char *PROG_PATH;
// path to root page (e.g. /path/to/index.html)
char *ROOT_PATH;

//this is so i can use strcasestr
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <ctype.h> //for strcasestr
#include <time.h> //logging current time


#define FILE_DIR_LEN 8192
#define MAX_FILE_DATA_SIZE 1048576
#define MAX_RESP_SIZE 16384

// create a struct for client info
struct sockaddr_in clientaddr;

// create a timeout struct for a 10 second connection timeout
struct timeval timeout;

int web_server_init(int port_num, char *prog_path, char *root_path) {
    // set library paths
    PROG_PATH = prog_path;
    ROOT_PATH = root_path;

    // init socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        return 1;
    }

    // init server struct
    struct sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr.sin_port = htons(port_num);

    // set socket options to reuse address and port
    int optval = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        return 1;
    }

    // set socket options to add a 10 second connection timeout
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        return 1;
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        return 1;
    }

    // bind socket to server struct
    if (bind(server_socket, (struct sockaddr *) &sockaddr, sizeof(sockaddr)) < 0) {
        return 1;
    }

    return server_socket;
}

char *create_server(int server_socket, int backlog) {
    if (backlog < 1) {
        return NULL;
    }

    // listen for connections
    if (listen(server_socket, backlog) < 0) {
        return NULL;
    }

    // create heap buffer for client request and file data so we only call malloc once per server instance
    char *heap_buffer = malloc((MAX_RESP_SIZE + MAX_FILE_DATA_SIZE + 2)*sizeof(char)); //+3 for 3 null terminators
    if (heap_buffer == NULL) {
        return NULL;
    }

    return heap_buffer;
}

int accept_connection(int server_socket) {
    //checks for client connection
    int client_sock = 0;
    socklen_t client_info_len = sizeof(clientaddr);
    if ((client_sock = accept(server_socket, (struct sockaddr *)&clientaddr, &client_info_len)) < 0) {
        close(client_sock);
        return 1;
    } else {
        return client_sock;
    }
}

// get and return client ip address
char *get_client_ip() {
    char *client_ip = inet_ntoa(clientaddr.sin_addr);
    if (client_ip == NULL) {
        return NULL;
    }
    return client_ip;
}

int get_client_request(int client_socket, char *heap_buffer) {
    // create the pointer for the client request part of the buffer
    char *client_request_buffer = heap_buffer;

    // read to full_resp and catch errors
    int read_bytes = 0;
    if ((read_bytes = recv(client_socket, client_request_buffer, MAX_RESP_SIZE, MSG_NOSIGNAL)) <= 0 ) {
        *client_request_buffer = '\0';

        // error 500: internal server error
        send(client_socket, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);
        return 1;
    }
    client_request_buffer[read_bytes] = '\0';

    return 0;
}

char *get_requested_directory(int client_socket, char *heap_buffer) {
    // create the pointer for the client request part of the buffer
    char *client_request_buffer = heap_buffer;

    // check if there is a '/' in the client request and if not, return 400 bad request
    char *file_dir;
    int i = 0;
    for (; i < strlen(client_request_buffer); i++) {
        if (client_request_buffer[i] == '/') {
            file_dir = &client_request_buffer[i];
            break;
        } else if (i == strlen(client_request_buffer) - 1) {
            // error 400: bad request
            send(client_socket, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);
            return NULL;
        }
    }

    // check if there is a '?' or ' ' after the file directory and if so, remove it. otherwise, return 400 bad request
    i = 0;
    for (; i < strlen(file_dir); i++) {
        if (file_dir[i] == '\0') {
            return NULL;
        }

        if (file_dir[i] == '?' || file_dir[i] == ' ') {
            file_dir[i] = '\0';
            break;
        } else if (i == strlen(file_dir) - 1) {
            // error 400: bad request
            send(client_socket, "HTTP/1.0 400\r\n\r\n", 16, MSG_NOSIGNAL);
            *file_dir = '\0';
            return NULL;
        }
    }

    // prepend file path to file_dir or root path if file_dir is '/'
    char temp_prepend[FILE_DIR_LEN];
    if (strcmp(file_dir, "/") == 0) {
        // prepend root path to file_dir
        strcpy(temp_prepend, ROOT_PATH);
        strcpy(file_dir, temp_prepend);
    } else {
        // prepend file path to file_dir
        strcpy(temp_prepend, PROG_PATH);
        strcat(temp_prepend, file_dir);
        strcpy(file_dir, temp_prepend);
    }

    return file_dir;
}

int get_file_size(FILE *fpointer) {
    // get file size
    fseek(fpointer, 0L, SEEK_END);
    int file_size = ftell(fpointer);
    rewind(fpointer);

    return file_size;
}


int send_http_response(int client_socket, char *file_dir) {
    int fsize = 0;
    FILE *fpointer = NULL;
    char http_response[MAX_RESP_SIZE];

    // open file
    if ((fpointer = fopen(file_dir, "rb")) == NULL) {
        // error 404: not found
        send(client_socket, "HTTP/1.0 404\r\n\r\n", 16, MSG_NOSIGNAL);
        return 1;
    }

    fsize = get_file_size(fpointer);

    if (fsize == 0) {
        // error 500: internal server error
        send(client_socket, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);
        *http_response = '\0';

        fclose(fpointer);
        return 1;
    }

    // sets the server http response and opens file descriptor for requested resource. also gets file size
    if (strcasestr(file_dir, ".jpeg") != NULL || strcasestr(file_dir, ".jpg") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".png") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: image/png\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".ico") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: image/x-icon\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".mp3") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: audio/mpeg\r\nContent-Length: %d\r\nContent-Transfer-Encoding: binary\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".html") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".css") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: text/css\r\nContent-Length: %d\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".js") != NULL) {
    	sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: text/javascript\r\nContent-Length: %d\r\n\r\n", fsize);
    } else if (strcasestr(file_dir, ".txt") != NULL) {
        sprintf(http_response, "HTTP/1.0 200\r\nConnection: close\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", fsize);
    } else {
        sprintf(http_response, "HTTP/1.0 200/r/nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n", fsize);
    }


    // send http part of the response
    int response_len = strlen(http_response);
    if (send(client_socket, http_response, response_len, MSG_NOSIGNAL) != response_len) {
        *http_response = '\0';
        return 1;
    }

    // done with http_response
    *http_response = '\0';

    return 0;
}

int send_file_data(int client_socket, char *heap_buffer, char *file_dir) {
    // create the pointer for the client request part of the buffer
    char *client_request_buffer = heap_buffer;


    FILE *fpointer = NULL;

    // check if the file exists
    if ((fpointer = fopen(file_dir, "rb")) == NULL) {
        // error 404: not found
        send(client_socket, "HTTP/1.0 404\r\n\r\n", 16, MSG_NOSIGNAL);
        return 1;
    }

    // get file size
    int fsize = get_file_size(fpointer);

    // set file_data pointer to file data part of heap_buffer and read file data into file_data buffer
    char *file_data = heap_buffer + MAX_RESP_SIZE+1;
    int fread_err = fread(file_data, fsize, 1, fpointer);

    // error checking
    if (fread_err != 1) {
        // error 500: internal server error
        send(client_socket, "HTTP/1.0 500\r\n\r\n", 16, MSG_NOSIGNAL);

        fclose(fpointer);
        return 1;
    }

    // done with file
    fclose(fpointer);

    // send file data
    if (send(client_socket, file_data, fsize, MSG_NOSIGNAL) != fsize) {
        return 1;
    }

    // done with the following variable(s)
    *file_data = '\0';

    return 0;
}

void close_connection(int client_socket, char *heap_buffer, char *file_dir) {
    // clear buffers and close socket
    *heap_buffer = '\0';
    *file_dir = '\0';
    close(client_socket);
}

void close_web_server(int client_socket, char *heap_buffer) {
    // clear buffers, free allocated memory, and close socket
    *heap_buffer = '\0';
    free(heap_buffer);
}



// gcc main.c /path/to/web_server.c -o main && ./main
