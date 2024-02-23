#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "addrinfo.h"


#define MAX_BUFFER_SIZE 1024

struct sockaddr_in server_addr, client_addr;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int sockfd; 
int serverPort;
char clientMachineName[MAX_BUFFER_SIZE];
int clientPort;



void *listener_thread(void *arg) {
    char buffer[MAX_BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    while (1) {
        ssize_t received_bytes;
        received_bytes = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &addr_len);
        buffer[received_bytes] = '\0';

        pthread_mutex_lock(&mutex);
        printf("Received message from sender: %s\n", buffer);
        pthread_mutex_unlock(&mutex);
    }

    pthread_exit(NULL);
}

void *sender_thread(void *arg) {
    const char *base_message = "Hello from the sender!";
    int messages_sent = 1;

    
    
    while (1) {

        // Calculate the length needed for the new message
        int new_message_length = snprintf(NULL, 0, "%d %s", messages_sent, base_message);
        
        // Allocate memory for the new message
        char *new_message = malloc(new_message_length + 1); // +1 for the null terminator
        
        // Construct the new message
        snprintf(new_message, new_message_length + 1, "%d %s", messages_sent, base_message);
        pthread_mutex_lock(&mutex);

        sendto(sockfd, new_message, strlen(new_message), 0, (const struct sockaddr*)&server_addr, sizeof(server_addr));
        pthread_mutex_unlock(&mutex);

        sleep(1); // Just for demonstration purposes, you might want to remove this in a real application
        messages_sent += 1;
        free(new_message);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_port> <client_machine_name> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Convert command-line arguments to integers
    serverPort = atoi(argv[1]);
    strncpy(clientMachineName, argv[2], sizeof(clientMachineName));
    strcat(clientMachineName, ".csil.sfu.ca");
    clientPort = atoi(argv[3]);

    //printf("clientMachine full domain: %s\n", clientMachineName);
    char *clientIPAddress = getIPAddress(clientMachineName);
    printf("the retrieved clientIPAddress for %s: %s\n", clientMachineName, clientIPAddress);

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(serverPort);

    // Binding socket
    if (bind(sockfd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    pthread_t listener, sender;

        // Create threads and pass necessary data
    if (pthread_create(&listener, NULL, listener_thread, NULL) != 0) {
        perror("pthread_create for listener failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&sender, NULL, sender_thread, NULL) != 0) {
        perror("pthread_create for sender failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Threads finish ever?\n");
    // Wait for threads to finish (never reached in this example)
    pthread_join(listener, NULL);
    pthread_join(sender, NULL);

    printf("Threads here?\n");

    // Close the socket (never reached in this example)
    close(sockfd);

    return 0;
}
