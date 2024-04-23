#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <semaphore.h>

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
    int flag;
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
    sem_t semaphore;
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

struct RunwayArgs
{
    int planeID, selectedRunway, airportNumber;
};

void *handleDeparture(void *arg)
{
    struct DepartureArgs *departure = (struct DepartureArgs *)arg;
    int selectedRunway = 0;
    int planeID = departure->message.plane.planeID;
    int airportNumber = departure->message.plane.departureAirport;
    printf("Total weight of the plane is: %d", departure->message.plane.totalPlaneWeight);

    for (int i = 0; i <= departure->numRunways; i++)
    {
        
        if (departure->message.plane.totalPlaneWeight <= departure->runways[i].loadCapacity)
        {
            if (departure->runways[i].loadCapacity < departure->runways[selectedRunway].loadCapacity)
            {
                selectedRunway = i;

            }
        }
    }
    printf("Best runway for planeID %d is %d\n", planeID, selectedRunway);

    sem_wait(&departure->runways[selectedRunway].semaphore);
    printf("Waiting state...\n");
    sleep(3); // using the runway
    sem_post(&departure->runways[selectedRunway].semaphore);
    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", planeID, selectedRunway, airportNumber);
    // simulate takeoff
    sleep(2);


    return NULL;
}

void *handleArrival(void *arg)
{
    printf("arrival airport is handling arrival\n");
    struct ArrivalArgs *arrival = (struct ArrivalArgs *)arg;
    int selectedRunway = 0;
    int planeID = arrival->message.plane.planeID;
    int airportNumber = arrival->message.plane.arrivalAirport;

    // finding best fit runway
    for (int i = 0; i <= arrival->numRunways; i++)
    {
        if (arrival->message.plane.totalPlaneWeight <= arrival->runways[i].loadCapacity)
        {
            if (arrival->runways[i].loadCapacity < arrival->runways[selectedRunway].loadCapacity)
            {
                selectedRunway = i;
            
            }
        }
    }
    printf("best fit is runway %d\n", selectedRunway);
    sem_wait(&arrival->runways[selectedRunway].semaphore);
    sleep(5); // plane in-flight
    sleep(3); // using the runway
    sem_post(&arrival->runways[selectedRunway].semaphore);

    printf("Plane %d has landed on Runway No. %d of Airport No. %d  and has completed deboarding/unloading.\n", planeID, selectedRunway, airportNumber);

    return NULL;
}

void* handlePlaneDep(void *arg){
    struct RunwayArgs *planeArg = (struct RunwayArgs*) arg;
    sleep(3); // using the runway
    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", planeArg->planeID, planeArg->selectedRunway, planeArg->airportNumber);
    // simulate takeoff
    sleep(2);
}

void* handleRunwayD(void* arg){
    pthread_t plane;
    struct RunwayArgs *planeArg = (struct RunwayArgs*) arg;
    pthread_create(&plane, NULL, handlePlaneDep, (void*) planeArg);
    pthread_join(plane, NULL);
}

void* handlePlaneArr(void *arg){
    struct RunwayArgs *planeArg = (struct RunwayArgs*) arg;
    sleep(5); // plane in-flight
    sleep(3); // using the runway

    printf("Plane %d has landed on Runway No. %d of Airport No. %d  and has completed deboarding/unloading.\n", planeArg->planeID, planeArg->selectedRunway, planeArg->airportNumber);
}

