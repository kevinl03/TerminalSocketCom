#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAX_BUFFER_SIZE 256

char* getIPAddress(const char *input) {
    char hostname[MAX_BUFFER_SIZE];
    snprintf(hostname, sizeof(hostname), "%s%s", input, ".csil.sfu.ca");

    struct addrinfo hints, *result, *rp;

    // Set up hints structure
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Stream socket

    // Perform DNS lookup for the modified hostname
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Iterate through the list of addresses and return the first IP address
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        void *addr;
        char *ipstr = malloc(INET6_ADDRSTRLEN);

        // Get the pointer to the address itself
        if (rp->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        // Convert the IP address to a string
        inet_ntop(rp->ai_family, addr, ipstr, INET6_ADDRSTRLEN);

        // Free the memory allocated by getaddrinfo
        freeaddrinfo(result);

        return ipstr;
    }

    // Free the memory allocated by getaddrinfo
    freeaddrinfo(result);

    return NULL;  // Return NULL if no IP address is found
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hostname or IP address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Get the IP address using the function
    char *ipAddress = getIPAddress(argv[1]);

    if (ipAddress != NULL) {
        printf("IP address: %s\n", ipAddress);
        free(ipAddress);  // Free the memory allocated by getIPAddress
    } else {
        printf("Unable to retrieve IP address.\n");
    }

    return 0;
}