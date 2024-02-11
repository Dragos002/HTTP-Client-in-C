#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

char* extract_token(const char* json_data) {
    // functie pentru extragere token acces biblioteca
    JSON_Value* root_value = json_parse_string(json_data);
    if (!root_value) {
        printf("Error parsing JSON.\n");
        return NULL;
    }

    const char* token = json_object_get_string(json_object(root_value), "token");
    if (!token) {
        printf("Token not found in JSON.\n");
        json_value_free(root_value);
        return NULL;
    }

    char* extracted_token = strdup(token);
    json_value_free(root_value);

    return extracted_token;
}

void traverse(JSON_Value *value, char **normal_string) {
    // functie pentru parsare de obiecte json
    JSON_Object *object;
    JSON_Array *array;
    size_t i;

    switch (json_value_get_type(value)) {
        case JSONString:
            strcat(*normal_string, json_value_get_string(value));
            break;

        case JSONNumber:
            {
                double num = json_value_get_number(value);
                char num_str[32];
                snprintf(num_str, sizeof(num_str), "%g", num);
                strcat(*normal_string, num_str);
            }
            break;

        case JSONObject:
            object = json_value_get_object(value);
            for (i = 0; i < json_object_get_count(object); i++) {
                const char *key = json_object_get_name(object, i);
                JSON_Value *child_value = json_object_get_value(object, key);
                strcat(*normal_string, key);
                strcat(*normal_string, "=");
                traverse(child_value, normal_string);
                if (i < json_object_get_count(object) - 1)
                    strcat(*normal_string, "\n");
            }
            break;

        case JSONArray:
            array = json_value_get_array(value);
            strcat(*normal_string, "[");
            for (i = 0; i < json_array_get_count(array); i++) {
                JSON_Value *child_value = json_array_get_value(array, i);
                traverse(child_value, normal_string);
                if (i < json_array_get_count(array) - 1)
                    strcat(*normal_string, "\n");
            }
            strcat(*normal_string, "]");
            break;

        default:
            break;
    }
}


