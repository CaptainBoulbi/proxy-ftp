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
#define MAXBUFFERLEN 2048           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define PORTFTP "21"

// macro qui permet d'afficher des log plus lisible tout en etant plus facile a écrire qu'un simple printf
#define LOG(...) printf("[INFO]          "__VA_ARGS__)

// socket du serveur et du client
int serveurSock = 0;
int clientSock = 0;

// lis ce qu'a envoyer le serveur et le stock dans buffer
int server_read(char *buffer){
  int ecode = 0;

  // rempli le buffer de 0, pour supprimer d'ancien requete inutile
  memset(buffer, 0, MAXBUFFERLEN);
  // lit dans le socket
  ecode = read(serveurSock, buffer, MAXBUFFERLEN-1);
  // affiche log dans la console
  printf("[SERVER READ]   size = %d - '%.*s'\n", ecode, ecode-2, buffer);

  return ecode;
}

// lis ce qu'a envoyer le client et le stock dans buffer
int client_read(char *buffer){
  int ecode = 0;

  // rempli le buffer de 0, pour supprimer d'ancien requete inutile
  memset(buffer, 0, MAXBUFFERLEN);
  // lit dans le socket
  ecode = read(clientSock, buffer, MAXBUFFERLEN-1);
  // affiche log dans la console
  printf("[CLIENT READ]   size = %d - '%.*s'\n", ecode, ecode-2, buffer);

  return ecode;
}

// evoie la requete stocker dans buffer de la taille len au serveur
int server_write(char *buffer, int len){
  // envoie la requete en l'ecrivant dans le socket
  len = write(serveurSock, buffer, len);
  // affiche log dans la console
  printf("[SERVER WRITE]  size = %d - '%.*s'\n", len, len-2, buffer);
  return len;
}

int client_write(char *buffer, int len){
  // envoie la requete en l'ecrivant dans le socket
  len = write(clientSock, buffer, len);
  // affiche log dans la console
  printf("[CLIENT WRITE]  size = %d - '%.*s'\n", len, len-2, buffer);
  return len;
}

// verifie s'il errcode est un code d'erreur et log le msg avant de quitter le programe
void check_err(int errcode, const char *msg){
  // en general il y'a une erreur si le code de retour et <= 0
  // si le cas d'erreur ne tombe pas dans ce cas, utiliser une autre maniere
  if (errcode <= 0) {
    // force l'affiche des log [INFO] avant d'ecrire les log d'erreur
    // permet de synchroniser stdout et stderr
    fflush(stdout);
    fprintf(stderr, "[ERROR]         %s.\n", msg);
    exit(42);
  }
}

void format_userid(char *buffer,char **userlogin, char **login, int *loginlen, char **serveur, int *serveurlen){
  int cursor = 0;
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
    (*serveurlen)++;
  }
  buffer[cursor-1] = '\0';

  *userlogin = malloc(*loginlen + sizeof("USER ") + sizeof("\r\n"));
  strcpy(*userlogin, buffer);
  (*userlogin)[*loginlen+5] = '\r';
  (*userlogin)[*loginlen+6] = '\n';

  *serveur = malloc(*serveurlen);
  strcpy(*serveur, buffer + *loginlen + sizeof("USER "));
}

