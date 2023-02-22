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
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Scriem tipul de request si il punem la inceput. Verificam si daca avem
    // parametrii pentru query, si daca avem, atunci ii adaugam.
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Adaugam numele hostului.
    memset (line, 0, LINELEN);
    sprintf (line, "Host: %s", host);
    compute_message (message, line);

    // Adaugam headere-le suplimentare care trebuie adaugate la request. Deci,
    // in mod normal, un singur header va fi cel de cookie-uri din vectorul cookies,
    // iar restul sunt headerele suplimentare care trebuie adaugate.
    for (int i = 0; i < cookies_count; i++) {
        memset (line, 0, LINELEN);
        sprintf (line, "%s", cookies[i]);
        compute_message (message, line);
    }
    // Adaugam linia finala.
    compute_message(message, "");
    free (line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *body_data,
                           char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Scriem tipul de request si il punem la inceput. Verificam si daca avem
    // parametrii pentru query, si daca avem, atunci ii adaugam.
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Adaugam numele hsotului.
    memset (line, 0, LINELEN);
    sprintf (line, "Host: %s", host);
    compute_message (message, line);

    // Adaugam tipul de payload si lungimea contentului pe care urmeaza sa-l
    // trimitem.
    memset (line, 0, LINELEN);
    sprintf (line, "Content-Type: %s", content_type);
    compute_message (message, line);

    int len = strlen (body_data);
    memset (line, 0, LINELEN);
    sprintf (line, "Content-Length: %d", len);
    compute_message (message, line);

    // Adaugam headere-le suplimentare care trebuie adaugate la request. Deci,
    // in mod normal, un singur header va fi cel de cookie-uri din vectorul cookies,
    // iar restul sunt headerele suplimentare care trebuie adaugate.
    if (cookies != NULL) {
        for (int i = 0; i < cookies_count; i++){
            memset (line, 0, LINELEN);
            sprintf (line, "%s", cookies[i]);
            compute_message (message, line);
        }
    }
    // Adaugam o linie intre headere si payload.
    compute_message (message, "");

    // Adaugam payloadul propriu zis.
    memset(line, 0, LINELEN);
    compute_message(message, body_data);

    free(line);
    return message;
}

char *compute_delete_request (char *host, char *url, char **cookies, int cookie_count){
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Scriem tipul de request si il punem la inceput. Verificam si daca avem
    // parametrii pentru query, si daca avem, atunci ii adaugam.
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Adaugam numele hostului.
    memset (line, 0, LINELEN);
    sprintf (line, "Host: %s", host);
    compute_message (message, line);

    // Adaugam headere-le suplimentare care trebuie adaugate la request. Deci,
    // in mod normal, un singur header va fi cel de cookie-uri din vectorul cookies,
    // iar restul sunt headerele suplimentare care trebuie adaugate.
    for (int i = 0; i < cookie_count; i++){
        memset (line, 0, LINELEN);
        sprintf (line, "%s", cookies[i]);
        compute_message (message, line);
    }

    // Adaugam o linie goala la final.
    compute_message(message, "");
    free (line);

    return message;
}
