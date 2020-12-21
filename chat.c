#include "chat.h"

volatile int nb_clients = 0;
int first_free = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
s_client *clients[MAX_CLIENTS];


int send_msg(int sock,char *msg)
{
    return write(sock,msg,strlen(msg));
}


int send_all(char *msg, int not_to)
{
    int i;
	
    pthread_mutex_lock(&mutex);	// debut de la section critique
    for(i=0;i<first_free;i++)
    {
	if(clients[i]->sock != not_to)
	    send_msg(clients[i]->sock,msg);
    }
    pthread_mutex_unlock(&mutex);	// fin de la section critique
	
    return 0;
}


void client_quit(s_client *me, char *msg)
{
    int i,j;
    char buf[8192+1];
	
    if(msg)
	snprintf(buf,8192,"%s nous quitte...(%s)\n",me->pseudo,msg);
    else
	snprintf(buf,8192,"%s nous quitte...\n",me->pseudo);

    buf[8192] = '\0';
    send_all(buf,me->sock);

    pthread_mutex_lock(&mutex);	// debut de la section critique
    for(i=0;(clients[i]->sock != me->sock);i++);	// recherche de l'index de la structure dans le tableau
	
    close(me->sock);
    free(me->pseudo);
    free(me);
	
    for(j=i+1;j<first_free;j++)	// on reorganise le tableau en decalant les elements situes APRES celui qui est supprime
    {
	clients[j-1] = clients[j];
    }
    nb_clients--;
    first_free--;
    pthread_mutex_unlock(&mutex);	// fin de la section critique

    printf("Un client en moins...%d clients\n",nb_clients);
}