int main(){
  int ecode;                       // Code retour des fonctions
  char serverAddr[MAXHOSTLEN];     // Adresse du serveur
  char serverPort[MAXPORTLEN];     // Port du server
  int descSockRDV;                 // Descripteur de socket de rendez-vous
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
  LOG("L'adresse d'ecoute est: %s\n", serverAddr);
  LOG("Le port d'ecoute est: %s\n", serverPort);

  // Definition de la taille du tampon contenant les demandes de connexion
  ecode = listen(descSockRDV, LISTENLEN);
  if (ecode == -1) {
    perror("Erreur initialisation buffer d'écoute");
    exit(5);
  }

  len = sizeof(struct sockaddr_storage);
  // Attente connexion du client
  // Lorsque demande de connexion, creation d'une socket de communication avec le client
  clientSock = accept(descSockRDV, (struct sockaddr *) &from, &len);
  if (clientSock == -1){
    perror("Erreur accept\n");
    exit(6);
  }
  // Echange de données avec le client connecté

  /*****
     * Testez de mettre 220 devant BLABLABLA ...
     *
     * **/
  buffer[MAXBUFFERLEN-1] = '\0';

  LOG("connexion et identification.\n");
  strcpy(buffer, "220: Identification: login@serveur (ex: anonymous@ftp.fau.de || etu@localhost)\n");
  ecode = client_write(buffer, strlen(buffer));
  check_err(ecode, "a l'envoie de la demande de l'identification au client");

  ecode = client_read(buffer);
  check_err(ecode, "a la lecture du login@serveur du client");

  char *userlogin = NULL, *loginat, *serveurat = NULL;
  int loginlen = 0, serveurlen = 0;
  format_userid(buffer, &userlogin, &loginat, &loginlen, &serveurat, &serveurlen);

  LOG("login    : %d bytes - '%s'\n", loginlen, loginat);
  LOG("serveur  : %d bytes - '%s'\n", serveurlen, serveurat);
  LOG("user cmd : %d bytes - '%s'\n", (int)(loginlen + sizeof("USER ")), userlogin);

  LOG("full serveur :\n");
  for (int i=0; i<serveurlen; i++){
    printf("%d - ", serveurat[i]);
  }
  printf("\n");

  ecode = connect2Server(serveurat, PORTFTP, &serveurSock);
  check_err(ecode, "a la connexion du serveur");
  LOG("connexion au serveur ftp réussie.\n");

  ecode = server_read(buffer);
  check_err(ecode, "a la reponse du serveur apres la connexion");

  ecode = server_write(userlogin, strlen(userlogin));
  check_err(ecode, "a la l'envoi du USER au serveur");
  free(userlogin);

  ecode = server_read(buffer);
  check_err(ecode, "a la reponse du serveur sur le USER du client");

  ecode = client_write(buffer, ecode);
  check_err(ecode, "a l'envoie de la reponse du serveur pour le USER au client");

  LOG("mot de passe.\n");
  {
    ecode = client_read(buffer);
    check_err(ecode, "a la lecture du mdp du client");

    ecode = server_write(buffer, ecode);
    check_err(ecode, "a l'envoie du mdp du client vers le serveur");

    ecode = server_read(buffer);
    check_err(ecode, "a la lecture de la reponse du serveur pour le mdp du client");

    ecode = client_write(buffer, ecode);
    check_err(ecode, "a l'envoie de la reponse du serveur pour le mdp au client");
  }

  LOG("la boucle a bouclé.\n");
  ecode = client_read(buffer);
  ecode = server_write(buffer, ecode);
  LOG("la boucle est bouclé.\n");

#if 0
  LOG("PASV.\n");
  {
    strcpy(buffer, "PASV\r\n");
    ecode = server_write(buffer, strlen(buffer));
    check_err(ecode, "a l'envoie de la cmd PASV au serveur");
    buffer[ecode-2] = '\0';

    ecode = server_read(buffer);
    check_err(ecode, "a la reponse du serveur pour la cmd PASV");

    char *port = buffer + 39;
    *(port + 4) = '\0';

    LOG("serveurat = '%s' - port = '%s' - serveurSock = '%d'.\n", serveurat, port, serveurSock);
    close(serveurSock);
    ecode = connect2Server(serveurat, port, &serveurSock);
    LOG("serveurat = '%s' - port = '%s' - serveurSock = '%d'.\n", serveurat, port, serveurSock);
    check_err(ecode, "a la connexion du serveur en mode passive");
    LOG("connexion au serveur ftp en mode passive réussie.\n");
  }
#endif

  LOG("entrez dans la boucle\n");

  // processus serveur --> client
  switch (fork()) {
    case -1: // erreur
      check_err(-1, "erreur a la création de processus.\n");
      break;
    case 0:  // enfant
      while (1){
        ecode = server_read(buffer);
        check_err(ecode, "server read.\n");

        ecode = client_write(buffer, ecode);
        check_err(ecode, "server read.\n");
      }
      break;
  }

  // processus client --> serveur
  switch (fork()) {
    case -1: // erreur
      check_err(-1, "erreur a la création de processus.\n");
      break;
    case 0:  // enfant
      while (1){
        ecode = client_read(buffer);
        check_err(ecode, "client read.\n");

        ecode = server_write(buffer, ecode);
        check_err(ecode, "server write.\n");
      }
      break;
  }

  LOG("sortie de la boucle.\n");

  //Fermeture de la connexion
  close(serveurSock);
  close(clientSock);
  close(descSockRDV);
  // libere la memoire utiliser
  free(serveurat);

  LOG("proxy close.\n");
  return 0;
}
