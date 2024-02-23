#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function definition
char* getUserInput() {
    char userInput[100]; // Assuming a maximum input length of 100 characters

    printf("Enter something: ");

    if (fgets(userInput, sizeof(userInput), stdin) != NULL) {
        // Remove the newline character at the end of the input
        size_t len = strlen(userInput);
        if (len > 0 && userInput[len - 1] == '\n') {
            userInput[len - 1] = '\0';
        }

        // Allocate memory for the result and copy the input
        char* result = malloc(strlen(userInput) + 1);
        if (result != NULL) {
            strcpy(result, userInput);
            return result;
        } else {
            perror("Memory allocation error");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("Error reading user input");
        exit(EXIT_FAILURE);
    }
}
