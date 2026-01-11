#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>  //close()
#include<arpa/inet.h>   //sockaddr_in , inet_ntoa()
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>

#define PORT 8080
int desc;

void fermeture(void* sig){
	printf("\n[Serveur] fermeture détectée...\n");
	shutdown(desc,SHUT_RDWR);
	close(desc);
	exit(EXIT_SUCCESS);
}

int main(){
	int ndesc;
    struct sockaddr_in adresse, client_adresse;
    int option = 1;
    int tailleAdresse = sizeof(client_adresse);
    char recepteur[1024] = {0};

    //Creation de la socket
    desc = socket(AF_INET, SOCK_STREAM, 0);
	if(desc == 0){
        perror("socket échoué");
        exit(EXIT_FAILURE);
    }

    //Option pour réutiliser l'adresse port
    if(setsockopt(desc,SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option))){
        perror("setsockopt échoué");
        exit(EXIT_FAILURE);
   	}

	//Signal de fermeture en cas de Ctrl+C
	signal(desc,(void*)fermeture);

    //Configuration de l'adresse du serveur
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;    //ecoute sur toutes les interfaces
    adresse.sin_port = htons(PORT);

    //Attachement de la socket à l'adresse port
    if(bind(desc, (struct sockaddr *)&adresse, sizeof(adresse)) < 0){
        perror("Erreur bind");
       	exit(EXIT_FAILURE);
    }

    //Definition d'une file d'attente
    if(listen(desc, 1) < 0){
        perror("Erreur listen");
       	exit(EXIT_FAILURE);
    }

   	while(1){
      	printf("[SERVEUR] en écoute sur le port %d...\n",PORT);

      	//Approbation de la connection du client
      	ndesc = accept(desc, (struct sockaddr *)&client_adresse, (socklen_t*)&tailleAdresse);
      	if(ndesc < 0){
        	perror("accept échoué");
        	exit(EXIT_FAILURE);
      	}

      	printf("Connection acceptée dépuis %s: au port %d\n",inet_ntoa(client_adresse.sin_addr),ntohs(client_adresse.sin_port));

      	//Reception et réenvoi de messages du client
      	do{
        	int tailleLue = recv(ndesc, recepteur, sizeof(recepteur), 0);
        	if(tailleLue>0){
         		recepteur[tailleLue] = '\0';
          		if(strcmp(recepteur,"/quit") == 0){
	          		send(ndesc, "Bye",3,0);
	          		break;
	        	}
          		send(ndesc, recepteur, strlen(recepteur), 0);
          		//printf("[SERVEUR] Réception et Réenvoie immédiat du message client effectué!\n");
        	}else if(tailleLue == 0){
	        	printf("[SERVEUR] Client %s:%d déconnecté.\n",inet_ntoa(client_adresse.sin_addr), ntohs(client_adresse.sin_port));
	        	break;
        	}else{
	        	perror("recv échoué");
	        	exit(EXIT_FAILURE);
        	}
      	}while(strcmp(recepteur,"/quit") != 0);

      	//Fermeture du socket client
      	close(ndesc);
    }
	return 0;
}
