#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

typedef enum
{
    TOKEN,
    DATA
} FrameType;

struct Frame
{
    FrameType type;
    int sender_id;
    char payload[BUFFER_SIZE];
};

void start_node(int my_id, int my_port, int successor_port, int has_token)
{
    int sockfd;
    struct sockaddr_in my_addr, next_addr;
    struct Frame frame;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Create UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = INADDR_ANY;
    my_addr.sin_port = htons(my_port);

    memset(&next_addr, 0, sizeof(next_addr));
    next_addr.sin_family = AF_INET;
    next_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    next_addr.sin_port = htons(successor_port);

    if (bind(sockfd, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if (has_token)
        {
            printf("\n[Node %d] I have the token.\n", my_id);

            // Simulate Data Transmission
            printf("[Node %d] Sending data frame...\n", my_id);
            frame.type = DATA;
            frame.sender_id = my_id;
            strcpy(frame.payload, "Sample Data Packet");
            // In a bus, we'd broadcast; here we send to successor to pass through
            sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&next_addr, addr_len);

            // Pass Token
            sleep(2); // Slow down for visibility
            printf("[Node %d] Passing token to port %d...\n", my_id, successor_port);
            frame.type = TOKEN;
            sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&next_addr, addr_len);

            has_token = 0;
        }
        else
        {
            printf("[Node %d] Waiting for token/data...\n", my_id);
            recvfrom(sockfd, &frame, sizeof(frame), 0, NULL, NULL);

            if (frame.type == DATA)
            {
                printf("[Node %d] Received DATA from Node %d: %s\n", my_id, frame.sender_id, frame.payload);
            }
            else if (frame.type == TOKEN)
            {
                has_token = 1;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        printf("Usage: %s <ID> <MyPort> <SuccessorPort> [initial_token: 1/0]\n", argv[0]);
        return 1;
    }

    int my_id = atoi(argv[1]);
    int my_port = atoi(argv[2]);
    int successor_port = atoi(argv[3]);
    int has_token = (argc == 5) ? atoi(argv[4]) : 0;

    start_node(my_id, my_port, successor_port, has_token);

    return 0;
}