void* handleRunwayA(void* arg){
    pthread_t plane;
    struct RunwayArgs *planeArg = (struct RunwayArgs*) arg;
    pthread_create(&plane, NULL, handlePlaneArr, (void*) arg);
    pthread_join(plane, NULL);
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

    pthread_t runwayThreads[runwaysCount+1];

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
    for (int i = 0; i < runwaysCount + 1; i++)
    {
        if (i != runwaysCount)
        {
            if (temp[i] < 1000 || temp[i] > 12000)
            {
                printf("Invalid load capacity. Please enter a valid capacity between 1000 and 12000.\n");
                return 1;
            }
            runways[i + 1].loadCapacity = temp[i];
            runways[i + 1].isAvailable = 1;
        }
        if (sem_init(&runways[i].semaphore, 0, 1) != 0)
        {
            printf("Error in initializing semaphore for runway %d\n", i);
            return 1;
        }
        printf("runway %d has limit %d\n", i, runways[i].loadCapacity);
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
        while(msgrcv(msgid, &message, sizeof(message), airportNumber + 10, 0) == -1)
        {
        }
 printf("msg received with type: %d and status: %d\n", message.msg_type, message.notification.completionStatus);
        // if (msgrcv(msgid, &message, sizeof(message), airportNumber+10, 0) == -1)
        // {
        //     printf("Error in receiving message\n");
        //     exit(1);
        // }
       
        printf("plane is saying depart from %d & arrive to %d\n", message.plane.departureAirport, message.plane.arrivalAirport);
        // Plane wants to depart from this airport
        if (message.notification.flag==3 && message.notification.completionStatus == 1)
        {
            printf("Inside if block\n");
            struct DepartureArgs departure;
            departure.message = message;
            departure.runways = runways;
            departure.numRunways = runwaysCount;

            int selectedFinalRunway = 0;
            for(int i = 0; i<=runwaysCount; i++){
                if (departure.message.plane.totalPlaneWeight <= departure.runways[i].loadCapacity)
                {
                    if (departure.runways[i].loadCapacity < departure.runways[selectedFinalRunway].loadCapacity)
                    {
                        selectedFinalRunway = i;
                    }
                }
            }
            printf("selected runway = %d\n", selectedFinalRunway);

            struct RunwayArgs runwayArgs;
            runwayArgs.airportNumber = airportNumber;
            runwayArgs.planeID = message.plane.planeID;
            runwayArgs.selectedRunway = selectedFinalRunway;

            sem_wait(&runways[selectedFinalRunway].semaphore);
            printf("Creating thread now...\n");
            pthread_create(&runwayThreads[selectedFinalRunway], NULL, handleRunwayD, (void*)&runwayArgs);
            pthread_join(runwayThreads[selectedFinalRunway], NULL);
            printf("Thread completed\n");
            sem_post(&runways[selectedFinalRunway].semaphore);

            // pthread_t departure_thread;
            // if (pthread_create(&departure_thread, NULL, handleDeparture, (void *)&departure) != 0)
            // {
            //     printf("Error in creating thread for plane %d\n", message.plane.planeID);
            //     return 1;
            // }

            
            // send arrival message to arrival airport
         //   message.msg_type = airportNumber + 20;
            message.msg_type = 100;
            message.notification.flag = 1;
            message.notification.completionStatus = 2;

            if (msgsnd(msgid, &message, sizeof(message), 0) == -1)
            {
                printf("error in sending arrival message\n");
                exit(1);
            }
            printf("arrival inbound message sent to ATC\n");
            
           
        }
        // Plane wants to arrive to this airport
        else if (message.notification.flag==4 && message.notification.completionStatus == 2)
        {
            printf("ATC told me flight inbound\n");
            struct ArrivalArgs arrival;
            arrival.message = message;
            arrival.runways = runways;
            arrival.numRunways = runwaysCount;

            int selectedFinalRunway = 0;
            for(int i = 0; i<=runwaysCount; i++){
                if (arrival.message.plane.totalPlaneWeight <= arrival.runways[i].loadCapacity)
                {
                    if (arrival.runways[i].loadCapacity < arrival.runways[selectedFinalRunway].loadCapacity)
                    {
                        selectedFinalRunway = i;
                    }
                }
            }

            printf("selected runway arrival= %d\n", selectedFinalRunway);

            struct RunwayArgs runwayArgs;
            runwayArgs.airportNumber = airportNumber;
            runwayArgs.planeID = message.plane.planeID;
            runwayArgs.selectedRunway = selectedFinalRunway;

            sem_wait(&runways[selectedFinalRunway].semaphore);
            printf("Creating thread now arrival...\n");
            pthread_create(&runwayThreads[selectedFinalRunway], NULL, handleRunwayA, (void*)&runwayArgs);
            pthread_join(runwayThreads[selectedFinalRunway], NULL);
            printf("Thread completed arrival\n");
            sem_post(&runways[selectedFinalRunway].semaphore);

            // pthread_t arrival_thread;
            // if (pthread_create(&arrival_thread, NULL, handleArrival, (void *)&arrival) != 0)
            // {
            //     printf("Error in creating thread for plane %d\n", message.plane.planeID);
            //     return 1;
            // }

            // pthread_join(arrival_thread, NULL);
            // send arrival message to ATC
            message.msg_type = airportNumber + 20;
            message.notification.completionStatus = 4;
            if (msgsnd(msgid, &message, sizeof(message), 0) == -1)
            {
                printf("error in sending arrival complete message\n");
                exit(1);
            }
        }

        else
        {
            printf("This message is prolly not for this airport.\n");
        }

        // check if airport needs to close
    }

    return 0;
}
