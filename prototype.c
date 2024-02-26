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
pthread_mutex_t mutexoutComingQueue = PTHREAD_MUTEX_INITIALIZER;

bool terminateThreads;

int sockfd;
int serverPort;
char clientMachineName[MAX_BUFFER_SIZE];
int clientPort;

List *inComingQueue;
List *outGoingQueue;

void *keyboard_input_thread(void *arg)
{
    while (!terminateThreads)
    {
        const char *specialChars = "!";
        char *userInput = getUserInput();
        if (strcmp(userInput, specialChars) == 0)
        {
            printf("Detected !, terminating communication\n");
            terminateThreads = true;
            return NULL;
        }
        // printf("Input Acquired %s\n", userInput);
        List_prepend(outGoingQueue, userInput);
    }

    //printf("Terminating Keyboard_input_thread\n");
}

void *screen_printer_thread(void *arg)
{
    while (!terminateThreads)
    {
        if (List_count(inComingQueue) != 0)
        {
            char *message = List_trim(inComingQueue);
            printf("Client Message: %s\n", message);
            free(message);
        }
    }
}

void *listener_thread(void *arg)
{
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in recInfoAddr;

    socklen_t addr_len = sizeof(recInfoAddr);

    while (!terminateThreads)
    {
        ssize_t received_bytes;

        // pthread_mutex_lock(&mutex);
        received_bytes = recvfrom(sockfd, (char *)buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&recInfoAddr, &addr_len);

        // pthread_mutex_unlock(&mutex);

        int terminateIDX = (received_bytes < MAX_BUFFER_SIZE) ? received_bytes : MAX_BUFFER_SIZE - 1;
        buffer[terminateIDX] = '\0';
        printf("Received message from sender (%s:%d): %s\n",
               inet_ntoa(recInfoAddr.sin_addr), ntohs(recInfoAddr.sin_port), buffer);

        char* receievedMSG = (char *)malloc(strlen(buffer) + 1);

        if (receievedMSG== NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        // Copy the message into the allocated memory
        strcpy(receievedMSG, buffer);

        List_prepend(inComingQueue, receievedMSG);

        // char* new_message = "Boy what the hell boy";
        // sendto(sockfd, new_message, strlen(new_message), 0, (const struct sockaddr*)&server_addr, sizeof(server_addr));
        // sleep(1);
    }
    return NULL;
    //pthread_exit(NULL);
}

void *sender_thread(void *arg)
{
    //const char *base_message = "Hello from the Kev!";
    //int messages_sent = 1;
    char* newestInput;
    //sleep(2);

    while (!terminateThreads)
    {
        if (List_count(outGoingQueue) != 0)
        {
            printf("Message has new data to be sent");
            newestInput = List_trim(outGoingQueue);
            
            sendto(sockfd, newestInput, strlen(newestInput), 0, (const struct sockaddr *)&client_addr, sizeof(client_addr));

            free(newestInput);
        }
        //random testing
        // else
        // {
        //     // Calculate the length needed for the new message
        //     int new_message_length = snprintf(NULL, 0, "%d %s", messages_sent, base_message);

        //     // Allocate memory for the new message
        //     char *new_message = malloc(new_message_length + 1); // +1 for the null terminator

        //     // Construct the new message
        //     snprintf(new_message, new_message_length + 1, "%d %s", messages_sent, base_message);
        //     pthread_mutex_lock(&mutex);

        //     sendto(sockfd, new_message, strlen(new_message), 0, (const struct sockaddr *)&client_addr, sizeof(client_addr));
        //     pthread_mutex_unlock(&mutex);

        //     sleep(5); // Just for demonstration purposes, you might want to remove this in a real application
        //     messages_sent += 1;
        //     free(new_message);
        // }
    }
    return NULL;
    //pthread_exit(NULL);
}

void setUpSockets(char *argv[])
{
    // Convert command-line arguments to integers
    serverPort = atoi(argv[1]);
    strncpy(clientMachineName, argv[2], sizeof(clientMachineName));
    strcat(clientMachineName, ".csil.sfu.ca");
    clientPort = atoi(argv[3]);

    // printf("clientMachine full domain: %s\n", clientMachineName);
    char *clientIPAddress = getIPAddress(clientMachineName);
    printf("Client IP Address for %s: %s\n", clientMachineName, clientIPAddress);

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
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_port> <client_machine_name> <client_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    terminateThreads = false;

    printCurrentMachineIPAddress();
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

    // char* ret = getUserInput();
    // printf("userinput = %s", ret);
    //  Wait for threads to finish (never reached in this example)

    pthread_join(keyboard, NULL);
    //pthread_join(listener, NULL);
    pthread_join(sender, NULL);
    pthread_join(screen_printer, NULL);

    pthread_cancel(listener);

    printf("Threads Terminated\n");

    // Close the socket at the end
    close(sockfd);

    return 0;
}
