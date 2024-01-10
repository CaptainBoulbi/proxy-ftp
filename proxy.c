#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define PORTFTP "21"

void format_userid(char *buffer, char **login, int *loginlen, char **serveur){
  int cursor = 0, serveurlen = 0;
  for (; buffer[cursor] != ' ' && cursor<MAXBUFFERLEN; cursor++);
  cursor++;
  *login = &buffer[cursor];
  for (; buffer[cursor] != '@' && cursor<MAXBUFFERLEN; cursor++){
    (*loginlen)++;
  }
  buffer[cursor] = '\0';
  cursor++;
  *serveur = &buffer[cursor];
  for (; buffer[cursor] != '\n' && cursor<MAXBUFFERLEN; cursor++){
    serveurlen++;
  }
  buffer[cursor-1] = '\0';
}

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

  // Initialisation de la socket de RDV IPv4/TCP
  descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
  if (descSockRDV == -1) {
    perror("Erreur création socket RDV\n");
    exit(2);
  }
  // Publication de la socket au niveau du système
  // Assignation d'une adresse IP et un numéro de port
  // Mise à zéro de hints
  memset(&hints, 0, sizeof(hints));
  // Initialisation de hints
  hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par 
  // la fonction getaddrinfo

  // Récupération des informations du serveur
  ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
  if (ecode) {
    fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
    exit(1);
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
  printf("[INFO] L'adresse d'ecoute est: %s\n", serverAddr);
  printf("[INFO] Le port d'ecoute est: %s\n", serverPort);

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

  /*****
     * Testez de mettre 220 devant BLABLABLA ...
     *
     * **/
  buffer[MAXBUFFERLEN-1] = '\0';

  strcpy(buffer, "220: Identification: login@serveur (ex: anonymous@ftp.fau.de)\n");
  write(descSockCOM, buffer, strlen(buffer));
  printf("[CLIENT WRITE] '%s'\n", buffer);;

  //memset(buffer, 0, MAXBUFFERLEN);
  read(descSockCOM, buffer, MAXBUFFERLEN-1);
  printf("[CLIENT READ] '%s'\n", buffer);

  char *loginat = NULL, *serveurat = NULL;
  int loginlen = 0;
  //int serveurlen = 0;

  format_userid(buffer, &loginat, &loginlen, &serveurat);

  char *userlogin = malloc(loginlen + sizeof("USER ") + sizeof("\r\n"));
  strcpy(userlogin, buffer);
  userlogin[loginlen+5] = '\r';
  userlogin[loginlen+6] = '\n';
  printf("[INFO] userlogin: '%s'\n", userlogin);
  printf("[INFO] userlogin len: '%ld'\n", strlen(userlogin));

  printf("[INFO] login:   '%s'\n", loginat);
  printf("[INFO] serveur: '%s'\n", serveurat);

  printf("[INFO] connection au serveur\n");
  int serveurSock = 0;
  connect2Server(serveurat, PORTFTP, &serveurSock);

  {
    int bufferLen = read(serveurSock, buffer, MAXBUFFERLEN-1);
    buffer[bufferLen] = '\0';
    printf("[SERVER READ] '%s'\n", buffer);
    printf("[INFO] connection accepter\n");
  }

  printf("[INFO] identification login\n");
  write(serveurSock, userlogin, strlen(userlogin)+1);
  printf("[SERVER WRITE] '%s'\n", userlogin);;

  {
    int bufferLen = read(serveurSock, buffer, MAXBUFFERLEN-1);
    printf("[SERVER READ] '%s'\n", buffer);

    write(descSockCOM, buffer, bufferLen);
    printf("[CLIENT WRITE] '%s'\n", buffer);
  }

  {
    printf("[INFO] getting password from client\n");

    int bufferLen = read(descSockCOM, buffer, MAXBUFFERLEN-1);
    buffer[bufferLen] = '\0';
    printf("[INFO] buffer len = %d.\n", bufferLen);
    printf("[CLIENT READ] '%s'\n", buffer);;

    // write(serveurSock, buffer, bufferLen);
    // printf("[SERVER WRITE] '%s'\n", buffer);;
  }

  strcpy(buffer, "PASS hamood\r\n");
  printf("[INFO] password sent: '%s'.\n", buffer);
  write(serveurSock, buffer, strlen(buffer));

  buffer[read(serveurSock, buffer, MAXBUFFERLEN-1)] = '\0';
  printf("[SERVER READ] '%s'\n", buffer);;

#if 0
  strcpy(buffer, "PASV");
  write(serveurSock, buffer, sizeof("PASV"));
  printf("[SERVER WRITE] '%s'\n", buffer);;

  read(serveurSock, buffer, MAXBUFFERLEN-1);
  printf("[SERVER READ] '%s'\n", buffer);;
#endif 

  //Fermeture de la connexion
  close(serveurSock);
  close(descSockCOM);
  close(descSockRDV);

  printf("[INFO] end of proxy.\n");
  return 0;
}
