#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_SIZE 1024

int sockfd;
char NAME[64];
int GAME_SCORE = -1;
short int TURN_SWITCH = 0;

struct my_msg {
    char name[64];
    char text[1024];
    int score;
};

void sigint_handler() {
    close(sockfd);
    printf("\nProgram closed.\n");
    exit(0);
}

int is_number(char str[], int len) {
    for(int i = 0; i < len; i++) {
        if(((str[i] >= '0' && str[i] <= '9') || str[i] == '-') == 0) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <address> <port> [name]\n", argv[0]);
        exit(1);
    }

    if (argc == 4) {
        strcpy(NAME, argv[3]);
    } else {
        strcpy(NAME, argv[1]);
    }

    signal(SIGINT, sigint_handler);

    srand(time(NULL));

    u_short my_port;
    struct sockaddr_in server_addr, peer_addr;
    struct my_msg msg;
    socklen_t peer_addr_len = sizeof(peer_addr);
    fd_set readfds;
    char buffer[BUFFER_SIZE];

    // Getting port number from argument
    my_port = atoi(argv[2]);

    // Creating socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        printf("socket ERROR\n");
        exit(1);
    }

    // Filling the server_addr structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(my_port);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("inet_pton ERROR\n");
        exit(1);
    }

    // Trying to bind socket (server)
    int is_server = (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0);

    if (is_server) {
        TURN_SWITCH = 1;
        printf("[%s]: Socket created, listening on port: %d\n", NAME, my_port);
    } else {
        printf("[%s]: Connection with server at address %s on port %d\n", NAME, argv[1], my_port);
        // On client side, after opening the socket:
        snprintf(msg.name, sizeof(msg.name), "%s", NAME);
        snprintf(msg.text, sizeof(msg.text), "Connection opened with %s", NAME);
        sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    // Setting socket to non-blocking mode
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Main loop
    while (GAME_SCORE < 50) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_fd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        // Listening for events (socket and stdin)
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            printf("select ERROR\n");
            break;
        }

        // Receiving messages
        if (FD_ISSET(sockfd, &readfds)) {
            ssize_t recv_len = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&peer_addr, &peer_addr_len);
            if (recv_len > 0) {
                // Initial condition
                if(strstr(msg.text, "Connection opened with ") != NULL) {
                    strcpy(msg.text, "SET_STARTING_SCORE");
                    GAME_SCORE = rand() % 10 + 1;
                    msg.score = GAME_SCORE;
                    if(sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&peer_addr, peer_addr_len)) {
                        printf("GAME STARTED, INITIAL SCORE: %d\n", GAME_SCORE);
                    }
                }

                // Setting the initial value
                if(strcmp(msg.text, "SET_STARTING_SCORE") == 0) {
                    if (is_server == 0) {
                        GAME_SCORE = msg.score;
                        printf("GAME STARTED, INITIAL SCORE: %d\n", GAME_SCORE);
                    }
                    continue;
                }

                // Checking for 'end' command
                if (strcmp(msg.text, "end") == 0) {
                    printf("Received end command - closing program...\n");
                    close(sockfd);
                    exit(0);
                }

                // Checking if it's the opponent's move
                if (GAME_SCORE < msg.score) {
                    printf("Opponent made a move!\n");
                    GAME_SCORE = msg.score;
                    TURN_SWITCH = 1;
                }

                // Printing the message
                printf("[%s]: %s\n", msg.name, msg.text);

                if(GAME_SCORE >= 50) {
                    printf("\n--------------------------\n\nPlayer %s wins!\n\n--------------------------\n\n", msg.name);
                    close(sockfd);
                    exit(0);
                }
            }
        }

        // Sending messages
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(buffer, BUFFER_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Removing newline character

            // If player wants to act before the game starts
            if (GAME_SCORE < 0) {
                printf("Wait for the second player to join!\n");
                continue;
            }

            if (strlen(buffer) > 0) {
                snprintf(msg.name, sizeof(msg.name), "%s", NAME);
                snprintf(msg.text, sizeof(msg.text), "%s", buffer);

                if (is_number(msg.text, strlen(msg.text))) {
                    if (TURN_SWITCH == 0) {
                        printf("It's not your turn yet! Wait for the opponent's move\n");
                        continue;
                    }
                    if (atoi(msg.text) - GAME_SCORE > 10 || atoi(msg.text) - GAME_SCORE < 1) {
                        printf("The difference should be in the range [1, 10]!\n");
                        continue;
                    }
                    GAME_SCORE = atoi(msg.text);
                    msg.score = atoi(msg.text);
                    TURN_SWITCH = 0;
                }

                if (is_server) {
                    // Server sends the message back to the last client
                    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&peer_addr, peer_addr_len);
                } else {
                    // Client sends the message to the server
                    sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                }

                if(strcmp(msg.text, "end") == 0) {
                    printf("Detected end command - closing program...\n");
                    close(sockfd);
                    exit(0);
                }

                printf("[%s]: %s\n", NAME, msg.text);

                if(GAME_SCORE >= 50) {
                    printf("\n--------------------------\n\nPlayer %s wins!\n\n--------------------------\n\n", msg.name);
                    close(sockfd);
                    exit(0);
                }
            }
        }
    }

    sigint_handler();

    return 0;
}
