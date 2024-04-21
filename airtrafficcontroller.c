#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdbool.h> // Include for boolean data type

// #define P_MSG_TYPE 1
// #define A_MSG_TYPE 2 
#define TERMINATION_MSG_TYPE 3 // New termination message type

int active_planes = 0; // Number of active planes

struct p_message //plane
{
    long msg_type;
    int plane_ID;
    int a_ID;   // Departure ID
    int dest_ID; // Destination ID
    int status;  // 0: start, 1: finish
    int type;    // 0: departure, 1: arrival
    int action;  // 0: boarding, 1: deboarding
    int kill;
};

struct a_message //airport
{
    long msg_type;
    int pl_ID;
    int dep_ID;
    int arr_ID;
    int kill;
};

// Function prototypes
void process_amsg(struct a_message msg, int msg_id);
void process_pmsg(struct p_message msg, int msg_id);
void process_tmsg(int active_planes, int num_airports, int msg_id, bool termination_requested);
void process_aft_termination_req(int pl_ID, int msg_id);

int main()
{
    int msg_id;
    struct p_message p_msg;
    struct a_message a_msg;
    struct p_message termination_msg;   // New termination message
    bool termination_requested = false; // Flag to track termination request

    int num_airports;

    // Create or get the message queue
    key = ftok("plane.c", 'x');
    if (key == -1)
    {
        printf("error in creating unique key\n");
        exit(1);
    }

    msg_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_id == -1)
    {
        perror("msgget");
        exit(1);
    }

    // Message queue setup complete
    printf("Message queue created with ID: %d\n", msg_id);

    // Get the number of airports
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    // Receiving and processing messages in a loop
    while (1)
    {
        // Receiving a_message from a1.c
        if (msgrcv(msg_id, &a_msg, sizeof(struct a_message), A_MSG_TYPE, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        // Processing a_message
        if (!termination_requested)
        {
            process_amsg(a_msg, msg_id); // Pass msg_id
            active_planes++;
        }

        if (termination_requested)
        {
            process_aft_termination_req(a_msg.pl_ID, msg_id);
            active_planes--;
        }

        // Receiving p_message from p1.c
        if (msgrcv(msg_id, &p_msg, sizeof(struct p_message), P_MSG_TYPE, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        // Processing p_message
        process_pmsg(p_msg, msg_id);

        // Receiving termination message
        if (!termination_requested && msgrcv(msg_id, &termination_msg, sizeof(struct p_message), TERMINATION_MSG_TYPE, IPC_NOWAIT) != -1)
        {
            // Process termination message
            termination_requested = true; // Set termination flag
        }

        process_tmsg(active_planes, num_airports, msg_id, termination_requested);
    }

    return 0;
}

// Function to process a_message
void process_amsg(struct a_message msg, int msg_id)
{
    struct p_message p_msg;

    // Prepare the p_message
    p_msg.msg_type = P_MSG_TYPE;
    p_msg.pl_ID = msg.pl_ID;
    p_msg.a_ID = msg.dep_ID;
    p_msg.dest_ID = msg.arr_ID;
    p_msg.status = 0; // Assuming it's a start status
    p_msg.type = 0;   // Assuming it's a departure type
    p_msg.action = 0; // Assuming it's a boarding action
    p_msg.kill = 0;

    // Sending the p_message to p1.c
    if (msgsnd(msg_id, &p_msg, sizeof(struct p_message), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    printf("Forwarded a_message to airport.c with pl_ID: %d, a_ID: %d, and dest_ID: %d\n", msg.pl_ID, msg.dep_ID, msg.arr_ID);
}

// Function to process p_message
void process_pmsg(struct p_message msg, int msg_id)
{
    struct a_message a_msg;

    // Check if the received message has attributes { dest_ID = 0, status = 1, type = 1, action = 1}
    if (msg.dest_ID == 0 && msg.status == 1 && msg.type == 1 && msg.action == 1)
    {
        // Prepare the new a_message
        a_msg.msg_type = A_MSG_TYPE;
        a_msg.pl_ID = msg.pl_ID;
        a_msg.dep_ID = 0;
        a_msg.arr_ID = msg.a_ID;
        a_msg.kill = 1;

        // Sending the new a_message
        if (msgsnd(msg_id, &a_msg, sizeof(struct a_message), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        active_planes--;

        printf("Sent a_message with pl_ID: %d, dep_ID: 0, arr_ID: %d, and kill: 1\n", msg.pl_ID, msg.a_ID);
    }

    // Check if the received message has attributes { dest_ID = 0, status = 1, type = 0, action = 0, kill = 0}
    if (msg.dest_ID == 0 && msg.status == 1 && msg.type == 0 && msg.action == 0 && msg.kill == 0)
    {
        // Prepare and send a new p_message
        struct p_message new_p_msg;
        new_p_msg.msg_type = P_MSG_TYPE;
        new_p_msg.a_ID = msg.dest_ID;
        new_p_msg.dest_ID = 0;
        new_p_msg.type = 1;
        new_p_msg.status = 0;
        new_p_msg.action = 1;
        new_p_msg.kill = 0;

        // Sending the new p_message
        if (msgsnd(msg_id, &new_p_msg, sizeof(struct p_message), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        printf("Sent p_message with a_ID: %d, dest_ID: 0, type: 1, status: 0, and action: 1\n", msg.dest_ID);
    }
}

// Function to process termination message
void process_tmsg(int active_planes, int num_airports, int msg_id, bool termination_requested)
{
    // Process termination message here
    if (termination_requested)
    {
        printf("Received termination message. Number of active planes: %d.\n", active_planes);

        if (active_planes == 0)
        {
            // Send p_message for all airports
            for (int airport_ID = 1; airport_ID <= num_airports; airport_ID++)
            {
                struct p_message p_msg;
                p_msg.msg_type = P_MSG_TYPE;
                p_msg.pl_ID = 0; // Set pl_ID to 0
                p_msg.a_ID = airport_ID;
                p_msg.dest_ID = 0;
                p_msg.status = 0;
                p_msg.type = 0;
                p_msg.action = 0;
                p_msg.kill = 1;

                // Sending the p_message
                if (msgsnd(msg_id, &p_msg, sizeof(struct p_message), 0) == -1)
                {
                    perror("msgsnd");
                    exit(1);
                }

                printf("Sent p_message with pl_ID: 0, a_ID: %d, kill: 1\n", airport_ID);
            }

            // Destroy the message queue using msgctl
            if (msgctl(msg_id, IPC_RMID, NULL) == -1)
            {
                perror("msgctl");
                exit(1);
            }
            printf("Message queue destroyed.\n");
        }
    }
}

// Function to process a_messages after termination
void process_aft_termination_req(int pl_ID, int msg_id)
{
    struct a_message a_msg;

    // Prepare the new a_message
    a_msg.msg_type = A_MSG_TYPE;
    a_msg.pl_ID = pl_ID;
    a_msg.dep_ID = 0;
    a_msg.arr_ID = 0;
    a_msg.kill = 2;

    // Sending the new a_message
    if (msgsnd(msg_id, &a_msg, sizeof(struct a_message), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    printf("Processed a_message after termination with plane_ID: %d and kill: 2\n", pl_ID);
}
