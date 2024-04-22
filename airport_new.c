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

    // Reset the availability flag of the selected runway
    pthread_mutex_lock(&departure->runways[selectedRunway].mutex);
    departure->runways[selectedRunway].isAvailable = 1; // Mark the runway as available
    pthread_mutex_unlock(&departure->runways[selectedRunway].mutex);

    return NULL;
}

int main()
{
    int airportNumber;
    int runwaysCount;

    key_t key;
    int msgid;
    struct msg_buffer message;

    printf("Enter Airport Number: ");
    scanf("%d", &airportNumber);

    // Validate the airport number
    if (airportNumber < 1 || airportNumber > 10)
    {
        //printf("Invalid airport number. Please enter a valid airport number between 1 and 10.\n");
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
    printf("runway count itna hai:%d", runwaysCount);
    // Initializing rest of the runways
    printf("Enter loadCapacity of Runways (give as a space separated list in a single line):");
    int temp[runwaysCount];
    for (int i = 0; i < 2; i++) {
        scanf("%d", &temp[i]);
    }
    printf("reached1");
    for (int i = 1; i <= runwaysCount; i++)
    {
        if (temp[i-1] < 1000 || temp[i-1] > 12000)
        {
            printf("Invalid load capacity. Please enter a valid capacity between 1000 and 12000.\n");
            return 1;
        }
        runways[i].loadCapacity = temp[i-1];
        runways[i].isAvailable = 1;
        pthread_mutex_init(&runways[i].mutex, NULL);
        printf("reached");
    }

    // Logic to receive message from ATC
    key = ftok("plane.c", 'x'); // Change path to atc.c when that file is made for uniformity.
    if (key == -1)
    {
        printf("Error in creating unique key\n");
        exit(1);
    }

    // msgget creates a message queue and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        printf("Error in creating message queue\n");
        exit(1);
    }

    // Receives the message from the message queue with the flight details
    if (msgrcv(msgid, &message, sizeof(message.plane), 1, 0) == -1)
    {
        printf("Error in receiving message\n");
        exit(1);
    }

    // If this airport is the departing airport
    if (message.plane.departureAirport == airportNumber)
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
    }

    return 0;
}
