#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>       // close(), fork()
#include<arpa/inet.h>    // sockaddr_in, inet_ntoa()
#include<sys/socket.h>
#include<netinet/in.h>
#include<signal.h>
#include<sys/wait.h>

#define PORT 8080
#define MAX_CLIENTS 20

int nb_client = 0; // compteur global

void signal_fin_enfant(void* sig) {
    int status;
    pid_t pid;
    // Récolte des processus enfants terminés
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        nb_client--; // décrémente quand un client se déconnecte
        //printf("[SERVEUR] Un client s'est déconnecté.\n");
    }
}

int desc;

void fermeture(void* sig){
	printf("\n[Serveur] fermeture détectée...\n");
	shutdown(desc,SHUT_RDWR);
	close(desc);
	exit(EXIT_SUCCESS);
}

void partie_fils(int client_socket, struct sockaddr_in client_addr) {
    char recepteur[1024];
    int tailleLue;

    printf("[SERVEUR] Client connecté depuis %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    while(1) {
        tailleLue = recv(client_socket, recepteur, sizeof(recepteur) - 1, 0);
        if (tailleLue > 0) {
            recepteur[tailleLue] = '\0';
            if (strcmp(recepteur, "/quit") == 0) {
                char *msg = "You will be terminated\n";
                send(client_socket, msg, strlen(msg), 0);
                break;
            }
            send(client_socket, recepteur, strlen(recepteur), 0);
            printf("[SERVEUR] Reception et Réenvoie immédiat du message du client %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        } else if (tailleLue == 0) {
            printf("[SERVEUR] Client %s:%d déconnecté.\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            break;
        } else {
            perror("recv échoué");
            break;
        }
    }

    close(client_socket);
    exit(EXIT_SUCCESS); // fin du processus enfant
}

int main() {
    int ndesc;
    struct sockaddr_in adresse, client_adresse;
    int option = 1;
    socklen_t tailleAdresse = sizeof(client_adresse);

    // Gestion des signaux pour récupérer les enfants
    signal(SIGCHLD, (void*)signal_fin_enfant);

    // Création socket
    desc = socket(AF_INET, SOCK_STREAM, 0);
    if (desc == -1) {
        perror("socket échoué");
        exit(EXIT_FAILURE);
    }

    // Réutilisation du port
    if (setsockopt(desc, SOL_SOCKET, SO_REUSEADDR,
                   &option, sizeof(option)) < 0) {
        perror("setsockopt échoué");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT,(void*)fermeture);

    // Configuration adresse serveur
    adresse.sin_family = AF_INET;
    adresse.sin_addr.s_addr = INADDR_ANY;
    adresse.sin_port = htons(PORT);

    // Bind
    if (bind(desc, (struct sockaddr *)&adresse, sizeof(adresse)) < 0) {
        perror("bind échoué");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(desc, MAX_CLIENTS) < 0) {
        perror("listen échoué");
        exit(EXIT_FAILURE);
    }

    printf("[SERVEUR] En écoute sur le port %d...\n", PORT);

    while (1) {
        ndesc = accept(desc, (struct sockaddr *)&client_adresse, &tailleAdresse);
        if (ndesc < 0) {
            perror("accept échoué");
            continue;
        }

        if (nb_client >= MAX_CLIENTS) {
            char *msg = "Server cannot accept incoming connections anymore. Try again later.\n";
            send(ndesc, msg, strlen(msg), 0);
            close(ndesc);
            printf("[SERVEUR] Connexion refusée (limite atteinte).\n");
            continue;
        }

        nb_client++; // incrémentation quand un client est accepté
        printf("[SERVEUR] nb_client = %d\n", nb_client);

        pid_t pid = fork();
        if (pid == 0) {
            // Enfant
            close(desc);
            partie_fils(ndesc, client_adresse);
        } else if (pid > 0) {
            // Parent
            close(ndesc);
        } else {
            perror("fork échoué");
            close(ndesc);
        }
    }

    close(desc);
    return 0;
}
