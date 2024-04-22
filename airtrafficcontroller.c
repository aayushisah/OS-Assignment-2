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
    //1 : departure plane started boarding
    //2 : departure plane finished boarding, now in flight
    //4 : departure plane finished deboarding

};

struct msg_buffer
{
    long msg_type;                           // the type of message
    struct PlaneInfo plane;                  // the plane
    struct NotificationMessage notification; // the notification
};

/*
//for messages to and from airport
struct p_message {
    long msg_type; // airport's message type always equal to airport_ID+10
    int pl_ID;     // ID of plane associated
    int a_ID;      // ID of present airport
    int dest_ID;   // where corresponding plane wants to reach next
    int tot_weight;
    int plane_type;
    int passenger_count;
    int status;    // 0: start action, 1: finish action
    int type;      // 0: departure, 1: arrival
    int action;    // 0: boarding, 1: deboarding
    int kill;      // close airport
};

//for messages to and from plane
struct a_message {
    long msg_type;     //plane's message type always plane_ID
    int pl_ID;         //ID of present plane
    int dep_ID;        //departure airport
    int arr_ID;        //arrival airport
    int tot_weight;    // total weight of plane
    int plane_type;
    int passenger_count;
    int kill;          // 0: no kill, 1: self kill, 2: force kill
}; 

struct t_message
{
    long msg_type;      //type always 404
};*/

// Function prototypes
void process_amsg(struct msg_buffer, int msg_queue_id);   //planes
void process_pmsg(struct msg_buffer, int msg_queue_id);   //airports
void process_tmsg(int active_planes, int num_airports, int msg_queue_id); //termination
void process_aft_termination_req(int plane_ID, int msg_queue_id);  //planes

