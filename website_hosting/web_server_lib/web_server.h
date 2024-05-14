#ifndef WEB_SERVER_H_
#define WEB_SERVER_H_

// initializes the web server
//
// return: server socket file descriptor or 1 on error
// port_num: port number to listen on
// prog_path: path to program directory (e.g. /home/user/program)
// root_path: path to root file directory (e.g. /home/user/program/websites/index.html)
int web_server_init(int port_num, char *prog_path, char *root_path);

// creates the server and returns heap memory to be used in the program
//
// return: heap memory to be used or NULL on error
// server_socket: server socket file descriptor
// backlog: number of connections to queue. Must be > 0
char *create_server(int server_socket, int backlog);


// accepts client and returns a client socket
//
// return: client socket descriptor or 1 on error
// sock: socket file descriptor
int accept_connection(int server_socket);

// gets the ip of the current client
//
// return: buffer to client ip string or NULL on error
char *get_client_ip();

// gets the request from the client and stores it in the client request part of heap_buffer
//
// return: 0 on success, 1 on error
// client_socket: client socket file descriptor
// heap_buffer: buffer to store client request
int get_client_request(int client_socket, char *heap_buffer);

// gets the requested directory string
//
// return: file directory string or NULL on error
// client_socket: client socket file descriptor
// heap_buffer: where the client request is stored
char *get_requested_directory(int client_socket, char *heap_buffer);


// sends the http response to the client
//
// return: 0 on success, 1 on error
// client_socket: client socket file descriptor
// file_dir: file directory string
int send_http_response(int client_socket, char *file_dir);

// sends the file data to the client
//
// return: 0 on success, 1 on error
// client_socket: client socket file descriptor
// heap_buffer: buffer to store client request and file data
// file_dir: file directory string
int send_file_data(int client_socket, char *heap_buffer, char *file_dir);

// closes the client connection
//
// client_socket: client socket file descriptor
// heap_buffer: buffer to store client request and file data
// file_dir: file directory string
void close_connection(int client_socket, char *heap_buffer, char *file_dir);

// closes the web server
//
// client_socket: client socket file descriptor
// heap_buffer: buffer to store client request and file data
void close_web_server(int client_socket, char *heap_buffer);

#endif