int main(int argc, char *argv[]){
    setvbuf(stdout,NULL,_IONBF,0);
    setvbuf(stderr,NULL,_IONBF,0);
    char *HOST = "34.254.242.81";
    int port = 8080;
    char *buff = malloc(1024*sizeof(char));
    int sockfd = open_connection(HOST,port,AF_INET,SOCK_STREAM,0);
    char *cookies = calloc(1000,sizeof(char));
    char *token = malloc(1000 * sizeof(char));
    while(1) {
        // se creaza socketul clientului
        int sockfd = open_connection(HOST,port,AF_INET,SOCK_STREAM,0);
        if (fgets(buff,1024,stdin)){
            // se verifica comenzile primite
            if(strcmp(buff,"exit\n")==0){
                free(buff);
                free(cookies);
                free(token);
                exit(0);
            }
            else if(strcmp(buff,"register\n")==0){
                char *username = malloc(50*sizeof(char));
                printf ("username=");
                fgets(username,50,stdin);
                // datele sunt citite cu tot cu \n asadar se adauga
                // terminatorul de sir
                username[strcspn(username,"\n")] = '\0';
                printf("password=");
                char *password = malloc(50*sizeof(char));
                fgets(password,50,stdin);
                password[strcspn(password,"\n")] = '\0';
                // se creeaza un json string
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                json_object_set_string(object,"username",username);
                json_object_set_string(object,"password",password);
                char *jsonstring = json_serialize_to_string(value);
                char *url = "/api/v1/tema/auth/register";
                char *type = "application/json";
                char **bodydata = malloc(2 * sizeof(char*));
                // se adauga stringul creat in vectorul de stringuri
                // pentru a fi dat ca argument functiei compute_request
                bodydata[0] = jsonstring;
                char *message = compute_post_request(HOST,url,type,bodydata,1,NULL,NULL);
                send_to_server(sockfd,message); // se trimite la server
                char *rec = receive_from_server(sockfd);
                strtok(rec," ");
                int status = atoi(strtok(NULL," "));
                // se ia numarul intors de request ca string si se face numar
                // pentru a verifica daca comanda a fost executata cu succes
                if(status > 300){
                    printf("Username folosit de altcineva!\n");
                }
                json_value_free(value);
                json_free_serialized_string(jsonstring);
                printf("Utilizator inregistrat cu succes!\n");
                free(username);
                free(password);
                free(bodydata);
                close(sockfd);
            }
            else if(strcmp(buff,"login\n")==0){
                char *username = malloc(50*sizeof(char));
                printf ("username=");
                fgets(username,50,stdin);
                username[strcspn(username,"\n")] = '\0';
                printf("password=");
                char *password = malloc(50*sizeof(char));
                fgets(password,50,stdin);
                password[strcspn(password,"\n")] = '\0';
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                json_object_set_string(object,"username",username);
                json_object_set_string(object,"password",password);
                char *jsonstring = json_serialize_to_string(value);
                char *url = "/api/v1/tema/auth/login";
                char *type = "application/json";
                char **bodydata = malloc(2 * sizeof(char*));
                bodydata[0] = jsonstring;
                char *message = compute_post_request(HOST,url,type,bodydata,1,NULL,NULL);
                send_to_server(sockfd,message);
                char *rec = receive_from_server(sockfd);
                char *cop = strdup(rec);
                strtok(cop," ");
                int status = atoi(strtok(NULL," "));
                if(status > 300){
                    printf("Login invalid!\n");
                    continue;
                }
                // se retine cookie-ul intors de login
                char *cookie_header = strtok(rec,";\r\n");
                while (cookie_header != NULL) {
                    if (strncmp(cookie_header, "Set-Cookie:", 11) == 0) {
                        char *cookie_value = cookie_header + 11;
                        while (*cookie_value == ' ') {
                            cookie_value++;
                        }
                        cookies = strdup(cookie_value);
                        break;
                    }
                    cookie_header = strtok(NULL, ";\r\n");
                }
                printf("Bun Venit!\n");
                free(username);
                free(password);
                free(bodydata);
                close(sockfd);
            }
            else if(strcmp(buff,"get_book\n")==0){
                if(strcmp(token,"")==0){
                    printf("Nu aveti acces la biblioteca!\n");
                    continue;
                }
                int id;
                printf("id=");
                char *idstring = calloc(10, sizeof(char));
                fgets(idstring,10,stdin);
                idstring[strcspn(idstring,"\n")]='\0';
                id = atoi(idstring);
                char *url = calloc(50,sizeof(char));
                strcat(url,"/api/v1/tema/library/books/"); 
                strcat(url,idstring);
                char *message = compute_get_request(HOST,url,NULL,cookies,token);
                send_to_server(sockfd,message);
                char *rec = receive_from_server(sockfd);
                char *coppy = strdup(rec);
                strtok(coppy, " ");
                int status = atoi(strtok(NULL," "));
                if ( status >300){
                    printf("Cartea cu id=%d nu exista!\n", id);
                    continue;
                }
                // se preia json-ul intors pentru a fi parsat
                char *book = rec + strcspn(rec,"{");
                JSON_Value *val = json_parse_string(book);
                char *normalstring = malloc(300 * sizeof(char));
                normalstring[0] = '\0';
                traverse(val,&normalstring); 
                printf("%s\n", normalstring);
                free(idstring);
                free(normalstring);
                close(sockfd);
            }
            else if(strcmp(buff,"add_book\n")==0){
                if(strcmp(token,"")==0){
                    printf("Nu aveti acces la biblioteca!\n");
                    continue;
                }
                printf("title=");
                char *title = malloc(50*sizeof(char));
                char *author = malloc(50*sizeof(char));
                char *genre = malloc(50*sizeof(char));
                char *publisher = malloc(50*sizeof(char));
                char *pagestring = malloc(10*sizeof(char));
                fgets(title,50,stdin);
                title[strcspn(title,"\n")] = '\0';
                printf("author=");
                fgets(author,50,stdin);
                author[strcspn(author,"\n")] = '\0';
                printf("genre=");
                fgets(genre,50,stdin);
                genre[strcspn(genre,"\n")] = '\0';
                printf("publisher=");
                fgets(publisher,50,stdin);
                publisher[strcspn(publisher,"\n")] = '\0';
                printf("page_count=");
                fgets(pagestring,10,stdin);
                int pagecount = atoi(pagestring);
                if (pagecount == 0 && strcmp(pagestring,"0")!=0){
                    printf("Wrong input, page_count can only be a number!\n");
                    continue;
                }
                if(strcmp(title,"")==0 || strcmp(author,"")==0 || strcmp(genre,"")==0 || strcmp(publisher,"")==0){
                    printf("Not all fields completed!\n");
                    continue;
                }
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                json_object_set_string(object,"title",title);
                json_object_set_string(object,"author",author);
                json_object_set_string(object,"genre",genre);
                json_object_set_number(object,"page_count",pagecount);
                json_object_set_string(object,"publisher",publisher);
                char *jsonstring = json_serialize_to_string(value);
                char *url = "/api/v1/tema/library/books";
                char *type = "application/json";
                char **bodydata = malloc(2 * sizeof(char*));
                bodydata[0] = jsonstring;
                char *message = compute_post_request(HOST,url,type,bodydata,1,cookies,token);
                send_to_server(sockfd,message);
                char *rec = receive_from_server(sockfd);
                free(title);
                free(author);
                free(genre);
                free(publisher);
                free(pagestring);
                free(bodydata);
                close(sockfd);
            }
            else if(strcmp(buff,"delete_book\n")==0){
                if(strcmp(token,"")==0){
                    printf("Nu aveti acces la biblioteca!\n");
                    continue;
                }
                printf("id=");
                char *idstring = malloc(10*sizeof(char));
                fgets(idstring,10,stdin);
                idstring[strcspn(idstring,"\n")] = '\0';
                int id = atoi(idstring); 
                char *url = calloc(50,sizeof(char));
                strcat(url,"/api/v1/tema/library/books/");
                strcat(url,idstring);
                char *message = compute_delete_request(HOST,url,cookies,token);
                send_to_server(sockfd,message);
                char *rec = receive_from_server(sockfd);
                strtok(rec, " ");
                int status = atoi(strtok(NULL," "));
                if (status > 300){
                    printf("Id invalid!\n");
                    continue;
                }
                free(idstring);
                free(url);
                close(sockfd);
            }
            else if(strcmp(buff,"enter_library\n")==0){
                if(strcmp(cookies,"")==0){
                    printf("Nu sunteti autentificat!\n");
                    continue;
                }
                char *url = "/api/v1/tema/library/access";
                char *get = compute_get_request(HOST,url,NULL,cookies,NULL);
                send_to_server(sockfd,get);
                char *rec = receive_from_server(sockfd);
                token = strdup(extract_token(rec + strcspn(rec,"{")));
                close(sockfd);
            }
            else if(strcmp(buff,"get_books\n")==0){
                if(strcmp(token,"")==0){
                    printf("Nu aveti acces la biblioteca!\n");
                }
                char *url = "/api/v1/tema/library/books";
                char *message = compute_get_request(HOST,url,NULL,cookies,token);
                send_to_server(sockfd,message);
                char *rec = receive_from_server(sockfd);
                char *books = rec + strcspn(rec,"[");
                JSON_Value *val = json_parse_string(books);
                JSON_Array *array = json_value_get_array(val);
                size_t count = json_array_get_count(array);
                // se parseaza vectorul de carti
                char *normalstring = (char *) malloc(1000 * sizeof(char));
                normalstring[0]='\0';
                for (size_t i = 0; i < count; i++)
                {
                    JSON_Value *jval = json_array_get_value(array,i);
                    traverse(jval,&normalstring);
                    if( i < count - 1){
                        strcat(normalstring, " \n");
                    }
                }
                
                printf("%s\n", normalstring);    
                free(normalstring);
                close(sockfd);

            }
            else if(strcmp(buff,"logout\n")==0){
                if (strcmp(cookies,"")==0){
                    printf("Nu sunteti autentificat!\n");
                    continue;
                }
                char *url = "/api/v1/tema/auth/logout";
                char *message = compute_get_request(HOST,url,NULL,cookies,NULL);
                send_to_server(sockfd,message);
                strcpy(cookies,"");
                strcpy(token,"");
                close(sockfd);
            }
        }
    }
}