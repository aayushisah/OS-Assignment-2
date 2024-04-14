#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_PASSENGERS 10

struct PassengerInfo
{
    int luggageWeight;
    int bodyWeight;
};

int main()
{
    int planeID, planeType, numOccupiedSeats;

    printf("Enter Plane ID: ");
    scanf("%d", &planeID);

    // Validate the plane ID
    if (planeID < 1 || planeID > 10)
    {
        fprintf(stderr, "Invalid plane ID. Please enter a valid ID between 1 and 10.\n");
        return 1;
    }
    // Request the type of plane from the user
    printf("Enter Type of Plane: ");
    scanf("%d", &planeType);
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

        // Create an array to store passenger information
        struct PassengerInfo passengerInfo[MAX_PASSENGERS];

        // Create child processes and pipes for each passenger
        int passengers = numOccupiedSeats;
        int passengerCount;
        pid_t passengerPIDs[MAX_PASSENGERS];
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

                // Add logic for passenger process here
                printf("Passenger %d process with PID %d is ready\n", passengerCount + 1, getpid());
                exit(0);
            }
        }

        // Access and utilize passenger information in the plane process
        for (int i = 0; i < passengers; i++)
        {
            struct PassengerInfo receivedInfo;
            close(passengerPipes[i][1]);
            read(passengerPipes[i][0], &receivedInfo, sizeof(struct PassengerInfo));
            close(passengerPipes[i][0]);
            totalLuggageWeight += receivedInfo.luggageWeight;
            totalPassengerWeight += receivedInfo.bodyWeight;
            printf("Passenger %d - Luggage Weight: %d, Body Weight: %d\n", i + 1, receivedInfo.luggageWeight, receivedInfo.bodyWeight);
        }

        // Calculate the total crew weight
        int totalCrewMembers = 7; // 2 pilots + 5 flight attendants
        int averageCrewWeight = 75;
        totalCrewWeight = totalCrewMembers * averageCrewWeight;

        // Calculate the total weight of the plane
        int totalPlaneWeight = totalLuggageWeight + totalPassengerWeight + totalCrewWeight;
    }
    else
    {
        // printf("This is a cargo plane.\n");
    }

    return 0;
}
