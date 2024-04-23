#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdbool.h>

int active_planes = 0;

struct PlaneInfo
{
    int planeID;
    int planeType;

    int numOccupiedSeats;   // or total number of cargo
    int totalLuggageWeight; // or total cargo weight

    int totalPassengerWeight;
    int totalCrewWeight;

    int departureAirport;
    int arrivalAirport;

    int totalPlaneWeight;
};
struct NotificationMessage
{
    int kill_status;      // 0: don't kill, 1: self kill for plane/airport, 2: force kill for plane
    int completionStatus; // This field can indicate the status of the deboarding/unloading process
    // 1 : departure plane started boarding
    // 2 : departure plane finished boarding, now in flight
    // 4 : departure plane finished deboarding
};

struct msg_buffer
{
    long msg_type;                           // the type of message
    struct PlaneInfo plane;                  // the plane
    struct NotificationMessage notification; // the notification
};

// Function prototypes
void stage2(struct msg_buffer, int msg_queue_id);                                      // planes
void stage4(struct msg_buffer, int msg_queue_id);                                      // airports
void process_tmsg(struct msg_buffer, int active_planes, int num_airports, int msg_queue_id); // termination
void process_aft_termination_req(struct msg_buffer, int msg_queue_id);                       // planes

int main()
{
    key_t key;
    struct msg_buffer msg;
    bool termination_requested = false; // Flag to track termination request

    int num_airports;

    key = ftok("plane.c", 'x'); // change path to atc.c when that file is made for uniformity.
    if (key == -1)
    {
        printf("error in creating unique key\n");
        exit(1);
    }

    // Create or get the message queue
    const int msg_queue_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_queue_id == -1)
    {
        perror("msgget");
        exit(1);
    }

    // Message queue setup complete
    printf("Message queue created with ID: %d\n", msg_queue_id);

    // Get the number of airports
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    // Receiving and processing messages in an infinte loop
    while (1)
    {

        if (termination_requested)
            process_tmsg(msg, active_planes, num_airports, msg_queue_id);

        // Receiving a_message
        if (msgrcv(msg_queue_id, &msg, sizeof(struct msg_buffer), 0, 0) == -1)
        {
            perror("msgrcv");
            exit(1);
        }

        if (msg.msg_type >= 1 && msg.msg_type <= 10)
        { // Plane to ATC was received, now stage 2 begins
            if (!termination_requested)
            {
                stage2(msg, msg_queue_id);
                active_planes++;
            }

            // Processing a_message from planes when termination requested
            if (termination_requested)
            {
                process_aft_termination_req(msg, msg_queue_id);
                active_planes--;
            }
        }

        if (msg.msg_type >=21 && msg.msg_type <= 30)
            // Airport to ATC was done, now srage 4 begins
            stage4(msg, msg_queue_id);

        // Receiving termination message
        if (!termination_requested && msg.msg_type == 404)
        {
            // Process termination message from cleanup
            termination_requested = true; // Set termination flag
        }

        // if both conditions met, destroy message queue and exit loop
        if (termination_requested && active_planes == 0)
        {
            if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1)
            {
                perror("msgctl");
                exit(EXIT_FAILURE);
            }
            printf("Message queue destroyed.\n");
            break;
        }
    }

    return 0;
}

// Function to process a_message from plane
// Plane sends message to atc only once, so only one case : atc extracts the info from plane message and forwards it to the departure airport using p_message
void stage2(struct msg_buffer msg, int msg_queue_id)
{

    printf("Received flight begin message from plane: %d \n", msg.plane.planeID);
    printf("type is %d, i will make it %d", msg.msg_type, msg.msg_type+10);
    /*msg.msg_type = msg.msg_type+10;
    msg.notification.kill_status = 0;

    // Sending the new a_message
    if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    } */
    msg.msg_type = msg.msg_type + 10;
    msg.notification.kill_status = 0;
    msg.notification.completionStatus = 1;

    // Sending the p_message to airport.c
    if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }
}

// Function to process p_message from airports
// Now there are two cases when atc receives a message from plane : one when boarding complete and one when deboarding complete
void stage4(struct msg_buffer msg, int msg_queue_id)
{

    // Deboarding complete case: We need to let the plane know that it needs to shut down, for this airport.c needs to send dest_ID as 0 because plane would go nowhere next)
    if (msg.notification.completionStatus == 4)
    {
        // Prepare the new a_message
	printf("type is %d, i will make it %d", msg.msg_type, msg.msg_type+10);
        msg.msg_type = msg.msg_type+10;
        msg.notification.kill_status = 1;
        msg.notification.completionStatus = 4;

        // Sending the new a_message to respective plane
        if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        active_planes--; // signifying this particular plane that the message has been sent to, is no longer active

        printf("Sent a_message with pl_ID: %d and kill: 1\n", msg.plane.planeID);
    }

    // Boarding complete case: We need to let the arrival airport know that flight is about to land,
    if (msg.notification.completionStatus == 2)
    {

        msg.msg_type = msg.plane.arrivalAirport + 10;
        msg.notification.kill_status = 0;
        msg.notification.completionStatus = 2;

        // Sending the new p_message
        if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
        {
            perror("msgsnd");
            exit(1);
        }

        FILE *file = fopen("AirTrafficController.txt", "a");
        if (file != NULL)
        {
            fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", msg.plane.planeID, msg.plane.departureAirport, msg.plane.arrivalAirport);
            fclose(file);
        }
        else
        {
            printf("Error opening file!\n");
        }
    }
}

// Function to process termination message
void process_tmsg(struct msg_buffer msg, int active_planes, int num_airports, int msg_queue_id)
{
    // Process termination message here
    printf("Received termination message. Number of active planes: %d.\n", active_planes);

    if (active_planes == 0)
    {
        // Send p_message for all airports
        for (int airport_ID = 1; airport_ID <= num_airports; airport_ID++)
        {

            msg.msg_type = airport_ID + 10;
            msg.notification.kill_status = 1;

            // Sending the p_message
            if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
            {
                perror("msgsnd");
                exit(1);
            }

            printf("Sent p_message with pl_ID: 0, a_ID: %d, kill: 1\n", airport_ID);
        }
    }
}

// Function to process a_messages after termination
void process_aft_termination_req(struct msg_buffer msg, int msg_queue_id)
{

    msg.msg_type = msg.plane.planeID+30;
    msg.notification.kill_status = 2;

    // Sending the new a_message
    if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    printf("Processed a_message after termination with pl_ID: %d and kill: 2\n", msg.plane.planeID);
}