#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUS_PORT 8888
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
    int destination_id; // Logical successor or data recipient
    char payload[BUFFER_SIZE];
};

void run_token_bus(int my_id, int total_nodes, int has_token)
{
    int sockfd;
    struct sockaddr_in bus_addr, sender_addr;
    struct Frame frame;
    int broadcast_permission = 1;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Create UDP Socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Allow broadcasting
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission));

    // Allow multiple instances to bind to the same port for local simulation
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&bus_addr, 0, sizeof(bus_addr));
    bus_addr.sin_family = AF_INET;
    bus_addr.sin_port = htons(BUS_PORT);
    bus_addr.sin_addr.s_addr = INADDR_ANY; // Listen to all traffic on the bus

    bind(sockfd, (struct sockaddr *)&bus_addr, sizeof(bus_addr));

    // Setup Broadcast Address for sending
    struct sockaddr_in send_addr;
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sin_family = AF_INET;
    send_addr.sin_port = htons(BUS_PORT);
    send_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    int successor_id = (my_id % total_nodes) + 1; // Logical Ring: 1->2->3->1

    while (1)
    {
        if (has_token)
        {
            printf("\n[Node %d] Accessing Bus...\n", my_id);

            // 1. Broadcast Data (Simulated)
            frame.type = DATA;
            frame.sender_id = my_id;
            frame.destination_id = 0; // 0 = Broadcast to all
            strcpy(frame.payload, "Bus Traffic");
            sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&send_addr, addr_len);

            sleep(1);

            // 2. Pass Token to Successor
            printf("[Node %d] Passing Token to Node %d\n", my_id, successor_id);
            frame.type = TOKEN;
            frame.destination_id = successor_id;
            sendto(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&send_addr, addr_len);

            has_token = 0;
        }
        else
        {
            recvfrom(sockfd, &frame, sizeof(frame), 0, (struct sockaddr *)&sender_addr, &addr_len);

            // Ignore our own broadcasted messages
            if (frame.sender_id == my_id)
                continue;

            if (frame.type == DATA)
            {
                printf("[Node %d] Bus Sense: Data from Node %d\n", my_id, frame.sender_id);
            }
            else if (frame.type == TOKEN && frame.destination_id == my_id)
            {
                printf("[Node %d] Token received. My turn.\n", my_id);
                has_token = 1;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <MyID> <TotalNodes> [StartToken: 1]\n", argv[0]);
        return 1;
    }
    int my_id = atoi(argv[1]);
    int total_nodes = atoi(argv[2]);
    int has_token = (argc == 4 && strcmp(argv[3], "1") == 0);

    run_token_bus(my_id, total_nodes, has_token);
    return 0;
}