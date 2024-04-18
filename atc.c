#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_TEXT_SIZE 100

struct PlaneInfo {
    int planeID;
    int planeType;
    int numOccupiedSeats;
    int totalLuggageWeight;
    int totalPassengerWeight;
    int totalCrewWeight;
    int departureAirport;
    int arrivalAirport;
    int totalPlaneWeight;
};

struct NotificationMessage {
    int completionStatus;
};

struct msg_buffer {
    long msg_type;
    struct PlaneInfo plane;
    struct NotificationMessage notification;
};

struct AirportStatus {
    int airport_id;
    int num_planes;
};

#define PLANE_MSG_TYPE_START 1
#define PLANE_MSG_TYPE_END 10
#define AIRPORT_MSG_TYPE_START 11
#define AIRPORT_MSG_TYPE_END 20
#define TERMINATION_MSG_TYPE 99

int main() {
    int num_airports;
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    key_t key;
    int msgid;
    key = ftok("plane", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    int num_planes_landed = 0;
    struct AirportStatus airport_statuses[num_airports];

    for (int i = 0; i < num_airports; i++) {
        airport_statuses[i].airport_id = i + 1;
        airport_statuses[i].num_planes = 0;
    }

    struct msg_buffer message;

    // Flag to track if termination request has been received
    int termination_requested = 0;

    while (1) {
        // Receive a message without specifying the type
        msgrcv(msgid, &message, sizeof(message), 0, 0);

        // Check if termination request received
        if (message.msg_type == TERMINATION_MSG_TYPE) {
            printf("Received termination request from cleanup process.\n");
            termination_requested = 1;
            continue;
        }

        if (termination_requested) {
            // Inform planes that no further departures will happen
            message.notification.completionStatus = 0; // 0 represents termination notification
            message.msg_type = TERMINATION_MSG_TYPE; // Message type for termination
            msgsnd(msgid, &message, sizeof(message), 0);
            
            // Inform airports to terminate
            for (int i = 1; i <= num_airports; i++) {
                message.notification.completionStatus = 2; // 2 represents termination signal for airports
                message.msg_type = AIRPORT_MSG_TYPE_START + i;
                msgsnd(msgid, &message, sizeof(message), 0);
            }
            
            // Cleanup: Remove message queue
            msgctl(msgid, IPC_RMID, NULL);
            break;
        }

        if (message.msg_type >= PLANE_MSG_TYPE_START && message.msg_type <= PLANE_MSG_TYPE_END) {
            // Process the received message as a message from a plane
            printf("Received message from plane: PlaneID: %d, DepartureAirport: %d, ArrivalAirport: %d\n",
                   message.plane.planeID, message.plane.departureAirport, message.plane.arrivalAirport);
        
            // If termination requested, reject further departures
            if (termination_requested) {
                // Inform plane that no further departures will happen
                message.notification.completionStatus = 0; // 0 represents termination notification
                message.msg_type = message.plane.planeID;
                msgsnd(msgid, &message, sizeof(message), 0);
                continue; // Skip processing further actions for this plane
            }
        
            // Extract information from message structure
            int plane_id = message.plane.planeID;
            int departure_airport_id = message.plane.departureAirport;
            int arrival_airport_id = message.plane.arrivalAirport;
        
            // Perform actions as per the received message
            // (You can add your logic here)
        
            // Send appropriate responses
            // Boarding and takeoff start message
            message.notification.completionStatus = 3; // Example completion status
            message.msg_type = departure_airport_id;
            msgsnd(msgid, &message, sizeof(message), 0);
        
            // Receive takeoff completion message
            msgrcv(msgid, &message, sizeof(message.notification), departure_airport_id + 10, 0);
            printf("Received takeoff completion message from departure airport\n");
        
            // Update departure airport status
            airport_statuses[departure_airport_id - 1].num_planes--;
        
            // Arrival notification message
            message.notification.completionStatus = 4; // Example completion status
            message.msg_type = arrival_airport_id;
            msgsnd(msgid, &message, sizeof(message), 0);
        
            // Update arrival airport status
            airport_statuses[arrival_airport_id - 1].num_planes++;
        
            // Receive landing and deboarding completion message
            msgrcv(msgid, &message, sizeof(message.notification), arrival_airport_id + 10, 0);
            printf("Received landing and deboarding completion message from arrival airport\n");
        
            // Update total planes landed count
            num_planes_landed++;
        
            // Send completion message to plane
            message.notification.completionStatus = 5; // Example completion status
            message.msg_type = plane_id;
            msgsnd(msgid, &message, sizeof(message), 0);
        
            // Log departure and arrival information to file
            FILE *file = fopen("AirTrafficController.txt", "a");
            if (file != NULL) {
                fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane_id, departure_airport_id, arrival_airport_id);
                fclose(file);
            }
        } 
    }

    return 0;
}
