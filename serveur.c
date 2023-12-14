#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>

#define SERVADDR "localhost"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, 
                                    // si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille du tampon de demande de connexion
#define MAXBUFFERLEN 1024
#define MAXHOSTLEN 64
#define MAXPORTLEN 6

int main(){
	int ecode;                       // Code retour des fonctions
	char serverAddr[MAXHOSTLEN];     // Adresse du serveur
	char serverPort[MAXPORTLEN];     // Port du server
	int descSockRDV;                 // Descripteur de socket de rendez-vous
	int descSockCOM;                 // Descripteur de socket de communication
	struct addrinfo hints;           // Contrôle la fonction getaddrinfo
	struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
	struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
	struct sockaddr_storage from;    // Informations sur le client connecté
	socklen_t len;                   // Variable utilisée pour stocker les 
				                        // longueurs des structures de socket
	char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur
    
	// Publication de la socket au niveau du système
	// Assignation d'une adresse IP et un numéro de port
	// Mise à zéro de hints
	memset(&hints, 0, sizeof(hints));
	// Initailisation de hints
	hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
	hints.ai_socktype = SOCK_STREAM;  // TCP
	hints.ai_family = AF_UNSPEC;      // les adresses IPv4 et IPv6 seront présentées par 
				                         // la fonction getaddrinfo

	// Récupération des informations du serveur
	ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
	if (ecode) {
		fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
		exit(1);
	}

	//Création de la socket IPv4/TCP
	descSockRDV = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (descSockRDV == -1) {
		perror("Erreur creation socket");
		exit(4);
	}

	// Publication de la socket
	ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
	if (ecode == -1) {
		perror("Erreur liaison de la socket de RDV");
		exit(3);
	}
	// Nous n'avons plus besoin de cette liste chainée addrinfo
	freeaddrinfo(res);
	// Récuppération du nom de la machine et du numéro de port pour affichage à l'écran

	len=sizeof(struct sockaddr_storage);
	ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
	if (ecode == -1)
	{
		perror("SERVEUR: getsockname");
		exit(4);
	}
	ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN, 
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
	if (ecode != 0) {
		fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
		exit(4);
	}
	printf("L'adresse d'ecoute est: %s\n", serverAddr);
	printf("Le port d'ecoute est: %s\n", serverPort);
	// Definition de la taille du tampon contenant les demandes de connexion
	ecode = listen(descSockRDV, LISTENLEN);
	if (ecode == -1) {
		perror("Erreur initialisation buffer d'écoute");
		exit(5);
	}

	len = sizeof(struct sockaddr_storage);
	// Attente connexion du client
	// Lorsque demande de connexion, creation d'une socket de communication avec le client
	descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
	if (descSockCOM == -1){
		perror("Erreur accept\n");
		exit(6);
	}
	// Echange de données avec le client connecté
	strcpy(buffer, "BLABLABLA\n");

	write(descSockCOM, buffer, strlen(buffer));
	//Fermeture de la connexion
	close(descSockCOM);
	close(descSockRDV);
}



