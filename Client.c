#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>	//close()
#include<arpa/inet.h>  //sockaddr_in, pton()
#include<signal.h>

int descripteur;	//descripteur est rendu global pour la fermeture en cas de Ctrl+C

void fermeture(void* sig){
	printf("\n[Client] fermeture détectée...\n");
	shutdown(descripteur,SHUT_RDWR);
	close(descripteur);
	exit(EXIT_SUCCESS);
}

int main(int argc,char* argv[]){
	int valeurLue;
	struct sockaddr_in client_adresse;
	char recepteur[1024] = {0};
	char message[1024];

	//Verification du nombre d'argument
	if(argc != 3){
		fprintf(stderr,"Usage: %s <port> <adresse_ip_du_serveur>\n",argv[0]);
		exit(EXIT_FAILURE);
	}

	//Création de la socket
	descripteur = socket(AF_INET,SOCK_STREAM,0);
	if(descripteur < 0){
		perror("socket échoué");
		exit(EXIT_FAILURE);
	}

	//Gestion des signaux (Crtl+C)
	signal(SIGINT,(void*)fermeture);

	//Configuration en fonction du port du serveur
	int PORT = atoi(argv[1]);
	client_adresse.sin_family = AF_INET;
	client_adresse.sin_port = htons(PORT);

	//Verification de la validité de l'adresse du port
	if(inet_pton(AF_INET, argv[2], &client_adresse.sin_addr) <= 0){
		perror("Adresse invalide / non supportée");
		exit(EXIT_FAILURE);
	}

	//Demande de connexion au serveur
	if(connect(descripteur, (struct sockaddr *)&client_adresse, sizeof(client_adresse)) < 0){
		perror("connect échouée");
		exit(EXIT_FAILURE);
	}

	//printf("Connexion réussit au serveur %s : au PORT: %d\n",argv[2],PORT);

	do{
		printf("'/quit' => deconnexion au [Serveur]\n");
		printf(">>>Message:");
		fgets(message,sizeof(message),stdin);
		message[strcspn(message,"\n")] = '\0';		//Enlever le retour à la ligne

		//Envoi du message au serveur
		if(send(descripteur, message, strlen(message), 0)<0){
			perror("send échoué");
			exit(EXIT_FAILURE);
		}

		//Retour du message envoyé
		valeurLue = recv(descripteur, recepteur, sizeof(recepteur)-1, 0);
		if(valeurLue > 0){
			recepteur[valeurLue] = '\0';
			printf("[Serveur] ::: %s\n",recepteur);
		}else if(valeurLue == 0){
			printf("Serveur déconnecté...\n");
			break;
		}else{
			perror("recv échoué");
			exit(EXIT_FAILURE);
		}
		printf("\n\n");
	}while(strcmp(message, "/quit") != 0 && valeurLue > 0);

	//Fermeture
	shutdown(descripteur, SHUT_RDWR);
	close(descripteur);
	return 0;
}
