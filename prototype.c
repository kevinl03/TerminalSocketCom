#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "addrinfo.h"
#include "userinput.h"
#include "list.h"

#define MAX_BUFFER_SIZE 1024

struct sockaddr_in server_addr, client_addr;
pthread_mutex_t mutexSocket = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexInComingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexOutGoingQueue = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condInComingQueue = PTHREAD_COND_INITIALIZER;
pthread_cond_t condOutGoingQueue = PTHREAD_COND_INITIALIZER;


bool terminateThreads; // Ssignal threads to terminate

int sockfd;
int serverPort;
char clientMachineName[MAX_BUFFER_SIZE];
int clientPort;

List *inComingQueue; // Store incoming messages
List *outGoingQueue; // Store outgoing messages



void isTermination(char* input) // Check if the input is a termination signal ("!")
{
    const char *cancellationChar = "!";
    if (strcmp(input, cancellationChar) == 0)
    {
        terminateThreads = true; // Signal threads to terminate
        pthread_cond_broadcast(&condInComingQueue);
        pthread_cond_broadcast(&condOutGoingQueue);
    }
    return;
}

void *keyboard_input_thread(void *arg) // Get user input and add it to the outgoing queue
{
    char* userInput;
    while (!terminateThreads)
    {
        userInput = getUserInput();

        pthread_mutex_lock(&mutexOutGoingQueue);
        {
            List_prepend(outGoingQueue, userInput);
            pthread_cond_signal(&condOutGoingQueue); // Signal that a new message is ready to send
        }
        pthread_mutex_unlock(&mutexOutGoingQueue);
    }
    return NULL;
}

void *screen_printer_thread(void *arg) // Print incoming messages
{
    char* message;
    while (!terminateThreads)
    {
        if (List_count(inComingQueue) != 0) // If there are messages to display
        {
            pthread_mutex_lock(&mutexInComingQueue);
            {
                while (List_count(inComingQueue) == 0) { // Prevents busy wait
                    pthread_cond_wait(&condInComingQueue, &mutexInComingQueue); // Wait for new messages to display
                }
                message = List_trim(inComingQueue); // Get the message from the queue
            }
            pthread_mutex_unlock(&mutexInComingQueue);

            printf("Client Message: %s\n", message); // Format and print the message
            free(message); // Free the memory allocated by malloc

        }
    }
    return NULL;
}

void *listener_thread(void *arg) // Listen for incoming messages
{
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in recInfoAddr;

    socklen_t addr_len = sizeof(recInfoAddr);

    while (!terminateThreads)
    {
        ssize_t received_bytes;
        
        pthread_mutex_lock(&mutexSocket);
        {    
            received_bytes = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&recInfoAddr, &addr_len);
        }
        pthread_mutex_unlock(&mutexSocket);

        int terminateIDX = (received_bytes < MAX_BUFFER_SIZE) ? received_bytes : MAX_BUFFER_SIZE - 1;
        buffer[terminateIDX] = '\0';

        char* receievedMSG = (char *)malloc(strlen(buffer) + 1);

        if (receievedMSG== NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        strcpy(receievedMSG, buffer);  // Copy the message into the allocated memory

        pthread_mutex_lock(&mutexInComingQueue);
        {
            List_prepend(inComingQueue, receievedMSG);
            pthread_cond_signal(&condInComingQueue); // Signal that a new message has arrived
        }
        pthread_mutex_unlock(&mutexInComingQueue);

        isTermination(receievedMSG); // Check if the message is a termination signal

    }
    return NULL;
}

void *sender_thread(void *arg) // Send messages
{
    char* newestInput; // Store the newest message to send

    while (!terminateThreads)
    {
        if (List_count(outGoingQueue) != 0) // If there are messages to send
        {
            pthread_mutex_lock(&mutexOutGoingQueue);
            {
                while (List_count(outGoingQueue) == 0) { // Prevents busy wait
                    pthread_cond_wait(&condOutGoingQueue, &mutexOutGoingQueue); // Wait for new messages to send
                }
                newestInput = List_trim(outGoingQueue);
            }
            pthread_mutex_unlock(&mutexOutGoingQueue);

            sendto(sockfd, newestInput, strlen(newestInput), 0, (const struct sockaddr *)&client_addr, sizeof(client_addr));

            isTermination(newestInput); // Check if the message is a termination signal

            free(newestInput); // Free the memory allocated by getUserInput
        }
    }
    return NULL;
}

void setUpSockets(char *argv[]) // Set up the server and client sockets
{
    // Convert command-line arguments to integers
    serverPort = atoi(argv[1]);
    strncpy(clientMachineName, argv[2], sizeof(clientMachineName));
    strcat(clientMachineName, ".csil.sfu.ca");
    clientPort = atoi(argv[3]);

    char *clientIPAddress = getIPAddress(clientMachineName);
    // printf("Client IP Address for %s: %s\n", clientMachineName, clientIPAddress);

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(serverPort);

    // Set up client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(clientIPAddress);
    client_addr.sin_port = htons(clientPort);

    // Binding socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    free(clientIPAddress);
}

void freeCharPointer(void* pItem) { // Free the memory allocated by malloc
    if (pItem != NULL) {
        free((char*)pItem);
    }
}

int main(int argc, char *argv[])
{
    // if (argc != 4)
    // {
    //     fprintf(stderr, "Usage: %s <server_port> <client_machine_name> <client_port>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    terminateThreads = false; // Initialize the termination signal

    setUpSockets(argv);

    inComingQueue = List_create();
    outGoingQueue = List_create();

    pthread_t listener, sender, keyboard, screen_printer;

    // Create threads and pass necessary data
    if (pthread_create(&listener, NULL, listener_thread, NULL) != 0)
    {
        perror("pthread_create for listener failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&sender, NULL, sender_thread, NULL) != 0)
    {
        perror("pthread_create for sender failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&keyboard, NULL, keyboard_input_thread, NULL) != 0)
    {
        perror("pthread_create for sender failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&screen_printer, NULL, screen_printer_thread, NULL) != 0)
    {
        perror("pthread_create for sender failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Wait for threads to finish    
    pthread_join(sender, NULL);
    pthread_join(screen_printer, NULL);


    //keyboard input is blocking so lets cancel this thread too
    pthread_cancel(keyboard);
    pthread_join(keyboard, NULL);
    
    //rcvfrom function is blocking so we need to cancel the thread
    pthread_cancel(listener);
    pthread_join(listener, NULL);

    // Close the socket at the end
    close(sockfd);

    // Clean up
    pthread_cond_destroy(&condInComingQueue);
    pthread_cond_destroy(&condOutGoingQueue);

    pthread_mutex_destroy(&mutexSocket);
    pthread_mutex_destroy(&mutexInComingQueue);
    pthread_mutex_destroy(&mutexOutGoingQueue);

    List_free(inComingQueue, freeCharPointer);
    List_free(outGoingQueue, freeCharPointer);

    return 0;
}
