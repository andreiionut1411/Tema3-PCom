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

#define HOST "34.241.4.235"
#define PORT 8080
#define JSON_TYPE "application/json"
#define AUTH_STRING "Authorization: Bearer "
#define URL_REGISTER "/api/v1/tema/auth/register"
#define URL_LOGIN "/api/v1/tema/auth/login"
#define URL_ACCESS "/api/v1/tema/library/access"
#define URL_BOOKS_LIBRARY "/api/v1/tema/library/books"
#define URL_LOGOUT "/api/v1/tema/auth/logout"

char buffer[LINELEN];

// Functia ii afiseaza clientului sa se conecteze cu un username si o parola
// si intoarce credentialele acestuia sub forma unui sir de caractere care
// reprezinta varianta JSON a acestuia.
char* post_with_authentication (){
    char body_data[2][LINELEN];
    int len;

    // Primim username-ul de la utilizator.
    fprintf (stdout, "username=");
    fgets (body_data[0], LINELEN, stdin);
    len = strlen (body_data[0]);

    for (int i = 0; i < len; i++){
        if (body_data[0][i] == ' '){
            fprintf (stderr, "Spaces are not allowed in username, try again.\n");
            return NULL;
        }
    }

    body_data[0][strlen (body_data[0]) - 1] = '\0';

    // Primim parola de la utilizator.
    fprintf (stdout, "password=");
    fgets (body_data[1], LINELEN, stdin);
    len = strlen (body_data[1]);

    for (int i = 0; i < len; i++){
        if (body_data[1][i] == ' '){
            fprintf (stderr, "Spaces are not allowed in password, try again.\n");
            return NULL;
        }
    }

    body_data[1][strlen (body_data[1]) - 1] = '\0';

    // Initializam obiectele de tip JSON cu care vom ajunge sa lucram.
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object (root_value);

    // Introducem in obiectul de tip JSON informatiile de care avem nevoie
    char* serializare = NULL;
    json_object_set_string (root_object, "username", body_data[0]);
    json_object_set_string (root_object, "password", body_data[1]);

    serializare = json_serialize_to_string (root_value);

    json_value_free (root_value);

    return serializare;
}

// Functia primeste raspunsul de la server si daca a aparut o eroare de la
// acesta, atunci informam si utilizatorul despre ea. Daca totul s-a facut
// cu succes, atunci afisam 200 - OK pentru a informa utilizatorul.
int print_error_from_server (char *response){
    JSON_Value *root_value;
    JSON_Object *root_object;
    char* json_response = basic_extract_json_response (response);

    if (response == NULL){
        fprintf (stderr, "No response from server.\n");
        return -1;
    }

    root_value = json_parse_string (json_response);
    root_object = json_value_get_object (root_value);
    const char* error = json_object_get_string (root_object, "error");

    // Daca raspunsul de la final dat de server este NULL, inseamna ca totul
    // s-a facut cu succes, altfel afisam eroarea data de server, care ne
    // indica faptul ca utilizatorul deja exista.
    if (error != NULL){
        printf ("Error: %s\n", error);
        json_value_free (root_value);
        return -1;
    }
    else{
        printf ("200 - OK\n");
    }

    json_value_free (root_value);
    return 0;
}

// Functia primeste cookie-ul de logare si tokenul JWT de autentificare la
// biblioteca, iar apoi intoarce un sir de 2 string-uri care reprezinta
// headerele ce trebuie adaugate in requestul de HTTP, pe langa cele standard.
char **create_cookie_and_auth_headers (char *cookie, char *token_auth,
                                       int *cookie_count){
    char **cookies;
    char *auth_header;
    int len_auth = strlen (AUTH_STRING);
    *cookie_count = 0;

    if (cookie != NULL){
        (*cookie_count)++;
    }

    if (token_auth != NULL){
        (*cookie_count)++;
    }

    cookies = malloc ((*cookie_count) * sizeof(char *));
    if (cookies == NULL){
        fprintf (stderr, "There was an error at allocating memory.\n");
        return NULL;
    }

    // Daca avem cookie-ul, il adaugam in lista de headere ce trebuie adaugate
    // la request.
    if (cookie != NULL){
        cookies[0] = malloc (strlen (cookie) + strlen ("Cookie: ") + 1);
        if (cookies[0] == NULL){
            fprintf (stderr, "There was an error at allocating memory.\n");
            free (cookies);
            return NULL;
        }

        strcpy (cookies[0], "Cookie: ");
        strcat (cookies[0], cookie);
    }

    if (token_auth != NULL){
        auth_header = malloc (len_auth + strlen (token_auth) + 1);
        if (auth_header == NULL){
            fprintf (stderr, "There was an error at allocating memory.\n");
            free (cookies[0]);
            free (cookies);
            return NULL;
        }

        strcpy (auth_header, AUTH_STRING);
        strcat (auth_header, token_auth);
        cookies[(*cookie_count) - 1] = auth_header;
    }

    return cookies;
}

