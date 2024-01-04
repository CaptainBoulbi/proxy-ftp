#include  <stdio.h>
#include  <unistd.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <stdlib.h>
#include  <stdbool.h>

#define MAXHOSTLEN 64
#define MAXPORTLEN 6
#define MAXBUFFERLEN 1024

int main(int argc, char* argv[]){
	int descSock;                  // Descripteur de la socket
	int ecode;                     // Retour des fonctions
	struct addrinfo *res,*resPtr;  // Résultat de la fonction getaddrinfo
	struct addrinfo hints;
	char serverName[MAXHOSTLEN];   // Nom de la machine serveur
	char serverPort[MAXPORTLEN];   // Numéro de port
	char buffer[MAXBUFFERLEN];     // Buffer stockant les messages entre 
                                  // le client et le serveur
	bool isConnected = false;      // booléen indiquant que l'on est bien connecté

    //On teste les valeurs rentrées par l'utilisateur
    if (argc != 3){ perror("Mauvaise utilisation de la commande: <nom serveur> <numero de port>\n"); exit(1);}
    if (strlen(argv[1]) >= MAXHOSTLEN){ perror("Le nom de la machine serveur est trop long\n"); exit(2);}
    if (strlen(argv[2]) >= MAXPORTLEN){ perror("Le numero de port du serveur est trop long\n"); exit(2);}
    strncpy(serverName, argv[1], MAXHOSTLEN);
    serverName[MAXHOSTLEN-1] = '\0';
    strncpy(serverPort, argv[2], MAXPORTLEN);
    serverPort[MAXPORTLEN-1] = '\0';


    
    // Initailisation de hints
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_family = AF_UNSPEC;      // les adresses IPv4 et IPv6 seront présentées par 
				                          // la fonction getaddrinfo

	//Récupération des informations sur le serveur
	ecode = getaddrinfo(serverName,serverPort,&hints,&res);
	if (ecode){
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	resPtr = res;

	while(!isConnected && resPtr!=NULL){

		//Création de la socket IPv4/TCP
		descSock = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
		if (descSock == -1) {
			perror("Erreur creation socket");
			exit(2);
		}
  
  		//Connexion au serveur
		ecode = connect(descSock, resPtr->ai_addr, resPtr->ai_addrlen);
		if (ecode == -1) {
			resPtr = resPtr->ai_next;    		
			close(descSock);	
		}
		// On a pu se connecté
		else isConnected = true;
	}
	freeaddrinfo(res);
	if (!isConnected){
		perror("Connexion impossible");
		exit(2);
	}
	//Echange de donneés avec le serveur
	ecode = read(descSock, buffer, MAXBUFFERLEN);
	if (ecode == -1) {perror("Problème de lecture\n"); exit(3);}
	buffer[ecode] = '\0';
	printf("MESSAGE RECU DU SERVEUR: \"%s\".\n",buffer);
	//Fermeture de la socket
	close(descSock);
}

