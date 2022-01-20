#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <arpa/inet.h>

struct _client
{
    char ipAddress[40];
    int port;
    char name[40];
} tcpClients[4];
int nbClients;
int fsmServer; // bascule mode quand nb connexion max (finit state machine)
int deck[13]={0,1,2,3,4,5,6,7,8,9,10,11,12};
int tableCartes[4][8];
char *nomcartes[]=
{"Sebastian Moran", "irene Adler", "inspector Lestrade",
  "inspector Gregson", "inspector Baynes", "inspector Bradstreet",
  "inspector Hopkins", "Sherlock Holmes", "John Watson", "Mycroft Holmes",
  "Mrs. Hudson", "Mary Morstan", "James Moriarty"};
int joueurCourant; // (joueurCourant+1)%joueurCourant

char com; int gid; int indice_colonne; // variables pour le scanf du "O"
int indice_ligne;char indice_colonne_char2[255]; // variables pour le scanf du "S"
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void melangerDeck()
{
    int i;
    int index1,index2,tmp;

    for (i=0;i<1000;i++)
    {
            index1=rand()%13;
            index2=rand()%13;

            tmp=deck[index1];
            deck[index1]=deck[index2];
            deck[index2]=tmp;
    }
}

void createTable()
{
	// Le joueur 0 possede les cartes d'indice 0,1,2
	// Le joueur 1 possede les cartes d'indice 3,4,5 
	// Le joueur 2 possede les cartes d'indice 6,7,8 
	// Le joueur 3 possede les cartes d'indice 9,10,11 
	// Le coupable est la carte d'indice 12
	int i,j,c;

	for (i=0;i<4;i++) // initialise table à 0
		for (j=0;j<8;j++)
			tableCartes[i][j]=0;
    // rempli table
	for (i=0;i<4;i++)
	{
		for (j=0;j<3;j++) // pour chaque joueur, on a 3 cartes
		{
			c=deck[i*3+j]; // on parcours le deck mélangés, on prend l'indice 0 1 2 puis 3 4 5
			switch (c)
			{
				case 0: // Sebastian Moran
					tableCartes[i][7]++;
					tableCartes[i][2]++;
					break;
				case 1: // Irene Adler
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					tableCartes[i][5]++;
					break;
				case 2: // Inspector Lestrade
					tableCartes[i][3]++;
					tableCartes[i][6]++;
					tableCartes[i][4]++;
					break;
				case 3: // Inspector Gregson 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					tableCartes[i][4]++;
					break;
				case 4: // Inspector Baynes 
					tableCartes[i][3]++;
					tableCartes[i][1]++;
					break;
				case 5: // Inspector Bradstreet 
					tableCartes[i][3]++;
					tableCartes[i][2]++;
					break;
				case 6: // Inspector Hopkins 
					tableCartes[i][3]++;
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					break;
				case 7: // Sherlock Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][2]++;
					break;
				case 8: // John Watson 
					tableCartes[i][0]++;
					tableCartes[i][6]++;
					tableCartes[i][2]++;
					break;
				case 9: // Mycroft Holmes 
					tableCartes[i][0]++;
					tableCartes[i][1]++;
					tableCartes[i][4]++;
					break;
				case 10: // Mrs. Hudson 
					tableCartes[i][0]++;
					tableCartes[i][5]++;
					break;
				case 11: // Mary Morstan 
					tableCartes[i][4]++;
					tableCartes[i][5]++;
					break;
				case 12: // James Moriarty 
					tableCartes[i][7]++;
					tableCartes[i][1]++;
					break;
			}
		}
	}
} 

void printDeck()
{
    int i,j;

    for (i=0;i<13;i++)
            printf("%d %s\n",deck[i],nomcartes[deck[i]]);

	for (i=0;i<4;i++) // joueur
	{
		for (j=0;j<8;j++) // objet
			printf("%2.2d ",tableCartes[i][j]);
		puts("");
	}
}

