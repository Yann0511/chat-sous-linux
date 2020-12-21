#ifndef CHAT
#define CHAT

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

#define BANNER "Serveur de chat par :Yann0511:\n(C) :Yann0511: Decembre 2020\n"
#define CLIENT_BANNER "Connection etablie...\nBienvenue sur le serveur de chat de Yann0511\nEntrez votre pseudo: "
#define ADMIN_PWD "starboy"
#define HELP_MSG "Commandes disponibles :\n\t- $pseudo=[nouveau pseudo] --> changer de pseudo\n\t- $quit=[message] --> quitter avec (ou sans) message\n\t- $list --> obtenir la liste des clients connectes\n\t- $admin=[password] --> obtenir les droits administrateur\n\t- $eject=[pseudo] --> ejecter un client(reserve aux admins)\n\t- $help --> afficher cette aide\n"

#define MAX_CLIENTS 100
#define LS_CLIENT_NB 5
#define INVALID_SOCKET -1
#define PORT 5000

typedef struct
{
    pthread_t id;
    int sock;
    char *pseudo;
    char admin;
}s_client ;


int create_server(int port) ;

int affiche_adresse_socket(int sock) ;

int server_accept(int main_sock,int timeout) ;

int send_msg(int sock,char *msg) ;

int send_all(char *msg, int not_to) ;

void client_quit(s_client *me, char *msg) ;

void *chat(void *param) ;


#endif
