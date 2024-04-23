#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_AIRPORTS 10
#define PLANE_TO_ATC_TYPE 100
#define TERMINATION_TYPE 2
#define BOARDING_COMPLETE_STATUS 2
#define DEBOARDING_COMPLETE_STATUS 4
#define PLANE_TO_ATC_FLAG 0
#define AIRPORT_TO_ATC_FLAG 1
#define TERMINATION_FLAG 2
#define PLANE_ARRIVAL_FLAG 4

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
    int flag;
    int kill_status;
    int completionStatus;
};

struct msg_buffer {
    long msg_type;
    struct PlaneInfo plane;
    struct NotificationMessage notification;
};

void stage2(struct msg_buffer msg, int msg_queue_id);
void stage4(struct msg_buffer msg, int msg_queue_id);
void process_tmsg(struct msg_buffer msg, int active_planes, int num_airports, int msg_queue_id);
void process_aft_termination_req(struct msg_buffer msg, int msg_queue_id);

int active_planes = 0;

int main() {
    key_t key;
    struct msg_buffer msg;
    bool termination_requested = false;
    int num_airports;

    key = ftok("plane.c", 'x');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    const int msg_queue_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    printf("Message queue created with ID: %d\n", msg_queue_id);

    printf("Enter the number of airports to be handled/managed: ");
    if (scanf("%d", &num_airports) != 1) {
        printf("Invalid input\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (termination_requested)
            process_tmsg(msg, active_planes, num_airports, msg_queue_id);

        if (msgrcv(msg_queue_id, &msg, sizeof(struct msg_buffer), PLANE_TO_ATC_TYPE, 0) == -1) {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }

        printf("Message received with type: %ld and status: %d\n", msg.msg_type, msg.notification.completionStatus);

        if (msg.notification.flag == PLANE_TO_ATC_FLAG) {
            if (!termination_requested) {
                stage2(msg, msg_queue_id);
                active_planes++;
            } else {
                process_aft_termination_req(msg, msg_queue_id);
                active_planes--;
            }
        }

        if (msg.notification.flag == AIRPORT_TO_ATC_FLAG) {
            stage4(msg, msg_queue_id);
        }

        if (!termination_requested && msg.notification.flag == TERMINATION_FLAG) {
            termination_requested = true;
            msg.msg_type = 500;
            if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }  
        }

        if (termination_requested && active_planes == 0) {
            if (msgctl(msg_queue_id, IPC_RMID, NULL) == -1) {
                perror("msgctl");
                exit(EXIT_FAILURE);
            }
            printf("Message queue destroyed.\n");
            break;
        }
    }

    return 0;
}

void stage2(struct msg_buffer msg, int msg_queue_id) {
    printf("Received flight begin message from plane: %d \n", msg.plane.planeID);
    msg.msg_type = msg.plane.departureAirport + 10;
    msg.notification.flag = AIRPORT_TO_ATC_FLAG;
    msg.notification.kill_status = 0;
    msg.notification.completionStatus = 1;

    if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}

void stage4(struct msg_buffer msg, int msg_queue_id) {
    if (msg.notification.completionStatus == DEBOARDING_COMPLETE_STATUS) {
        msg.msg_type = msg.plane.planeID + 30;
        msg.notification.kill_status = 1;
        msg.notification.completionStatus = DEBOARDING_COMPLETE_STATUS;

        if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        active_planes--;

        printf("Sent message with plane ID: %d and kill: 1\n", msg.plane.planeID);
    }

    if (msg.notification.completionStatus == BOARDING_COMPLETE_STATUS) {
        printf("ATC will now send arrival message to arrival airport\n");
        msg.msg_type = msg.plane.arrivalAirport + 10;
        msg.notification.flag = PLANE_ARRIVAL_FLAG;
        msg.notification.kill_status = 0;
        msg.notification.completionStatus = BOARDING_COMPLETE_STATUS;

        if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
            perror("msgsnd");
            exit(EXIT_FAILURE);
        }

        FILE *file = fopen("AirTrafficController.txt", "a");
        if (file != NULL) {
            fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", msg.plane.planeID, msg.plane.departureAirport, msg.plane.arrivalAirport);
            fclose(file);
        } else {
            printf("Error opening file!\n");
        }
    }
}

void process_tmsg(struct msg_buffer msg, int active_planes, int num_airports, int msg_queue_id) {
    printf("Received termination message. Number of active planes: %d.\n", active_planes);

    if (active_planes == 0) {
        for (int airport_ID = 1; airport_ID <= num_airports; airport_ID++) {
            msg.msg_type = airport_ID + 10;
            msg.notification.kill_status = 1;

            if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            printf("Sent message with plane ID: 0, airport ID: %d, kill: 1\n", airport_ID);
        }
    }
}

void process_aft_termination_req(struct msg_buffer msg, int msg_queue_id) {
    msg.msg_type = msg.plane.planeID + 30;
    msg.notification.kill_status = 2;

    if (msgsnd(msg_queue_id, &msg, sizeof(struct msg_buffer), 0) == -1) {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }

    printf("Processed message after termination with plane ID: %d and kill: 2\n", msg.plane.planeID);
}

