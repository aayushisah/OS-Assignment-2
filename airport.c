// updated one
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>

#define MSG_QUEUE_KEY 'k'

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
    int kill_status;
    int completionStatus; // This field can indicate the status of the deboarding/unloading process
};

struct msg_buffer
{
    long msg_type;                           // the type of message
    struct PlaneInfo plane;                  // the plane
    struct NotificationMessage notification; // the notification
};

struct Runway
{
    int id;
    int loadCapacity;
    int isAvailable;
    pthread_mutex_t mutex;
};

struct DepartureArgs
{
    struct msg_buffer message;
    struct Runway *runways;
    int numRunways;
};

struct ArrivalArgs
{
    struct msg_buffer message;
    struct Runway *runways;
    int numRunways;
};

void *handleDeparture(void *arg)
{
    struct DepartureArgs *departure = (struct DepartureArgs *)arg;
    int selectedRunway = 0;
    int planeID = departure->message.plane.planeID;
    int airportNumber = departure->message.plane.departureAirport;

    for (int i = 1; i <= departure->numRunways; i++)
    {
        pthread_mutex_lock(&departure->runways[i].mutex);
        if (departure->message.plane.totalPlaneWeight <= departure->runways[i].loadCapacity && departure->runways[i].isAvailable)
        {
            if (selectedRunway == 0 || departure->runways[i].loadCapacity < departure->runways[selectedRunway].loadCapacity)
            {
                if (selectedRunway != 0)
                {
                    // Mark the previously selected runway as available again
                    departure->runways[selectedRunway].isAvailable = 1;
                }
                selectedRunway = i;
                // Mark the current runway as unavailable
                departure->runways[i].isAvailable = 0;
            }
        }
        pthread_mutex_unlock(&departure->runways[i].mutex);
    }

    // Boarding/loading process
    sleep(3);

    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", planeID, selectedRunway, airportNumber);

    // simulate takeoff
    sleep(2);

    // Reset the availability flag of the selected runway
    pthread_mutex_lock(&departure->runways[selectedRunway].mutex);
    departure->runways[selectedRunway].isAvailable = 1; // Mark the runway as available
    pthread_mutex_unlock(&departure->runways[selectedRunway].mutex);

    return NULL;
}

void *handleArrival(void *arg)
{
    struct ArrivalArgs *arrival = (struct ArrivalArgs *)arg;
    int selectedRunway = 0;
    int planeID = arrival->message.plane.planeID;
    int airportNumber = arrival->message.plane.arrivalAirport;

    for (int i = 1; i <= arrival->numRunways; i++)
    {
        pthread_mutex_lock(&arrival->runways[i].mutex);
        if (arrival->message.plane.totalPlaneWeight <= arrival->runways[i].loadCapacity && arrival->runways[i].isAvailable)
        {
            if (selectedRunway == 0 || arrival->runways[i].loadCapacity < arrival->runways[selectedRunway].loadCapacity)
            {
                if (selectedRunway != 0)
                {
                    // Mark the previously selected runway as available again
                    arrival->runways[selectedRunway].isAvailable = 1;
                }
                selectedRunway = i;
                // Mark the current runway as unavailable
                arrival->runways[i].isAvailable = 0;
            }
        }
        pthread_mutex_unlock(&arrival->runways[i].mutex);
    }

    // Boarding/loading process
    sleep(3);

    printf("Plane %d has landed on Runway No. %d of Airport No. %d  and has completed deboarding/unloading.\n", planeID, selectedRunway, airportNumber);

    // Reset the availability flag of the selected runway
    pthread_mutex_lock(&arrival->runways[selectedRunway].mutex);
    arrival->runways[selectedRunway].isAvailable = 1; // Mark the runway as available
    pthread_mutex_unlock(&arrival->runways[selectedRunway].mutex);

    return NULL;
}

