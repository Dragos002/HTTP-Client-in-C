#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"


char *compute_get_request(char *host, char *url, char *query_params,
                            char *cookies, char *headerr)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    sprintf(line, "Authorization: Bearer %s", headerr);
    compute_message(message, line);
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
             sprintf(line, "Cookie: %s", cookies);
             compute_message(message, line);
    }
    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_delete_request(char *host, char *url, char* cookies, char *headerr){
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);
    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    sprintf(line, "Authorization: Bearer %s", headerr);
    compute_message(message, line);
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
             sprintf(line, "Cookie: %s", cookies);
             compute_message(message, line);
    }
    // Step 4: add final new line
    compute_message(message, "");
    return message;

}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char *cookies,char* headerrr)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    sprintf(line, "Authorization: Bearer %s", headerrr);
    compute_message(message, line);

    // Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    size_t total_data_size = 0;
    for (int i = 0; i < body_data_fields_count; i++) {
        total_data_size += strlen(body_data[i]);
    }
    sprintf(line, "Content-Length: %zu", total_data_size);
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
        
            sprintf(line, "Cookie: %s", cookies);
            compute_message(message, line);
        
    }

    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    body_data_buffer[0] = '\0';
    for (int i = 0; i < body_data_fields_count; i++) {
        compute_message(body_data_buffer, body_data[i]);
    }
    compute_message(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}