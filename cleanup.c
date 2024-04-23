#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

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
{   int flag;
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

int main()
{
    key_t key;
    
    struct msg_buffer msg;
    char var;
    key = ftok("plane.c", 'x');
    if (key == -1)
    {
        printf("error in creating unique key\n");
        exit(1);
    }

    const int msg_queue_id = msgget(key, 0666 | IPC_CREAT); // Create a message queue
    if (msg_queue_id == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No)\n");
        scanf(" %c", &var);

        msg.msg_type = 100;
        msg.notification.flag = 2;
        msg.notification.completionStatus = 100;
        msg.plane.planeID =0;
        msg.plane.planeType = 0;
        msg.plane.numOccupiedSeats = 0;
        msg.plane.totalLuggageWeight = 0;
        msg.plane.totalPassengerWeight = 0;
        msg.plane.totalCrewWeight = 0;
        msg.plane.departureAirport = 0;
        msg.plane.arrivalAirport = 0;
        msg.plane.totalPlaneWeight = 0;
        msg.notification.kill_status = 0;
 
        if (var == 'Y')
        {
            if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1)
            { // Send message to the message queue
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            return 0;

        }
    }

    return 0;
}