int main()
{
    int airportNumber;
    int runwaysCount;

    key_t key;
    struct msg_buffer message;

    printf("Enter Airport Number: ");
    scanf("%d", &airportNumber);

    // Validate the airport number
    if (airportNumber < 1 || airportNumber > 10)
    {
        // printf("Invalid airport number. Please enter a valid airport number between 1 and 10.\n");
        return 1;
    }

    // Input number of runways
    printf("Enter number of Runways: ");
    scanf("%d", &runwaysCount);

    if (runwaysCount < 1 || runwaysCount > 10)
    {
        printf("Invalid number of runways. Please enter a valid number between 1 and 10.\n");
        return 1;
    }

    // Array to hold the capacity of each runway
    struct Runway runways[runwaysCount + 1];

    // Backup runway always at index 0
    runways[0].loadCapacity = 15000;
    runways[0].isAvailable = 1;
    printf("runway count itna hai:%d\n", runwaysCount);
    // Initializing rest of the runways
    printf("Enter loadCapacity of Runways (give as a space separated list in a single line):");
    int temp[runwaysCount];
    for (int i = 0; i < runwaysCount; i++)
    {
        scanf("%d", &temp[i]);
    }
    for (int i = 0; i < runwaysCount; i++)
    {
        if (temp[i] < 1000 || temp[i] > 12000)
        {
            printf("Invalid load capacity. Please enter a valid capacity between 1000 and 12000.\n");
            return 1;
        }
        runways[i + 1].loadCapacity = temp[i];
        runways[i + 1].isAvailable = 1;
        pthread_mutex_init(&runways[i].mutex, NULL);
        printf("runway %d has limit %d\n", i + 1, runways[i + 1].loadCapacity);
    }

    // Logic to receive message from ATC
    key = ftok("plane.c", 'x'); // Change path to atc.c when that file is made for uniformity.
    if (key == -1)
    {
        printf("Error in creating unique key\n");
        exit(1);
    }

    // msgget creates a message queue and returns identifier
    const int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        printf("Error in creating message queue\n");
        exit(1);
    }

    int airportactive = 1;
    while (airportactive == 1)
    {
        // Receives the message from the message queue with the flight details
        while (msgrcv(msgid, &message, sizeof(message), airportNumber + 10, 0) == -1)
        {
        }
        // if (msgrcv(msgid, &message, sizeof(message), airportNumber+10, 0) == -1)
        // {
        //     printf("Error in receiving message\n");
        //     exit(1);
        // }
        printf("message received\n");
        printf("plane is saying depart from %d & arrive to %d\n", message.plane.departureAirport, message.plane.arrivalAirport);
        // Plane wants to depart from this airport
        if (message.plane.departureAirport == airportNumber && message.notification.completionStatus == 1)
        {
            struct DepartureArgs departure;
            departure.message = message;
            departure.runways = runways;
            departure.numRunways = runwaysCount;

            pthread_t departure_thread;
            if (pthread_create(&departure_thread, NULL, handleDeparture, (void *)&departure) != 0)
            {
                printf("Error in creating thread for plane %d\n", message.plane.planeID);
                return 1;
            }

            pthread_join(departure_thread, NULL);

            // send arrival message to arrival airport
            message.msg_type = airportNumber + 10;
            message.notification.completionStatus = 2;
            if (msgsnd(msgid, &message, sizeof(message), 0) == -1)
            {
                printf("error in sending arrival message\n");
                exit(1);
            }
            printf("arrival inbound message sent to ATC\n");
        }
        // Plane wants to arrive to this airport
        else if (message.plane.arrivalAirport == airportNumber && message.notification.completionStatus == 2)
        {
            struct ArrivalArgs arrival;
            arrival.message = message;
            arrival.runways = runways;
            arrival.numRunways = runwaysCount;

            pthread_t arrival_thread;
            if (pthread_create(&arrival_thread, NULL, handleArrival, (void *)&arrival) != 0)
            {
                printf("Error in creating thread for plane %d\n", message.plane.planeID);
                return 1;
            }

            pthread_join(arrival_thread, NULL);
        }
        else
        {
            printf("This message is prolly not for this airport.\n");
        }

        // check if airport needs to close
    }

    return 0;
}