// Functia dezaloca memoria pentru toate headerele suplimentare pe care le-am
// adaugat in request.
void free_cookies_headers (char **cookies, int cookie_count){
    
    for (int i = 0; i < cookie_count; i++){
        free (cookies[i]);
    }

    free (cookies);
}


// Functia dezaloca memoria ramasa pentru anumite siruri de caractere care
// apar in mai multe functii.
void free_memory (char *message, char *response, char *serializare, 
                  char **cookies, int cookie_count){
    if (message != NULL){
        free (message);
    }

    if (response != NULL){
        free (response);
    }

    if (serializare != NULL){
        free (serializare);
    }

    if (cookie_count > 0){
        free_cookies_headers (cookies, cookie_count);
    }
}

// Functia face un request pentru a se inregistra un nou cont.
void make_register_request (){
    char *message;
    char *response;
    char *serializare;

    // Cream socketul TCP pe care vom comunica cu serverul.
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    serializare = post_with_authentication();
    if (serializare == NULL){
        return;
    }
    
    // Cream mesajul de tip POST de care avem nevoie si il trimitem apoi la
    // server.
    message = compute_post_request (HOST, URL_REGISTER, JSON_TYPE,
                                    serializare, NULL, 0);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);

    // Daca server-ul ne-a dat o eroare, atunci o afisam.
    print_error_from_server(response);

    free_memory (message, response, serializare, NULL, 0);
    close_connection (sockfd);
}

// Functia ii afiseaza utilizatorului un prompt pentru username si parola, iar
// daca credentialele au fost cele corecte, se returneaza cookie-ul pentru
// autentificare, sau NULL, in caz contrar.
char* make_login_request (char *cookie_extern){
    char *serializare = post_with_authentication ();
    char *message;
    char *response;

    if (serializare == NULL){
        return NULL;
    }

    // Daca utilizatorul era deja logat, atunci il anuntam.
    if (cookie_extern != NULL){
        fprintf (stderr, "Error: You are already logged in.\n");
        return cookie_extern;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    // Cream mesajul de tip POST de care avem nevoie si il trimitem apoi la
    // server.
    message = compute_post_request (HOST, URL_LOGIN, JSON_TYPE,
                                    serializare, NULL, 0);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);

    // Daca server-ul ne-a dat o eroare, atunci o afisam.
    int ret = print_error_from_server (response);

    // Daca nu s-a reusit logarea ne oprim.
    if (ret == -1){
        return NULL;
    }

    int cur_index_in_response = 0;
    int response_len = strlen (response);
    int start_index = -1;
    int finish_index = -1;
    char *cookie = NULL;
    
    // Cautam in raspuns header-ul pentru cookie-uri. Stim ca fiecare linie se
    // termina cu \r\n, asa ca dupa aceste caractere ne vom imparti noi liniile.
    // Dupa ce am gasit linia care seteaza cookie-urile, le retinem.
    while (cur_index_in_response + 12 <= response_len){
        if (strncmp (response + cur_index_in_response, "Set-Cookie: ", 12) == 0){
            start_index = cur_index_in_response + 12; 
        }

        while (response[cur_index_in_response] != '\r'){
            cur_index_in_response++;
        }

        if (start_index != -1){
            finish_index = cur_index_in_response - 1;
            break;
        }

        cur_index_in_response += 2;
    }

    if (start_index == -1){
        fprintf (stderr, "Error: Cookie was not received.\n");
        return NULL;
    }

    cookie = malloc (finish_index - start_index + 2);
    if (cookie == NULL){
        fprintf (stderr, "There was an error at allocating memory.\n");
        return NULL;
    }

    memcpy (cookie, response + start_index, (finish_index - start_index) + 1);
    cookie[finish_index - start_index + 1] = '\0';

    free_memory (message, response, serializare, NULL, 0);
    close_connection (sockfd);

    return cookie;
}

