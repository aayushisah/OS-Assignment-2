#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int main(){
    int airtportNumber;
    int runwaysCount;

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
    printf("Enter  loadCapacity  of  Runways  (give  as  a  space  separated  list  in  a single line): ");
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
}