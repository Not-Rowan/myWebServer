#include <stdio.h>
#include <web_server_lib/web_server.h>

int main() {
    int web_server_sock = web_server_init(8080, "path/to/websites/folder", "/path/to/homepage/page/navigator");
    if (web_server_sock == 1) {
        printf("Error: web_server_init() failed\n");
        return 1;
    }

    char *heap_buffer = create_server(web_server_sock, 100);
    if (heap_buffer == NULL) {
        printf("Error: create_server() failed\n");
        return 1;
    }

    int client_sock = 0;
    int bytes_read = 0;
    char *requested_file = NULL;

    while (1) {
        if ((client_sock = accept_connection(web_server_sock)) == 1) {
            //printf("Error: accept_client() failed\n");
            //close_connection(client_sock, heap_buffer, requested_file);
            continue;
        }

        if ((bytes_read = get_client_request(client_sock, heap_buffer)) == 1) {
            printf("Error: get_client_request() failed\n");
            close_connection(client_sock, heap_buffer, requested_file);
            continue;
        }

        printf("%s\n", heap_buffer);

        if ((requested_file = get_requested_directory(client_sock, heap_buffer)) == NULL) {
            printf("Error: get_requested_directory() failed\n");
            close_connection(client_sock, heap_buffer, requested_file);
            continue;
        }

        if (send_http_response(client_sock, requested_file, NULL) == 1) {
            printf("Error: send_http_response() failed\n");
            close_connection(client_sock, heap_buffer, requested_file);
            continue;
        }

        if (send_file_data(client_sock, heap_buffer, requested_file) == 1) {
            printf("Error: send_file_data() failed\n");
            close_connection(client_sock, heap_buffer, requested_file);
            continue;
        }

        close_connection(client_sock, heap_buffer, requested_file);
    }


    close_web_server(client_sock, heap_buffer);

    return 0;
}

// gcc manage.c /usr/include/web_server_lib/web_server.c -o manage && ./manage