// Functia face un request serverului pentru enter_library si intoarce un
// sir de caractere care reprezinta tokenul JWT primit de la server.
char* make_access_request (char *cookie){
    char *message;
    char *response = NULL;
    char **cookies;
    char *token;
    int cookie_count = 0;

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    cookies = create_cookie_and_auth_headers (cookie, NULL, &cookie_count);

    message = compute_get_request (HOST, URL_ACCESS, NULL, cookies, cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    int ret = print_error_from_server (response); 
    // Afisam daca a aparut vreo eroare de la server.

    if (ret == -1){
        return NULL;
    }

    JSON_Value *root_value;
    JSON_Object *root_object;
    char* json_response = basic_extract_json_response (response);

    root_value = json_parse_string (json_response);
    root_object = json_value_get_object (root_value);
    int len = strlen (json_object_get_string (root_object, "token"));
    token = malloc (len + 1); // Alocam memorie si pentru \0.
    strcpy (token, json_object_get_string(root_object, "token"));

    json_value_free (root_value);
    free_memory (message, response, NULL, cookies, cookie_count);
    close_connection (sockfd);

    return token;
}

// Functia trimite un request serverului pentru comanda get_books si afiseaza
// la stdout toate informatiile despre cartile din biblioteca.
void get_books_in_library (char *cookie, char *token_auth){
    char *message;
    char *response;
    char **cookies;
    int cookie_count = 0;

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    cookies = create_cookie_and_auth_headers (cookie, token_auth, &cookie_count);

    // Daca nu am putut obtine headere-le pentru token si cookie, atunci nu putem
    // trimite mai departe requestul.
    if (cookies == NULL){
        return;
    }

    message = compute_get_request (HOST, URL_BOOKS_LIBRARY, NULL, cookies, 
                                  cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    int ret = print_error_from_server (response); 

    if (ret == -1){
        return;
    }
    // Afisam daca a aparut vreo eroare de la server.

    JSON_Value *root_value;
    JSON_Array *books_array;
    char* json_response = extract_json_array (response);

    root_value = json_parse_string (json_response);
    books_array = json_value_get_array (root_value);

    if (json_array_get_count (books_array) == 0){
        printf ("There are no books in the library yet.\n");
    }

    for (int i = 0; i < json_array_get_count (books_array); i++){
        JSON_Object *book = json_array_get_object (books_array, i);
        printf ("title: %s and id: %d\n", json_object_get_string (book, "title"),
                (int)json_object_get_number (book, "id"));
    }

    json_value_free (root_value);
    free_memory (message, response, NULL, cookies, cookie_count);
    close_connection (sockfd);
}

// Functia trimite la server request pentru comanda get_book si afiseaza
// rezultatele primite de la server dupa ce au fost interpretate de biblioteca
// de JSON.
void get_book (char *cookie, char *token_auth){
    char *message;
    char *response;
    char id[LINELEN];
    char **cookies;
    int cookie_count = 0;

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);

    printf ("id=");
    fgets (id, LINELEN, stdin);

    if (id[strlen (id) - 1] == '\n'){
        id[strlen (id) - 1] = '\0';
    }

    for (int i = 0; i < strlen (id); i++){
        if (id[i] < '0' || id[i] > '9'){
            fprintf (stderr, "Id is a number, please try again.\n");
            return;
        }
    }

    char *final_url = malloc (strlen (id) + strlen (URL_BOOKS_LIBRARY) + 2);
    if (final_url == NULL){
        fprintf (stderr,  "There was an error at allocating memory.\n");
        return;
    }
    strcpy (final_url, URL_BOOKS_LIBRARY);
    strcat (final_url, "/");
    strcat (final_url, id);

    // Cream headerele ce trebuie adaugate requestului.
    cookies = create_cookie_and_auth_headers (cookie, token_auth, &cookie_count);
    if (cookies == NULL){
        return;
    }

    message = compute_get_request (HOST, final_url, NULL, cookies, cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    int ret = print_error_from_server (response); 
    // Afisam daca a aparut vreo eroare de la server.

    if (ret == -1){
        return;
    }

    JSON_Value *root_value;
    JSON_Object *book;
    char* json_response = basic_extract_json_response (response);

    root_value = json_parse_string (json_response);
    book = json_value_get_object (root_value);

    // Parsam raspunsul serverului.
    printf ("title = %s\n", json_object_get_string (book, "title"));
    printf ("author = %s\n", json_object_get_string (book, "author"));
    printf ("genre = %s\n", json_object_get_string (book, "genre"));
    printf ("publisher = %s\n", json_object_get_string (book, "publisher"));
    printf ("page count = %d\n", (int)json_object_get_number (book, "id"));
    
    close_connection (sockfd);
    json_value_free (root_value);
    free (final_url);
    free_memory (message, response, NULL, cookies, cookie_count);
}

// Functia primeste un obiect JSON si tipul de camp pe care vrem sa-l citim.
// Apoi, functia ii afiseaza un prompt utilizatorului ca sa completeze
// campul dorit, iar raspunsul este adaugat in obiectul JSON.
// Daca utilizatorul nu introduce nimic de la tastatura, atunci functia
// returneaza 1 pentru a indica o eroare.
int add_string_to_json (JSON_Object *root_object, char *field){
    char buffer[LINELEN];

    memset (buffer, 0, LINELEN);
    printf ("%s=", field);
    fgets (buffer, LINELEN, stdin);

    // Eliminam endline-ul de la finalul sirului citit de la tastatura.
    if (buffer[strlen (buffer) - 1] == '\n'){
        buffer[strlen (buffer) - 1] = '\0';
    }

    if (strlen (buffer) == 0){
        return 1;
    }

    // Adaugam ce am primit de la utilizator in json.
    json_object_set_string (root_object, field, buffer);
    return 0;
}

// Functia primeste un obiect JSON si numele campului pe care utilizatorul
// este pus sa il completeze. Mai intai, se verifica daca utilizatorul a
// introdus intr-adevar un numar, iar apoi se adauga acesta in obiectul JSON.
int add_number_to_json (JSON_Object *root_object, char *field){
    char buffer[LINELEN];
    int len;
    int is_number = 1;

    memset (buffer, 0, LINELEN);
    printf ("%s=", field);
    fgets (buffer, LINELEN, stdin);

    // Eliminam endline-ul de la finalul sirului citit de la tastatura.
    if (buffer[strlen (buffer) - 1] == '\n'){
        buffer[strlen (buffer) - 1] = '\0';
    }

    len = strlen (buffer);
    for (int i = 0; i < len && is_number == 1; i++){
        if (!(buffer[i] >= '0' && buffer[i] <= '9')){
            is_number = 0;
        }
    }

    if (is_number == 0 || len == 0){
        return 1;
    }

    // Adaugam numarul in json.
    json_object_set_number (root_object, field, atoi (buffer));
    return 0;
}

// Functia trimite un request la server de tip POST pentru a adauga cartea la
// biblioteca.
void add_book (char *cookie, char *token_auth){
    char *message;
    char *response;
    char **cookies;
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object (root_value);
    char *serializare;
    int cookie_count = 0;
    int ret = 0;

    // Oferim utilizatorului un prompt in care sa treaca informatiile despre
    // carte pe care le doreste.
    ret = ret || add_string_to_json (root_object, "title");
    ret = ret || add_string_to_json (root_object, "author");
    ret = ret || add_string_to_json (root_object, "genre");
    ret = ret || add_string_to_json (root_object, "publisher");
    ret = ret || add_number_to_json (root_object, "page_count");

    // Daca nu s-au introdus corect datele nu mai adaugam cartea.
    if (ret == 1){
        fprintf (stderr, "The information was wrong or incomplete, try again.\n");
        json_value_free (root_value);
        return;
    }

    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    serializare = json_serialize_to_string (root_value);
    cookies = create_cookie_and_auth_headers (cookie, token_auth, &cookie_count);
    if (cookies == NULL){
        return;
    }
    
    message = compute_post_request (HOST, URL_BOOKS_LIBRARY, JSON_TYPE, 
                                    serializare, cookies, cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    ret = print_error_from_server (response); 
    // Afisam daca a aparut vreo eroare de la server.

    if (ret == -1){
        return;
    }

    json_free_serialized_string (serializare);
    json_value_free (root_value);
    free_memory (message, response, NULL, cookies, cookie_count);
    close_connection (sockfd);
}

// Functia ii ofera un prompt clientului sa completeze cu id-ul cartii pe care
// doreste sa o elimine, iar apoi trimite un request la server sa stearga cartea.
void delete_book (char *cookie, char *token_auth){
    char *message;
    char *response;
    char **cookies;
    char id[LINELEN];
    int cookie_count = 0;

    if (cookie == NULL || token_auth == NULL){
        fprintf (stderr, "You are not logged in, please try again.\n");
        return;
    }

    printf ("id=");
    fgets (id, LINELEN, stdin);

    if (id[strlen (id) - 1] == '\n'){
        id[strlen (id) - 1] = '\0';
    }

    for (int i = 0; i < strlen (id); i++){
        if (id[i] < '0' || id[i] > '9'){
            fprintf (stderr, "Id is a number, please try again.\n");
            return;
        }
    }

    cookies = create_cookie_and_auth_headers (cookie, token_auth, &cookie_count);
    if (cookies == NULL){
        return;
    }

    int sockfd = open_connection (HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *final_url = malloc (strlen (URL_BOOKS_LIBRARY) + strlen(id) + 2);
    if (final_url == NULL){
        fprintf (stderr,  "There was an error at allocating memory.\n");
        return;
    }
    strcpy (final_url, URL_BOOKS_LIBRARY);
    strcat (final_url, "/");
    strcat (final_url, id);

    message = compute_delete_request (HOST, final_url, cookies, cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    int ret = print_error_from_server (response); 
    // Afisam daca a aparut vreo eroare de la server.

    if (ret == -1){
        return;
    }

    close_connection (sockfd);
    free (final_url);
    free_memory (message, response, NULL, cookies, cookie_count);
}

// Pentru a face logout trimitem requestul corespunzator la server, si daca
// am primit ca parametri un cookie de sesiune si un token JWT, atunci le
// dezalocam memoria si le facem sa pointeze catre NULL. Astfel, utilizatorul
// nu va mai avea acces la biblioteca in continuare.
void logout (char **cookie, char **token_auth){
    char *message;
    char *response;
    char **cookies;
    int cookie_count;

    cookies = create_cookie_and_auth_headers (*cookie, *token_auth, &cookie_count);

    int sockfd = open_connection (HOST, PORT, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request (HOST, URL_LOGOUT, NULL, cookies, cookie_count);
    send_to_server (sockfd, message);
    response = receive_from_server (sockfd);
    int ret = print_error_from_server (response); 
    // Afisam daca a aparut vreo eroare de la server.

    if (ret == -1){
        return;
    }

    close_connection (sockfd);
    free_memory (message, response, NULL, cookies, cookie_count);

    if (*cookie != NULL){
        free (*cookie);
        *cookie = NULL;
    }

    if (*token_auth != NULL){
        free (*token_auth);
        *token_auth = NULL;
    }
}

int main(int argc, char *argv[])
{
    int is_open = 1;
    char *cookie_auth = NULL;
    char *token_auth = NULL;

    setvbuf (stdout, NULL, _IONBF, 0);

    while (is_open){
        fgets (buffer, LINELEN, stdin);
        buffer[strlen(buffer) - 1] = '\0'; // Stergem \n de la finalul sirului.

        if (strcmp (buffer, "register") == 0){
            make_register_request ();
        }
        else if (strcmp (buffer, "login") == 0){
            cookie_auth = make_login_request (cookie_auth);
        }
        else if (strcmp (buffer, "enter_library") == 0){
            token_auth = make_access_request (cookie_auth);
        }
        else if (strcmp (buffer, "get_books") == 0){
            get_books_in_library (cookie_auth, token_auth);
        }
        else if (strcmp (buffer, "get_book") == 0){
            get_book (cookie_auth, token_auth);
        }
        else if (strcmp (buffer, "add_book") == 0){
            add_book (cookie_auth, token_auth);
        }
        else if (strcmp (buffer, "delete_book") == 0){
            delete_book (cookie_auth, token_auth);
        }
        else if (strcmp (buffer, "logout") == 0){
            logout (&cookie_auth, &token_auth);
        }
        else if (strcmp (buffer, "exit") == 0){
            is_open = 0;
            
            if (cookie_auth != NULL){
                free (cookie_auth);
            }

            if (token_auth != NULL){
                free (token_auth);
            }
        }
        else{
            fprintf (stderr, "Nu este o comanda admisibila, incercati din nou.\n");
        }
    }

    return 0;
}