void *chat(void *param)
{
    int sck = *((int *) param) , len , i , valid_command , valid_pseudo , trouve ;
    char msg_to_send[9000] , *buf = NULL , msg[5000] , *old_pseudo = NULL , *pseudo ;
    s_client *me = NULL , *client;
    pthread_t th_id ;
	
    me = (s_client *) malloc(sizeof(s_client));
    
    if(!me)
    {
	printf("\nErreur d'allocation memoire!\n");
	close(sck);
	nb_clients--;
	pthread_exit(NULL);
    }
    
    bzero(me,sizeof(s_client));
	
    send_msg(sck,CLIENT_BANNER);
    len = read(sck,msg,4096);

    if(len <= 0)
    {
	printf("\nErreur\n");
	close(sck);
	free(me);
	me = NULL;
	nb_clients--;
	pthread_exit(NULL);
    }

    msg[255] = '\0';	// on limite le pseudo a 255 caracteres

    for(i=0;(msg[i]!='\0') && (msg[i]!='\r') && (msg[i]!='\n') && (msg[i]!='\t');i++);

    msg[i] = '\0';	// on isole le pseudo
	
    pthread_mutex_lock(&mutex);	// debut de la section critique
    for(i=0;i<first_free;i++)
    {
	if(!strcmp(msg,clients[i]->pseudo))
	{
	    send_msg(sck,"\nPseudo deja utilise! Deconnection...\r\n");
	    close(sck);
	    free(me);
	    nb_clients--;
	    pthread_mutex_unlock(&mutex);	// fin de la section critique
	    pthread_exit(NULL);
	}
    }
    pthread_mutex_unlock(&mutex);	// fin de la section critique
	
    me->id = pthread_self();
    me->sock = sck;
    me->pseudo = strdup(msg);
    me->admin = 0;
	
    pthread_mutex_lock(&mutex);	// debut de la section critique
    clients[first_free] = me;
    first_free++;
    pthread_mutex_unlock(&mutex);	// fin de la section critique
	
    snprintf(msg_to_send,8192,"Nouveau client : %s\r\n",me->pseudo);
    msg_to_send[8192]='\0';
    send_all(msg_to_send,INVALID_SOCKET);

    while(1)
    {
	len = read(sck,msg,4096);
	if(len <= 0)
	{
	    client_quit(me,"Erreur reseau");
	    pthread_exit(NULL);
	}
	msg[len] = '\0';

	if(msg[0] == '$')	// le message est une commande
	{
	    valid_command = 0;
			
	    if(!strncmp(msg,"$pseudo=",8))	// changement de pseudo
	    {
		valid_pseudo = 1;
				
		msg[255+8] = '\0';	// on limite le pseudo a 255 caracteres
		for(i=8;(msg[i]!='\0') && (msg[i]!='\n') && (msg[i]!='\t');i++);
		msg[i] = '\0';	// on isole le pseudo
				
				/* on verifie que le nouveau pseudo n'existe pas deja */

		pthread_mutex_lock(&mutex);	// debut de la section critique
		for(i=0;i<first_free;i++)
		{
		    if(!strcmp(&msg[8],clients[i]->pseudo))
			valid_pseudo = 0;
		}
		pthread_mutex_unlock(&mutex);	// fin de la section critique
								
		if(valid_pseudo)
		{
		    old_pseudo = me->pseudo;
		    me->pseudo = strdup(&msg[8]);
		    snprintf(msg_to_send,8192,"%s s'appelle maintenant %s\n",old_pseudo,me->pseudo);
		    free(old_pseudo);
		    send_all(msg_to_send,INVALID_SOCKET);
		}
		else
		    send_msg(sck,"Pseudo deja utilise!\n");
		valid_command = 1;
	    }
	    else if(!strncmp(msg,"$quit",5))	// sortie "propre" du serveur (avec message)
	    {		
		if(msg[5]=='=')
		{
		    for(i=6;(msg[i]!='\0') && (msg[i]!='\n') && (msg[i]!='\t');i++);
		    msg[i]='\0';
		    client_quit(me,&msg[6]);
		}

		else client_quit(me,NULL);
		pthread_exit(NULL);
	    }
	    
	    else if(!strncmp(msg,"$list",5))	// obtenir la liste des pseudos sur le serveur
	    {
		pthread_mutex_lock(&mutex);	// debut de la section critique
		for(i=0;i<first_free;i++)
		{
		    send_msg(me->sock,clients[i]->pseudo);
		    send_msg(me->sock,"\r\n");
		}
		pthread_mutex_unlock(&mutex);	// fin de la section critique
		valid_command = 1;
	    }

	    else if(!strncmp(msg,"$admin=",7))	// droits admin (avec mot de passe)
	    {
		if(!strncmp(&msg[7],ADMIN_PWD,strlen(ADMIN_PWD)))
		{
		    send_msg(me->sock,"Droits administrateur actives.\r\n");
		    me->admin = 1;
		}
		else send_msg(me->sock,"Mot de passe incorrect!\r\n");
		valid_command = 1;
	    }

	    else if(!strncmp(msg,"$eject=",7))
	    {
		if(me->admin)
		{
		    pseudo = &msg[7];
		    trouve = 0;
					
		    pseudo[255+8] = '\0';	// on limite le pseudo a 255 caracteres
		    for(i=0;(pseudo[i]!='\0') && (pseudo[i]!='\r') && (pseudo[i]!='\n') && (pseudo[i]!='\t');i++);
		    pseudo[i] = '\0';	// on isole le pseudo

					/* on cherche si le pseudo existe */

		    pthread_mutex_lock(&mutex);	// debut de la section critique
		    for(i=0;i<first_free;i++)
		    {
			if(!strcmp(pseudo,clients[i]->pseudo))
			{
			    trouve = 1;
			    if(!clients[i]->admin)
			    {
				th_id = clients[i]->id;
				client = clients[i];
				
				pthread_mutex_unlock(&mutex);
				send_msg(client->sock,"Vous etes ejecte par un admin\n");
				client_quit(client,"Ejecter par un admin");	
				pthread_cancel(th_id);	// termine le thread correspondant
				send_msg(me->sock,"Le client a ete ejecte!\n");
				break;
			    }
			    else send_msg(me->sock,"Impossible d'ejecter un admin!\n");
			}
		    }
		    pthread_mutex_unlock(&mutex);	// fin de la section critique
					
		    if(!trouve)
			send_msg(me->sock,"Impossible de trouver le pseudo!\n");
		}
		
		else send_msg(me->sock,"Cette commande necessite les droits administrateur!\r\n");
		valid_command = 1;
	    }
	    
	    else if(!strncmp(msg,"$help",5))
	    {
		send_msg(me->sock,HELP_MSG);
		valid_command = 1;
	    }
				
	    if(!valid_command)	// commande invalide
		send_msg(sck,"Commande non valide!\n");
	}

	else			// message normal
	{	snprintf(msg_to_send,8192,"%s : %s",me->pseudo,msg);
	    msg_to_send[8192] = '\0';
	    send_all(msg_to_send,me->sock);
	}
    }
	
    return NULL;
}



int main(int argc, char **argv)
{
    int server,sck;
    pthread_t th_id;
	
    printf(BANNER);
	
    server = create_server(PORT);
    while(1)
    {
	sck = server_accept(server,0);
	if(sck == INVALID_SOCKET)
	{
	    printf("\nErreur de accept()!\n");
	    exit(-1);
	}

	if(nb_clients < MAX_CLIENTS)
	{
	    pthread_create(&th_id,NULL,chat,(void *)&sck);
	    nb_clients++;
	    printf("Nouveau client! %d clients\n",nb_clients);
	}

	else close(sck);
    }
	
    return 0;
}
