#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "./simpleSocketAPI.h"

#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define PORTFTP "21"

void format_userid(char *buffer,char **userlogin, char **login, int *loginlen, char **serveur){
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

  *userlogin = malloc(*loginlen + sizeof("USER ") + sizeof("\r\n"));
  strcpy(*userlogin, buffer);
  (*userlogin)[*loginlen+5] = '\r';
  (*userlogin)[*loginlen+6] = '\n';
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
  ecode = write(descSockCOM, buffer, strlen(buffer));
  if (ecode < 0) {
    perror("[ERROR] a l'identification du client.\n");
    exit(69);
  }
  printf("[CLIENT WRITE] '%s'\n", buffer);;

  //memset(buffer, 0, MAXBUFFERLEN);
  ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
  if (ecode < 0) {
    perror("[ERROR] a la  lecture de l'identification du client.\n");
    exit(69);
  }
  printf("[CLIENT READ] '%s'\n", buffer);

  char *userlogin = NULL, *loginat = NULL, *serveurat = NULL;
  int loginlen = 0;
  //int serveurlen = 0;

  format_userid(buffer, &userlogin, &loginat, &loginlen, &serveurat);

  printf("[INFO] userlogin: '%s'\n", userlogin);
  printf("[INFO] login:   '%s'\n", loginat);
  printf("[INFO] serveur: '%s'\n", serveurat);

  printf("[INFO] connection au serveur\n");
  int serveurSock = 0;
  ecode = connect2Server(serveurat, PORTFTP, &serveurSock);
  if (ecode < 0) {
    perror("[ERROR] a la connexion du serveur.\n");
    exit(69);
  }

  memset(buffer, 0, MAXBUFFERLEN);
  ecode = read(serveurSock, buffer, MAXBUFFERLEN-1);
  if (ecode < 0) {
    perror("[ERROR] a la reponse du serveur.\n");
    exit(69);
  }
  buffer[ecode] = '\0';
  printf("[SERVER READ] '%s'\n", buffer);
  printf("[INFO] connection accepter\n");

  printf("[INFO] identification login\n");
  write(serveurSock, userlogin, strlen(userlogin)+1);
  printf("[SERVER WRITE] '%s'\n", userlogin);;

  printf("[INFO] entrez dans la boucle\n");
  printf("[INFO] EAGAIN = %d - EWOULDBLOCK = %d\n", EAGAIN, EWOULDBLOCK);
  while (1) {
    int ecode = 0;
    do {
      memset(buffer, 0, MAXBUFFERLEN);
      ecode = recv(serveurSock, buffer, MAXBUFFERLEN-1, MSG_DONTWAIT);
      printf("[SERVER READ] size = %d, '%s'\n", ecode, buffer);
      //if (ecode <= 0) break;
      //printf("[INFO] at ecode-2 : '%c' - %d.\n", buffer[ecode-2], buffer[ecode-2]);
      printf("[INFO] errno = %d\n", errno);
      //if (errno == EAGAIN || errno == EWOULDBLOCK) continue;

      ecode = write(descSockCOM, buffer, ecode);
      printf("[CLIENT WRITE] size = %d, '%s'\n", ecode, buffer);
      //if (ecode <= 0) break;
      printf("[INFO] ecode = %d, errno = %d\n", ecode, errno);
    } while(ecode >= MAXBUFFERLEN || errno == EAGAIN || errno == EWOULDBLOCK);
    if (ecode <= 0) break;

    do {
      memset(buffer, 0, MAXBUFFERLEN);
      ecode = read(descSockCOM, buffer, MAXBUFFERLEN-1);
      printf("[CLIENT READ] size = %d, '%s'\n", ecode, buffer);
      //if (ecode <= 0) break;
      //printf("[INFO] at ecode-2 : '%c' - %d.\n", buffer[ecode-2], buffer[ecode-2]);
      printf("[INFO] errno = %d\n", errno);
      //if (errno == EAGAIN || errno == EWOULDBLOCK) continue;

      ecode = write(serveurSock, buffer, ecode);
      printf("[SERVER WRITE] size = %d, '%s'\n", ecode, buffer);
      //if (ecode <= 0) break;
      printf("[INFO] ecode = %d, errno = %d\n", ecode, errno);
    } while(ecode >= MAXBUFFERLEN || errno == EAGAIN || errno == EWOULDBLOCK);
    if (ecode <= 0) break;
  }

  printf("[INFO] main loop broke.\n");

  while (1){
    memset(buffer, 0, MAXBUFFERLEN);
    ecode = read(serveurSock, buffer, MAXBUFFERLEN-1);
    printf("[SERVER READ] size = %d, '%s'\n", ecode, buffer);
    if (ecode <= 0) break;
    //printf("[INFO] at ecode-2 : '%c' - %d.\n", buffer[ecode-2], buffer[ecode-2]);

    ecode = write(descSockCOM, buffer, ecode);
    printf("[CLIENT WRITE] size = %d, '%s'\n", ecode, buffer);
    if (ecode <= 0) break;
  }

  //Fermeture de la connexion
  close(serveurSock);
  close(descSockCOM);
  close(descSockRDV);

  printf("[INFO] proxy close.\n");
  return 0;
}
