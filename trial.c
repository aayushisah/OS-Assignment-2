#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>

#define MAX_RUNWAYS 10
#define MAX_LOADCAPACITY 12000
#define BACKUP_LOADCAPACITY 15000
#define BOARDING_TIME 3
#define LANDING_TIME 2
#define UNLOADING_TIME 3

struct PlaneInfo {
    int planeID;
    int totalWeight;
    int departureAirport;
    int arrivalAirport;
    bool isDeparture;
};

struct Runway {
    int loadCapacity;
    bool isAvailable;
    pthread_mutex_t mutex;
};

struct Message {
    long msg_type;
    struct PlaneInfo plane;
};

struct AirportThreadArgs {
    struct PlaneInfo plane;
    struct Runway *runways;
    int numRunways;
};

void *departure_handler(void *args) {
    struct AirportThreadArgs *threadArgs = (struct AirportThreadArgs *)args;
    struct PlaneInfo plane = threadArgs->plane;
    struct Runway *runways = threadArgs->runways;
    int numRunways = threadArgs->numRunways;

    // Find the best-fit runway for departure
    int bestFitIndex = -1;
    for (int i = 0; i < numRunways; i++) {
        if (runways[i].isAvailable && runways[i].loadCapacity >= plane.totalWeight) {
            if (bestFitIndex == -1 || runways[i].loadCapacity < runways[bestFitIndex].loadCapacity) {
                bestFitIndex = i;
            }
        }
    }

    // If no suitable runway found, use backup runway
    if (bestFitIndex == -1) {
        for (int i = 0; i < numRunways; i++) {
            if (runways[i].isAvailable && runways[i].loadCapacity >= BACKUP_LOADCAPACITY) {
                bestFitIndex = i;
                break;
            }
        }
    }

    pthread_mutex_lock(&runways[bestFitIndex].mutex);
    runways[bestFitIndex].isAvailable = false;
    pthread_mutex_unlock(&runways[bestFitIndex].mutex);

    printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n", plane.planeID, bestFitIndex + 1, plane.departureAirport);

    sleep(BOARDING_TIME + 2); // Simulate takeoff

    printf("Plane %d has successfully completed takeoff.\n", plane.planeID);

    pthread_mutex_lock(&runways[bestFitIndex].mutex);
    runways[bestFitIndex].isAvailable = true;
    pthread_mutex_unlock(&runways[bestFitIndex].mutex);

    free(threadArgs);
    return NULL;
}

void *arrival_handler(void *args) {
    struct AirportThreadArgs *threadArgs = (struct AirportThreadArgs *)args;
    struct PlaneInfo plane = threadArgs->plane;
    struct Runway *runways = threadArgs->runways;
    int numRunways = threadArgs->numRunways;

    // Find the best-fit runway for arrival
    int bestFitIndex = -1;
    for (int i = 0; i < numRunways; i++) {
        if (runways[i].isAvailable && runways[i].loadCapacity >= plane.totalWeight) {
            if (bestFitIndex == -1 || runways[i].loadCapacity < runways[bestFitIndex].loadCapacity) {
                bestFitIndex = i;
            }
        }
    }

    // If no suitable runway found, use backup runway
    if (bestFitIndex == -1) {
        for (int i = 0; i < numRunways; i++) {
            if (runways[i].isAvailable && runways[i].loadCapacity >= BACKUP_LOADCAPACITY) {
                bestFitIndex = i;
                break;
            }
        }
    }

    pthread_mutex_lock(&runways[bestFitIndex].mutex);
    runways[bestFitIndex].isAvailable = false;
    pthread_mutex_unlock(&runways[bestFitIndex].mutex);

    printf("Plane %d has landed on Runway No. %d of Airport No. %d.\n", plane.planeID, bestFitIndex + 1, plane.arrivalAirport);
    sleep(LANDING_TIME);

    printf("Plane %d has completed deboarding/unloading.\n", plane.planeID);

    pthread_mutex_lock(&runways[bestFitIndex].mutex);
    runways[bestFitIndex].isAvailable = true;
    pthread_mutex_unlock(&runways[bestFitIndex].mutex);

    free(threadArgs);
    return NULL;
}

int main() {
    int airportNumber;
    int numRunways;

    printf("Enter Airport Number: ");
    scanf("%d", &airportNumber);

    printf("Enter number of Runways: ");
    scanf("%d", &numRunways);

    if (numRunways % 2 != 0 || numRunways < 1 || numRunways > MAX_RUNWAYS) {
        fprintf(stderr, "Invalid number of runways. Please enter an even number between 2 and 10.\n");
        return 1;
    }

    // Create an array of runways
    struct Runway runways[numRunways + 1]; // +1 for backup runway
    for (int i = 0; i < numRunways; i++) {
        printf("Enter loadCapacity of Runway %d: ", i + 1);
        scanf("%d", &runways[i].loadCapacity);
        runways[i].isAvailable = true;
        pthread_mutex_init(&runways[i].mutex, NULL);
    }

    // Additional backup runway
    runways[numRunways].loadCapacity = BACKUP_LOADCAPACITY;
    runways[numRunways].isAvailable = true;
    pthread_mutex_init(&runways[numRunways].mutex, NULL);

    // Create message queue
    key_t key = ftok("airport.c", airportNumber);
    int msgid = msgget(key, IPC_CREAT | 0666);

    // Listen for incoming plane messages
    struct Message message;
    while (msgrcv(msgid, &message, sizeof(message), 1, 0) != -1) {
        struct AirportThreadArgs *threadArgs = (struct AirportThreadArgs *)malloc(sizeof(struct AirportThreadArgs));

        threadArgs->plane = message.plane;
        threadArgs->runways = runways;
        threadArgs->numRunways = numRunways + 1; // Include backup runway

        pthread_t thread;
        if (message.plane.isDeparture) {
            pthread_create(&thread, NULL, departure_handler, (void *)threadArgs);
        } else {
            pthread_create(&thread, NULL, arrival_handler, (void *)threadArgs);
        }
    }

    // Cleanup
    msgctl(msgid, IPC_RMID, NULL);
    for (int i = 0; i < numRunways + 1; i++) {
        pthread_mutex_destroy(&runways[i].mutex);
    }

    return 0;
}