int main() {
    key_t key;
    int msg_queue_id;
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
    msg_queue_id = msgget(key, 0666 | IPC_CREAT);
    if (msg_queue_id == -1) {
        perror("msgget");
        exit(1);
    }

    // Message queue setup complete
    printf("Message queue created with ID: %d\n", msg_queue_id);

    // Get the number of airports
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    // Receiving and processing messages in an infinte loop
    while (1) {

        if (termination_requested)
           process_tmsg(active_planes, num_airports, msg_queue_id); 

        // Receiving a_message from plane.c
        if (msgrcv(msg_queue_id, &msg, sizeof(struct msg_buffer), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }
        
        if (msg.msg_type>=1 && msg.msg_type<=10)
        {// Processing a_message from planes when termination not requested yet
        if (!termination_requested) {
            process_amsg(msg, msg_queue_id); 
            active_planes++;
        }
        
        // Processing a_message from planes when termination requested
        if (termination_requested) {
            process_aft_termination_req(msg.plane.planeID, msg_queue_id);
            active_planes--;
        }
        }
        
        if (msg.msg_type>=11 && msg.msg_type<=20)
        // Processing p_message from airports
        process_pmsg(msg, msg_queue_id);

        // Receiving termination message
        if (!termination_requested && msg.msg_type==404) {
            // Process termination message from cleanup
            termination_requested = true; // Set termination flag
        }

        //if both conditions met, destroy message queue and exit loop
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
void process_amsg(struct msg_buffer msg, int msg_queue_id) {
    struct msg_buffer boardingstart;

    // Prepare the p_message (message signifies that plane is departure type and is about to start boarding according to the flags)
    // Instructs departure airport to start boarding process
    /*p_msg.msg_type = msg.dep_ID+10;
    p_msg.pl_ID = msg.pl_ID;
    p_msg.a_ID = msg.dep_ID; // message is intended for departure airport (airport.c needs to check if (airport_ID == p_msg.a_ID) to ensure that the message is meant for that particular airport) 
    p_msg.dest_ID = msg.arr_ID;
    p_msg.tot_weight=msg.tot_weight;
    p_msg.plane_type=msg.plane_type;
    p_msg.passenger_count=msg.passenger_count;
    p_msg.status = 0; // Assuming it's a start status
    p_msg.type = 0;   // Assuming it's a departure type
    p_msg.action = 0; // Assuming it's a boarding action
    p_msg.kill = 0;*/

    printf("Received flight begin message from plane: %d \n", msg.plane.planeID);

    boardingstart.msg_type = msg.plane.departureAirport+10;
    boardingstart.plane.planeID = msg.plane.planeID;
    boardingstart.plane.planeType = msg.plane.planeType;
    boardingstart.plane.numOccupiedSeats = msg.plane.numOccupiedSeats;   
    boardingstart.plane.totalLuggageWeight = msg.plane.totalLuggageWeight; 
    boardingstart.plane.totalPassengerWeight = msg.plane.totalPassengerWeight;
    boardingstart.plane.totalCrewWeight = msg.plane.totalCrewWeight;
    boardingstart.plane.departureAirport = msg.plane.departureAirport;
    boardingstart.plane.arrivalAirport = msg.plane.arrivalAirport;
    boardingstart.plane.totalPlaneWeight = msg.plane.totalPlaneWeight;
    boardingstart.notification.kill_status = 0;      
    boardingstart.notification.completionStatus = 1;

    // Sending the p_message to airport.c
    if (msgsnd(msg_queue_id, &boardingstart, sizeof(struct msg_buffer), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Forwarded a_message to airport.c with pl_ID: %d, dep_ID: %d, and arr_ID: %d\n", msg.plane.planeID, msg.plane.departureAirport, msg.plane.arrivalAirport);
}

// Function to process p_message from airports
// Now there are two cases when atc receives a message from plane : one when boarding complete and one when deboarding complete
void process_pmsg(struct msg_buffer msg, int msg_queue_id) {
    
    // Deboarding complete case: We need to let the plane know that it needs to shut down, for this airport.c needs to send dest_ID as 0 because plane would go nowhere next)
    // Check if the received message has attributes { dest_ID = 0, status = 1, type = 1, action = 1 (arrival plane finished deboarding)}
    if (msg.notification.completionStatus == 4) {
        // Prepare the new a_message
        /*a_msg.msg_type = msg.pl_ID;
        a_msg.pl_ID = msg.pl_ID;
        a_msg.dep_ID = 0;
        a_msg.arr_ID = msg.a_ID;
        a_msg.tot_weight=msg.tot_weight;
        a_msg.plane_type=msg.plane_type;
        a_msg.passenger_count=msg.passenger_count;
        a_msg.kill = 1;*/

        struct msg_buffer deboardingcomplete;

        deboardingcomplete.msg_type = msg.plane.planeID;
        deboardingcomplete.plane.planeID = msg.plane.planeID;
        deboardingcomplete.plane.planeType = msg.plane.planeType;
        deboardingcomplete.plane.numOccupiedSeats = msg.plane.numOccupiedSeats;   
        deboardingcomplete.plane.totalLuggageWeight = msg.plane.totalLuggageWeight; 
        deboardingcomplete.plane.totalPassengerWeight = msg.plane.totalPassengerWeight;
        deboardingcomplete.plane.totalCrewWeight = msg.plane.totalCrewWeight;
        deboardingcomplete.plane.departureAirport = msg.plane.departureAirport;
        deboardingcomplete.plane.arrivalAirport = msg.plane.arrivalAirport;
        deboardingcomplete.plane.totalPlaneWeight = msg.plane.totalPlaneWeight;
        deboardingcomplete.notification.kill_status = 1;      
        deboardingcomplete.notification.completionStatus = 4;

        // Sending the new a_message to respective plane
        if (msgsnd(msg_queue_id, &deboardingcomplete, sizeof(struct msg_buffer), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

active_planes--;   //signifying this particular plane that the message has been sent to, is no longer active

        printf("Sent a_message with pl_ID: %d and kill: 1\n", msg.plane.planeID);
    }

    // Boarding complete case: We need to let the arrival airport know that flight is about to land, 
    // Check if the received message has attributes { dest_ID = 0, status = 1, type = 0, action = 0, kill = 0}
    if (msg.notification.completionStatus == 2) {
        // Prepare and send a new p_message
        /*struct p_message new_p_msg;
        new_p_msg.msg_type = msg.dest_ID;
        new_p_msg.pl_ID = msg.pl_ID;
        new_p_msg.a_ID = msg.dest_ID;
        new_p_msg.dest_ID = 0;
        new_p_msg.type = 1;
        new_p_msg.tot_weight=msg.tot_weight;
        new_p_msg.plane_type=msg.plane_type;
        new_p_msg.passenger_count=msg.passenger_count;
        new_p_msg.status = 0;
        new_p_msg.action = 1; 
        new_p_msg.kill = 0;*/

        struct msg_buffer boardingcomplete;

        boardingcomplete.msg_type = msg.plane.arrivalAirport+10;
        boardingcomplete.plane.planeID = msg.plane.planeID;
        boardingcomplete.plane.planeType = msg.plane.planeType;
        boardingcomplete.plane.numOccupiedSeats = msg.plane.numOccupiedSeats;   
        boardingcomplete.plane.totalLuggageWeight = msg.plane.totalLuggageWeight; 
        boardingcomplete.plane.totalPassengerWeight = msg.plane.totalPassengerWeight;
        boardingcomplete.plane.totalCrewWeight = msg.plane.totalCrewWeight;
        boardingcomplete.plane.departureAirport = msg.plane.departureAirport;
        boardingcomplete.plane.arrivalAirport = msg.plane.arrivalAirport;
        boardingcomplete.plane.totalPlaneWeight = msg.plane.totalPlaneWeight;
        boardingcomplete.notification.kill_status = 0;      
        boardingcomplete.notification.completionStatus = 2;

        // Sending the new p_message
        if (msgsnd(msg_queue_id, &boardingcomplete, sizeof(struct msg_buffer), 0) == -1) {
            perror("msgsnd");
            exit(1);
        }

        FILE *file = fopen("AirTrafficController.txt", "a");
        if (file != NULL) {
        fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", msg.plane.planeID, msg.plane.departureAirport, msg.plane.arrivalAirport);
        fclose(file);
    } else {
        printf("Error opening file!\n");
    }
         printf("Sent boarding message to %d\n", msg.plane.arrivalAirport);
        //printf("Sent p_message with a_ID: %d, dest_ID: 0, type: 1, status: 0, and action: 1\n", msg.plane.arrivalAirport+10);
    }
}

// Function to process termination message
void process_tmsg(int active_planes, int num_airports, int msg_queue_id) {
    // Process termination message here
    printf("Received termination message. Number of active planes: %d.\n", active_planes);
    
    if (active_planes == 0) {
        // Send p_message for all airports
        for (int airport_ID = 1; airport_ID <= num_airports; airport_ID++) {
            struct msg_buffer killairport;
            /*p_msg.msg_type = airport_ID+10;
            p_msg.pl_ID = 0; // Set pl_ID to 0
            p_msg.a_ID = airport_ID;
            p_msg.dest_ID = 0;
            p_msg.tot_weight=0;
            p_msg.plane_type=0;
            p_msg.passenger_count=0;
            p_msg.status = 0;
            p_msg.type = 0;
            p_msg.action = 0;
            p_msg.kill = 1;*/

            killairport.msg_type = airport_ID+10;
            killairport.plane.planeID = 0;
            killairport.plane.planeType = 0;
            killairport.plane.numOccupiedSeats = 0;   
            killairport.plane.totalLuggageWeight = 0; 
            killairport.plane.totalPassengerWeight = 0;
            killairport.plane.totalCrewWeight = 0;
            killairport.plane.departureAirport = 0;
            killairport.plane.arrivalAirport = 0;
            killairport.plane.totalPlaneWeight = 0;
            killairport.notification.kill_status = 1;      
            killairport.notification.completionStatus = 0;

            // Sending the p_message
            if (msgsnd(msg_queue_id, &killairport, sizeof(struct msg_buffer), 0) == -1) {
                perror("msgsnd");
                exit(1);
            }

            printf("Sent p_message with pl_ID: 0, a_ID: %d, kill: 1\n", airport_ID);
        }
    }
}

// Function to process a_messages after termination
void process_aft_termination_req(int plane_ID, int msg_queue_id) {
    struct msg_buffer killplane;

    // Prepare the new a_message
    /*a_msg.msg_type = A_MSG_TYPE;
    a_msg.pl_ID = pl_ID;
    a_msg.dep_ID = 0;
    a_msg.arr_ID = 0;
    a_msg.tot_weight=0;
    a_msg.plane_type=0;
    a_msg.passenger_count=0;
    a_msg.kill = 2;*/

            killplane.msg_type=plane_ID;
            killplane.plane.planeID = plane_ID;
            killplane.plane.planeType = 0;
            killplane.plane.numOccupiedSeats = 0;   
            killplane.plane.totalLuggageWeight = 0; 
            killplane.plane.totalPassengerWeight = 0;
            killplane.plane.totalCrewWeight = 0;
            killplane.plane.departureAirport = 0;
            killplane.plane.arrivalAirport = 0;
            killplane.plane.totalPlaneWeight = 0;
            killplane.notification.kill_status = 2;      
            killplane.notification.completionStatus = 0;

    // Sending the new a_message
    if (msgsnd(msg_queue_id, &killplane, sizeof(struct msg_buffer), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }

    printf("Processed a_message after termination with pl_ID: %d and kill: 2\n", plane_ID);
}

