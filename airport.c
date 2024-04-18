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

struct DepartureArgs{
    struct msg_buffer message;
    int* runways;
    int numRunways;
};

int handleDeparture(struct DepartureArgs departure){
    int selectedRunway = 0;
    int runwaysCount = departure.numRunways;
    int* runways = departure.runways;
    int planeID = departure.message.plane.planeID;
    int airportNumber = departure.message.plane.departureAirport;

    //mutex lock here

    //find best fit runway
    for(int i = 1; i <= runwaysCount; i++){
        if(departure.message.plane.totalPlaneWeight <= runways[i]){
            if(runways[i] < runways[selectedRunway]){
                selectedRunway = i;
            }
        }
    }

    //boarding/loading process sleep 3 seconds
    sleep(3);

    //mutex released here

    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", planeID, selectedRunway, airportNumber);

    // void* point = &selectedRunway;
    pthread_exit(NULL);
}

int main(){
    int airtportNumber;
    int runwaysCount;


    key_t key;
    int msgid;
    struct msg_buffer message;

    printf("Enter Airport Number: ");
    scanf("%d", &airtportNumber);

    //validate the airport number
    if(airtportNumber < 1 || airtportNumber > 10){
        fprintf(stderr, "Invalid airport number. Please enter a valid airport number between 1 and 10.\n");
        return 1;
    }

    //input number of runways
    printf("Enter number of Runways: ");
    scanf("%d", &runwaysCount);

    if(runwaysCount < 1 || runwaysCount > 10){
        fprintf(stderr, "Invalid airport number. Please enter a valid airport number between 1 and 10.\n");
        return 1;
    }

    //array to hold the capacity of each runway
    int runways[runwaysCount+1];

    //backup runway always at index 0;
    runways[0] = 15000;

    //initializing rest of the runways
    printf("Enter  loadCapacity  of  Runways  (give as a space separated list in a single line): ");
    for (int i = 1; i <= runwaysCount; i++) {
        int temp;
        scanf("%d", &temp);
        
        if(temp < 1000 || temp > 12000){
        fprintf(stderr, "Invalid load capacity. Please enter a valid capacity between 1000 and 12000.\n");
        return 1;
        }
        runways[i] = temp;
    }

    //logic to receive message from ATC
     key = ftok("plane.c", 'x');    //change path to atc.c when that file is made for uniformity.
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
    // receives the message from the message queue with the flight details
    if (msgrcv(msgid, &message, sizeof(message.plane), 1, 0) == -1)
    {
        printf("error in receiving message\n");
        exit(1);
    }

    // if this airport is the departing airport
    if(message.plane.departureAirport == airtportNumber){
        pthread_t departure_thread;

        //initalizing the depatrure arguments structure
        struct DepartureArgs departure;
        departure.message = message;
        departure.numRunways = runwaysCount;
        departure.runways = runways;

        //return value for handleDeparture function
        

        if(pthread_create(&departure_thread, NULL, (void*)&handleDeparture, (void*)&departure) != 0){
            printf("Error in creating threads\n");
            return -1;
        }

        //void* selectedRunway;
        pthread_join(departure_thread, NULL);
        // int departureRunway = *((int*)selectedRunway);
    }

   // logic for arrival airport here
}