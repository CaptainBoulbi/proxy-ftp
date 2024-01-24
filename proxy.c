/******************************************************************************
*  R3.05	                                                              *
*******************************************************************************
*                                                                             *
*  Intitulé : Proxy FTP 	                                              *
*                                                                             *
*******************************************************************************
*                                                                             *
*  NANAHA Hamza		                                                      *
*                                                                             *
*  BOUDIN Raphael                                                             *
*                                                                             *
*******************************************************************************
*                                                                             *
*  Nom du fichier : proxy.c                                                   *
*                                                                             *
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
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
// permet de passer un string et sa taille definit durant la compilation au fonction qui on besoin des 2 informations
#define EXPAND_LIT(string) string, sizeof(string)

// socket du serveur et du client
int serveurSock = 0;
int clientSock  = 0;
int passiveSock = 0;
int portSock    = 0;

int server_read(char *buffer);
int server_write(char *buffer, int len);
int client_read(char *buffer);
int client_write(char *buffer, int len);

void gerer_connexion(char *buffer);
void check_err(int errcode, const char *msg);
int passive_data(char *cursor, char **server, char **port);
void go_passive(char *buffer);
void format_userid(char *buffer,char **userlogin, char **login, int *loginlen, char **serveur, int *serveurlen);

// passe la connexion au serveur en mode passive
void go_passive(char *buffer){
  LOG("passage en mode passive.\n");
  int ecode = 0;

  ecode = server_write(EXPAND_LIT("PASV\r\n"));
  check_err(ecode, "a l'envoie de la cmd PASV au serveur\n");

  ecode = server_read(buffer);
  check_err(ecode, "a la reponse du serveur pour la cmd PASV\n");

  char *serveur = NULL, *port = NULL;
  ecode = passive_data(buffer, &serveur, &port);
  check_err(ecode, "a la recupération des données addr / port du mode passive\n");

  LOG("serveur : '%s'\n", serveur);
  LOG("port    : '%s'\n", port);

  ecode = connect2Server(serveur, port, &passiveSock);
  check_err(ecode+1, "a la connexion au nouveau socket pour le mode passive\n");

  LOG("passive socket : %d\n", passiveSock);

  free(serveur);
  free(port);
  LOG("passage en mode passive réussie\n");
}


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

// evoie la requete stocker dans buffer de la taille len au client
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

// extrait les informations de la cmd USER login@serveur envoie par le client
void format_userid(char *buffer,char **userlogin, char **login, int *loginlen, char **serveur, int *serveurlen){
  int cursor = 0;
  // deplace le curseur jusqu'au 1er espace, séparant "USER " et le reste de la cmd
  for (; buffer[cursor] != ' ' && cursor<MAXBUFFERLEN; cursor++);
  // debut du login (anonymous ou etu ou utilisateur de la machien local) au char juste apres le curseur
  *login = &buffer[++cursor];
  // deplace le curseur jusqu'au @, séparant le login et serveur
  for (; buffer[cursor] != '@' && cursor<MAXBUFFERLEN; cursor++){
    // incremente la variable qui stoque la taille de la chaine de char login 
    (*loginlen)++;
  }
  // modifie le char @ en '\0' pour séparer le login et serveur par le char null
  buffer[cursor++] = '\0';
  // debut du serveur a l'emplacement du curseur
  *serveur = &buffer[cursor];
  // deplace le curseur jusqu'a la fin pour compter la taille de la chaine de char serveur
  for (; buffer[cursor] != '\n' && cursor<MAXBUFFERLEN; cursor++){
    // incremente la taille du serveur
    (*serveurlen)++;
  }
  // change le \r\n de la fin de la cmd recus par \0
  buffer[cursor-1] = '\0';

  // alloue de la memoire suffisante pour la cmd "USER login"
  *userlogin = malloc(*loginlen + sizeof("USER ") + sizeof("\r\n"));
  // copie la cmd du buffer dans la variable userlogin
  strcpy(*userlogin, buffer);
  // ajoute les char \r\n a la fin de la cmd qui sont necessaire pour le protocole ftp
  (*userlogin)[*loginlen+5] = '\r';
  (*userlogin)[*loginlen+6] = '\n';

  // alloue de la memoire pour le nom serveur
  *serveur = malloc(*serveurlen);
  // copie le nom du serveur de son emplacement dans le buffer dans la variable serveur
  strcpy(*serveur, buffer + *loginlen + sizeof("USER "));
}

// extraint les données utiles (ip / port) de la cmd PORT et PASV
int passive_data(char *cursor, char **server, char **port){
  int n[6] = {0};
  // deplace le curseur jusqu'au debut des nombres
  while (*cursor != '(') cursor++;
  cursor++;
  // boucle pour récuperer les 6 nombre pour calculer l'addresse et le port du mode passive
  for (int i=0; i<6; i++){
    // compte le nombre de char du nombre
    int next = 0;
    while (*(cursor + next) != ',' && *(cursor + next) != ')') next++;
    // set le char ',' ou ')' a '\0' pour definir la limite du nombre pour la fonction atoi()
    *(cursor + next) = '\0';
    // recupere le nombre n1 du protocole
    n[i] = atoi(cursor);
    // deplace le cursor au prochain nombre
    cursor = cursor + next + 1;
  }
  // alloue de la memoire pour le serveur et port et copy les bonnes valeur dedans
  *server = malloc(3*4 + 4 + 1);   // taille nombre max (127 : 3) * nombre de nombre (127.0.0.1 : 4) + nombre de point + fin de chaine '\0'
  //memset(*server, 0, 3*4 + 4 + 1); // remplie la memoire de 0
  sprintf(*server, "%d.%d.%d.%d", n[0], n[1], n[2], n[3]);
  *port = malloc(5 + 1);           // taille du port max (65 635) donc 5 char + fin de chaine '\0'
  //memset(*server, 0, 5 + 1);       // remplie la memoire de 0
  sprintf(*port, "%d", n[4]*256 + n[5]);

  return 1;
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
    perror("SERVEUR: getsockname\n");
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
    perror("Erreur initialisation buffer d'écoute\n");
    exit(5);
  }

  len = sizeof(struct sockaddr_storage);

  // boucle principale qui attent une connexion et gere la connexion dans un processus pour pouvoir receptionner la prochaine connexion
  while (1){
    // Attente connexion du client
    // Lorsque demande de connexion, creation d'une socket de communication avec le client
    clientSock = accept(descSockRDV, (struct sockaddr *) &from, &len);
    if (clientSock == -1){
      perror("Erreur accept\n");
      exit(6);
    }

    // crée le processus
    switch (fork()) {
      case -1: // erreur
        check_err(-1, "erreur a la création de processus.\n");
        break;
      case 0:  // enfant
        gerer_connexion(buffer);
        break;
    }
  }

  //Fermeture de la connexion
  close(serveurSock);
  close(clientSock);
  close(descSockRDV);

  LOG("proxy close.\n");
  return 0;
}

// gere la connexion d'un client avec un serveur
void gerer_connexion(char *buffer){
  buffer[MAXBUFFERLEN-1] = '\0';
  int ecode = 0;

  // envoie msg de connexion au client
  LOG("connexion et identification.\n");
  ecode = client_write(EXPAND_LIT("220: Identification: login@serveur (ex: cptbb@localhost || etu@localhost)\n"));
  check_err(ecode, "a l'envoie de la demande de l'identification au client\n");

  // lit la reponse du client pour avoir login@serveur
  ecode = client_read(buffer);
  check_err(ecode, "a la lecture du login@serveur du client\n");

  // recupere les données utiles de la cmd USER login@serveur
  char *userlogin = NULL, *loginat, *serveurat = NULL;
  int loginlen = 0, serveurlen = 0;
  format_userid(buffer, &userlogin, &loginat, &loginlen, &serveurat, &serveurlen);

  // deboguage : affiche les données recupere
  LOG("login    : %d bytes - '%s'\n", loginlen, loginat);
  LOG("serveur  : %d bytes - '%s'\n", serveurlen, serveurat);
  LOG("user cmd : %d bytes - '%s'\n", (int)(loginlen + sizeof("USER ")), userlogin);

  // ce connecte au serveur envoyer par le client
  ecode = connect2Server(serveurat, PORTFTP, &serveurSock);
  check_err(ecode+1, "a la connexion du serveur\n");
  LOG("connexion au serveur ftp réussie.\n");

  // libere la memoire utiliser
  free(serveurat);

  // recupere la reponse du serveur apres la connexion
  ecode = server_read(buffer);
  check_err(ecode, "a la reponse du serveur apres la connexion\n");

  // ce connecte avec l'identifiant fournis par le client au serveur
  ecode = server_write(userlogin, strlen(userlogin));
  check_err(ecode, "a la l'envoi du USER au serveur\n");
  free(userlogin);

  // recupere la reponse du serveur / check si l'identification est bonne
  ecode = server_read(buffer);
  check_err(ecode, "a la reponse du serveur sur le USER du client\n");

  // envois la reponse du serveur pour l'identification au client
  ecode = client_write(buffer, ecode);
  check_err(ecode, "a l'envoie de la reponse du serveur pour le USER au client\n");

  // partie mot de passe
  LOG("mot de passe.\n");
  {
    // recupere le mdp du client
    ecode = client_read(buffer);
    check_err(ecode, "a la lecture du mdp du client\n");

    // envoie le mdp du client au serveur
    ecode = server_write(buffer, ecode);
    check_err(ecode, "a l'envoie du mdp du client vers le serveur\n");

    // recupere la reponse du serveur pour la connexion avec le mdp
    ecode = server_read(buffer);
    check_err(ecode, "a la lecture de la reponse du serveur pour le mdp du client\n");

    // envoie la reponse du serveur a la connexion au mdp au client
    ecode = client_write(buffer, ecode);
    check_err(ecode, "a l'envoie de la reponse du serveur pour le mdp au client\n");
  }

  // passe la connexion proxy --> serveur en mode passive
  go_passive(buffer);

  /*
  * on est partie sur une solution de double processus pour gérer le probleme ou on recois
  * une requete plus grande que MAXBUFFERLEN, chaque processus est responsable de l'envoie de données
  * d'une machine a une autre (server vers client et client vers serveur), comme ça pas de probleme
  * ou le proxy a lu et envoyer la moitier d'une requete du client vers le serveur et attent la reponse
  * du serveur alors qu'il attent la suite de la requete.
  */

  LOG("creation processus\n");

  // processus qui gere les requete serveur --> client
  switch (fork()) {
    case -1: // erreur
      check_err(-1, "erreur a la création de processus.\n");
      break;
    case 0:  // enfant
      // boucle qui renvoie toute les données recus par le serveur au client
      while (1){
        // si ecode < 0 alors le socket a etait fermer par le serveur alors on peut exit le processus (effectuer dans le check_err)
        ecode = server_read(buffer);
        check_err(ecode, "server socket closed.\n");

        ecode = client_write(buffer, ecode);
        check_err(ecode, "client socket closed.\n");
      }
      break;
  }

  // processus qui gere les requete client --> serveur
  switch (fork()) {
    case -1: // erreur
      check_err(-1, "erreur a la création de processus.\n");
      break;
    case 0:  // enfant
      // boucle qui renvoie toute les données recus par le client au serveur
      while (1){
        // si ecode < 0 alors le socket a etait fermer par le client alors on peut exit le processus (effectuer dans le check_err)
        ecode = client_read(buffer);
        check_err(ecode, "client socket closed.\n");

        ecode = server_write(buffer, ecode);
        check_err(ecode, "server socket closed.\n");
      }
      break;
  }

  // attente 1er processus finit
  wait(NULL);
  // attente 2eme processus finit
  wait(NULL);
}
