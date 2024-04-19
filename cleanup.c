#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

#define MSG_SIZE 128
// pls once cross verify after they are done with airtraffic controller
struct msg_buffer
{
    long msg_type;
    char msg_text[MSG_SIZE];
};

int main()
{
    key_t key;
    int msg_id;
    struct msg_buffer message;

    key = ftok("plane.c", 'x');
    if (key == -1)
    {
        printf("error in creating unique key\n");
        exit(1);
    }

    msg_id = msgget(key, 0666 | IPC_CREAT); // Create a message queue
    if (msg_id == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("Do you want the Air Traffic Control System to terminate? (Y for Yes and N for No)\n");
        fgets(message.msg_text, MSG_SIZE, stdin);

        // Remove the newline character from the input
        message.msg_text[strcspn(message.msg_text, "\n")] = 0;

        message.msg_type = 3;
        if (strcmp(message.msg_text, "Y") == 0)
        {
            if (msgsnd(msg_id, &termination_msg, sizeof(msg_buffer), 0) == -1)
            { // Send message to the message queue
                perror("msgsnd");
                exit(EXIT_FAILURE);
            }

            if (msgctl(msg_id, IPC_RMID, NULL) == -1)
            { // Remove the message queue
                perror("msgctl");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    return 0;
}