void printClients()
{
    int i;

    for (i=0;i<nbClients;i++)
            printf("%d: %s %5.5d %s\n",i,tcpClients[i].ipAddress,
                    tcpClients[i].port,
                    tcpClients[i].name);
}

int findClientByName(char *name) // retourne indice de la structure
{
        int i;

        for (i=0;i<nbClients;i++)
                if (strcmp(tcpClients[i].name,name)==0)
                        return i;
        return -1;
}

void sendMessageToClient(char *clientip,int clientport,char *mess)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server = gethostbyname(clientip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(clientport);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        {
                printf("ERROR connecting\n");
                exit(1);
        }

        sprintf(buffer,"%s\n",mess);
        n = write(sockfd,buffer,strlen(buffer));

    close(sockfd);
}

void broadcastMessage(char *mess)
{
        int i;

        for (i=0;i<nbClients;i++)
                sendMessageToClient(tcpClients[i].ipAddress,
                        tcpClients[i].port,
                        mess);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
	int i;

        char com; // lettre msg client
        char clientIpAddress[256], clientName[256];
        int clientPort;
        int id;
        char reply[256];


     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

	printDeck();
	melangerDeck();
	createTable();
	printDeck();
	joueurCourant=0;

	for (i=0;i<4;i++)
	{
        	strcpy(tcpClients[i].ipAddress,"localhost");
        	tcpClients[i].port=-1;
        	strcpy(tcpClients[i].name,"-");
	}

     while (1)
     {    
     	newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     	if (newsockfd < 0) 
          	error("ERROR on accept");

     	bzero(buffer,256);
     	n = read(newsockfd,buffer,255); //r
     	if (n < 0) 
		error("ERROR reading from socket");

        printf("Received packet from %s:%d\nData: [%s]\n\n",
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buffer);

        if (fsmServer==0) // attend 4 connexions
        {
        	switch (buffer[0])
        	{
                	case 'C':
                        	sscanf(buffer,"%c %s %d %s", &com, clientIpAddress, &clientPort, clientName);
                        	printf("COM=%c ipAddress=%s port=%d name=%s\n",com, clientIpAddress, clientPort, clientName);

                        	// fsmServer==0 alors j'attends les connexions de tous les joueurs
                                strcpy(tcpClients[nbClients].ipAddress,clientIpAddress);
                                tcpClients[nbClients].port=clientPort;
                                strcpy(tcpClients[nbClients].name,clientName);
                                nbClients++;

                                printClients();

				// rechercher l'id du joueur qui vient de se connecter

                                id=findClientByName(clientName);
                                printf("id=%d\n",id);

				// lui envoyer un message personnel pour lui communiquer son id

                                sprintf(reply,"I %d",id);
                                sendMessageToClient(tcpClients[id].ipAddress,
                                       tcpClients[id].port,
                                       reply);

				// Envoyer un message broadcast pour communiquer a tout le monde la liste des joueurs actuellement
				// connectes

                                sprintf(reply,"L %s %s %s %s", tcpClients[0].name, tcpClients[1].name, tcpClients[2].name, tcpClients[3].name);
                                broadcastMessage(reply);

				// Si le nombre de joueurs atteint 4, alors on peut lancer le jeu

                                if (nbClients==4)
				{
					// On envoie ses cartes au joueur 0, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI (OK)
                    sprintf(reply,"D %d %d %d %d %d %d %d %d %d %d %d",deck[0],deck[1],deck[2],tableCartes[0][0],tableCartes[0][1],tableCartes[0][2],tableCartes[0][3],
                        tableCartes[0][4],tableCartes[0][5],tableCartes[0][6],tableCartes[0][7]);
                            sendMessageToClient(tcpClients[0].ipAddress,
                                   tcpClients[0].port,
                                   reply);                 
					// On envoie ses cartes au joueur 1, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI (OK)
                    sprintf(reply,"D %d %d %d %d %d %d %d %d %d %d %d",deck[3],deck[4],deck[5],tableCartes[1][0],tableCartes[1][1],tableCartes[1][2],tableCartes[1][3],
                        tableCartes[1][4],tableCartes[1][5],tableCartes[1][6],tableCartes[1][7]);
                    sendMessageToClient(tcpClients[1].ipAddress,
                           tcpClients[1].port,
                           reply);

					// On envoie ses cartes au joueur 2, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI (OK)
                    sprintf(reply,"D %d %d %d %d %d %d %d %d %d %d %d",deck[6],deck[7],deck[8],tableCartes[2][0],tableCartes[2][1],tableCartes[2][2],tableCartes[2][3],
                        tableCartes[2][4],tableCartes[2][5],tableCartes[2][6],tableCartes[2][7]);
                    sendMessageToClient(tcpClients[2].ipAddress,
                           tcpClients[2].port,
                           reply);

					// On envoie ses cartes au joueur 3, ainsi que la ligne qui lui correspond dans tableCartes
					// RAJOUTER DU CODE ICI (OK)
                    sprintf(reply,"D %d %d %d %d %d %d %d %d %d %d %d",deck[9],deck[10],deck[11],tableCartes[3][0],tableCartes[3][1],tableCartes[3][2],tableCartes[3][3],
                        tableCartes[3][4],tableCartes[3][5],tableCartes[3][6],tableCartes[3][7]);
                    sendMessageToClient(tcpClients[3].ipAddress,
                           tcpClients[3].port,
                           reply);

					// On envoie enfin un message a tout le monde pour definir qui est le joueur courant=0
					// RAJOUTER DU CODE ICI (OK)
                    sprintf(reply,"M %d",joueurCourant);
                    broadcastMessage(reply);
                    fsmServer=1;
				}
				break;
                }
	}
	else if (fsmServer==1)
	{
		switch (buffer[0])
		{
                	case 'G': // coupable
				// RAJOUTER DU CODE ICI
				break;
                	case 'O': // demander à tout le monde : "est ce que vous avez ce symbole"
				        // RAJOUTER DU CODE ICI
                        // sprintf(sendBuffer,"O %d %d",gId, objetSel); // SERT A QUOI gID
                        
                        sscanf(buffer,"%c %d %d",&com,&gid,&indice_colonne);
                        char reply_bis[255]; char indice_colonne_char[255];
                        memset(reply_bis,0,255);memset(indice_colonne_char,0,255);
                        for(int i=0;i<4;i++){
                            if(i != gid && tableCartes[i][indice_colonne] > 0){
                                strcat(reply_bis,tcpClients[i].name);
                                strcat(reply_bis," a bien le symbole ");
                                sprintf(indice_colonne_char,"%d",indice_colonne);
                                strcat(reply_bis,indice_colonne_char);
                            }
                        }
                        broadcastMessage(reply_bis);

                        joueurCourant = (joueurCourant+1)%4;
                        sprintf(reply,"M %d",joueurCourant);
                        broadcastMessage(reply);
				break;
			case 'S': // demander à un joueur combien il en a
				// RAJOUTER DU CODE ICI
                // sprintf(sendBuffer,"S %d %d %d",gId, joueurSel,objetSel);                

                sscanf(buffer,"%c %d %d %d",&com,&gid,&indice_ligne,&indice_colonne);
                memset(reply,0,255);
                strcat(reply,tcpClients[indice_ligne].name);
                strcat(reply," a ");
                sprintf(indice_colonne_char2,"%d",tableCartes[indice_ligne][indice_colonne]);
                strcat(reply,indice_colonne_char2);
                strcat(reply," fois le symbole ");
                sprintf(indice_colonne_char2,"%d",indice_colonne);
                strcat(reply,indice_colonne_char2);

                broadcastMessage(reply);

                joueurCourant = (joueurCourant+1)%4;

                sprintf(reply,"V %d %d %d",indice_ligne,indice_colonne,tableCartes[indice_ligne][indice_colonne]);
                broadcastMessage(reply);

                sprintf(reply,"M %d",joueurCourant);
                broadcastMessage(reply);

				break;
            default:
                break;
		}
        }
     	close(newsockfd);
     }
     close(sockfd);
     return 0; 
}
