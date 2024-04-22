#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> // is this needed
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PASSENGERS 10
#define MAX_CARGO_ITEMS 100
#define MAX_CREW_MEMBERS 7
#define MAX_CREW_CARGO 2
#define MAX_WEIGHT_LUGGAGE 25
#define MAX_WEIGHT_PASSENGER 100
#define AVG_WEIGHT_CREW 75
#define BOARDING_TIME 3
#define DEBOARDING_TIME 3
#define JOURNEY_TIME 30
#define MSG_QUEUE_KEY 'atc_message_queue'

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
    int completionStatus; // This field can indicate the status of the deboarding/unloading process
};

struct msg_buffer
{
    long msg_type;                           // the type of message
    struct PlaneInfo plane;                  // the plane
    struct NotificationMessage notification; // the notification
};

struct PassengerInfo
{
    int luggageWeight;
    int bodyWeight;
};

int main()
{
    // int planes[MAX_PASSENGERS][6]; // 2D array to store plane information (shrishti:not needed)

    // generate a mapping of which index of plane array stores wht info

    // 0 - planeID
    // 1 - planeType

    // 2 - arrivalAirport
    // 3 - departureAirport

    // 4 - totalPlaneWeight
    // 5 number of passengers (only for passenger planes)

    int planeID;
    int planeType;
    int numOccupiedSeats;
    int totalLuggageWeight = 0;
    int totalPassengerWeight = 0;
    int totalCrewWeight = 0;
    int departureAirport;
    int arrivalAirport;
    int totalPlaneWeight;

    key_t key;
    int msgid;
    struct msg_buffer message;

    printf("Enter Plane ID: ");
    scanf("%d", &planeID);

    // ftok to generate unique key,planeID as unique key
    key = ftok("plane.c", 'x'); // change path to atc.c when that file is made for uniformity.
    if (key == -1)
    {
        printf("error in creating unique key\n");
        exit(1);
    }

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        printf("error in creating message queue\n");
        exit(1);
    }

    // Validate the plane ID
    if (planeID < 1 || planeID > 10)
    {
        fprintf(stderr, "Invalid plane ID. Please enter a valid ID between 1 and 10.\n");
        return 1;
    }
    // Request the type of plane from the user
    printf("Enter Type of Plane: ");
    scanf("%d", &planeType);

    // planes[planeID - 1][0] = planeID;  not needed
    // planes[planeID - 1][1] = planeType;  mot needed

    if (planeType == 1)
    {
        // printf("This is a passenger plane.\n");
        //  Request the number of occupied seats for the passenger plane
        printf("Enter Number of Occupied Seats : ");
        scanf("%d", &numOccupiedSeats);

        // Validate the number of occupied seats //idt we need still
        if (numOccupiedSeats < 1 || numOccupiedSeats > 10)
        {
            fprintf(stderr, "Invalid number of occupied seats. Please enter a value between 1 and 10.\n");
            return 1;
        }

        // Store the number of occupied seats in the planes array
        // planes[planeID - 1][5] = numOccupiedSeats; // 5 is the index for number of passengers

        // Create an array to store passenger information
        struct PassengerInfo passengerInfo[MAX_PASSENGERS];

        // Create child processes and pipes for each passenger
        int passengers = numOccupiedSeats;
        int passengerCount;
        pid_t passengerPIDs[MAX_PASSENGERS]; // Array to store passenger PIDs
        int passengerPipes[MAX_PASSENGERS][2];

        for (passengerCount = 0; passengerCount < passengers; passengerCount++)
        {
            if (pipe(passengerPipes[passengerCount]) < 0)
            {
                fprintf(stderr, "Error creating pipe for passenger %d\n", passengerCount + 1);
                return 1;
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                // Fork failed
                fprintf(stderr, "Fork failed for passenger %d\n", passengerCount + 1);
                return 1;
            }
            else if (pid == 0)
            {
                // Child process (passenger)
                printf("Enter Weight of Your Luggage: ");
                scanf("%d", &passengerInfo[passengerCount].luggageWeight);

                // Validate the luggage weight
                if (passengerInfo[passengerCount].luggageWeight < 0 || passengerInfo[passengerCount].luggageWeight > 25)
                {
                    fprintf(stderr, "Invalid luggage weight. Please enter a value between 0 and 25.\n");
                    return 1;
                }

                printf("Enter Your Body Weight: ");
                scanf("%d", &passengerInfo[passengerCount].bodyWeight);

                // Validate the body weight
                if (passengerInfo[passengerCount].bodyWeight < 10 || passengerInfo[passengerCount].bodyWeight > 100)
                {
                    fprintf(stderr, "Invalid body weight. Please enter a value between 10 and 100.\n");
                    return 1;
                }

                // Communicate the luggage weight and body weight to the plane process through the pipe
                close(passengerPipes[passengerCount][0]); // Close the read end of the pipe
                write(passengerPipes[passengerCount][1], &passengerInfo[passengerCount], sizeof(struct PassengerInfo));
                close(passengerPipes[passengerCount][1]); // Close the write end of the pipe

                // for checking concurrency
                printf("Passenger %d process with PID %d is ready\n", passengerCount + 1, getpid());
                exit(0);
            }
            wait(NULL);
            //  else
            //  {
            //      // Parent process
            //      close(passengerPipes[passengerCount][1]); // Close the write end of the pipe
            //  }
        }

        // Access and utilize passenger information in the plane process
        for (int i = 0; i < passengers; i++)
        {
            struct PassengerInfo receivedInfo;
            read(passengerPipes[i][0], &receivedInfo, sizeof(struct PassengerInfo));
            close(passengerPipes[i][0]);
            totalLuggageWeight += receivedInfo.luggageWeight;
            totalPassengerWeight += receivedInfo.bodyWeight;
            // printf("Passenger %d - Luggage Weight: %d, Body Weight: %d\n", i + 1, receivedInfo.luggageWeight, receivedInfo.bodyWeight);
        }

        // Calculate the total crew weight
        int totalCrewMembers = 7; // 2 pilots + 5 flight attendants
        int averageCrewWeight = 75;
        totalCrewWeight = totalCrewMembers * averageCrewWeight;

        // Calculate the total weight of the plane
        int totalPlaneWeight = totalLuggageWeight + totalPassengerWeight + totalCrewWeight;
        // planes[planeID - 1][4] = totalPlaneWeight; not needed
    }
    else
    {
        // Cargo plane
        printf("Enter Number of Cargo Items: ");
        scanf("%d", &numOccupiedSeats); // using the same variable as for passengers

        if (numOccupiedSeats < 1 || numOccupiedSeats > 100)
        {
            fprintf(stderr, "Invalid number of cargo items. Please enter a value between 1 and 100.\n");
            return 1;
        }

        printf("Enter Average Weight of Cargo Items: ");
        scanf("%d", &totalPassengerWeight); // this is actually the average weight
        // using the same variable as passengers

        if (totalPassengerWeight < 1 || totalPassengerWeight > 100)
        {
            fprintf(stderr, "Invalid average weight of cargo items. Please enter a value between 1 and 100.\n");
            return 1;
        }
        totalPassengerWeight = numOccupiedSeats * totalPassengerWeight;
        // Calculate total crew weight
        int totalCrewMembers = 2; // 2 pilots for cargo plane
        int averageCrewWeight = 75;
        totalCrewWeight = totalCrewMembers * averageCrewWeight;

        // Calculate total plane weight
        totalPlaneWeight = totalPassengerWeight + totalCrewWeight;

        // planes[planeID - 1][4] = totalPlaneWeight; not needed
    }
    printf("Enter Departure Airport: ");
    scanf("%d", &departureAirport);

    // planes[planeID - 1][3] = departureAirport; not needed

    printf("Enter Arrival Airport: ");
    scanf("%d", &arrivalAirport);

    // planes[planeID - 1][2] = arrivalAirport; not needed

    // Send plane information through message queue
    message.msg_type = arrivalAirport+10;
    message.plane.planeID = planeID;
    message.plane.planeType = planeType;
    message.plane.numOccupiedSeats = numOccupiedSeats;
    message.plane.totalLuggageWeight = totalLuggageWeight;
    message.plane.totalPassengerWeight = totalPassengerWeight;
    message.plane.totalCrewWeight = totalCrewWeight;
    message.plane.departureAirport = departureAirport;
    message.plane.arrivalAirport = arrivalAirport;
    message.plane.totalPlaneWeight = totalPlaneWeight;
    message.notification.completionStatus = 0;
    // msgsnd to send message
    if (msgsnd(msgid, &message, sizeof(message.plane), 0) == -1)
    {
        printf("error in sending message\n");
        exit(1);
    }
    // Simulate the boarding/loading process
    sleep(BOARDING_TIME); // Boarding/loading process
    // Simulate the plane journey duration
    sleep(JOURNEY_TIME); // Journey duration
    // Receive notification from the plane process
    sleep(DEBOARDING_TIME); // deboarding/unloading process
    msgrcv(msgid, &message, sizeof(message.notification), 1, 0);

    /*Once the plane arrives at the arrival airport and the deboarding/unloading process is completed, the air  traffic controller process (after receiving a confirmation from the arrival airport) informs the plane  process that the deboarding/unloading is completed via the single message queue of 2.(c). For a cargo  plane, deboarding implies unloading the cargo. Upon receiving this intimation, the plane process  displays the following message before terminating itself.
     */
    if (message.notification.completionStatus == 1)
        printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n", planeID, departureAirport, arrivalAirport);

    // Destroy the message queue
    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}