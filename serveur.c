#include "chat.h"

int create_server(int port)
{
    int sock,optval = 1;
    struct sockaddr_in sockname;

    if((sock = socket(PF_INET,SOCK_STREAM,0))<0)
    {
	printf("Erreur d'ouverture de la socket");
	exit(-1);
    }
  
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int));
    memset((char *) &sockname,0,sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(port);
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock,(struct sockaddr *) &sockname, sizeof(struct sockaddr_in)) < 0)
    {
	printf("Erreur de bind!");
	exit(-1);
    }

    if(listen(sock,LS_CLIENT_NB) <0)
    {
	printf("listen error!");
	exit(-1);
    }
  
    return sock;
}


int affiche_adresse_socket(int sock)
{
    struct sockaddr_in adresse;
    socklen_t longueur = sizeof(struct sockaddr_in);

    if (getsockname(sock, (struct sockaddr*)&adresse, &longueur) < 0)
    {
	fprintf(stderr, "Erreur getsockname\n");
	return -1;
    }

    printf("IP = %s, Port = %u\n", inet_ntoa(adresse.sin_addr),ntohs(adresse.sin_port));

    return 0;
}


int server_accept(int main_sock,int timeout)
{
    int sock;

    if(timeout > 0)
	alarm(timeout);

    if((sock = accept(main_sock,NULL,0)) < 0)
    {
	if(errno == EINTR)
	{
	    shutdown(main_sock,SHUT_RDWR);
	    close(main_sock);
	    if(timeout > 0)
		alarm(0);
	    return -1;
	}
	else
	{
	    printf("\nAccept error.\n");
	    exit(-1);
	}
    }

    if(timeout > 0)
	alarm(0);
    fcntl(sock,F_SETFD,1);
  
    return sock;
}
