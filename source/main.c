#include <error.h>
#include <iso646.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyosc/tinyosc.h"
#include <netinet/in.h>
#include <sys/socket.h>

enum ExitCode {
    kExitSuccess = EXIT_SUCCESS,
    kExitErrorStd = EXIT_FAILURE,
    kExitErrorGetFlag = 10,
    kExitErrorSocket = 20,
    kExitErrorVrchatLimit = 30,
};

void SubcommandChatbox(int argc, char *argv[], int arg_now)
{
    arg_now++;

    char *message;
    if (arg_now >= argc) {
        message = "Hello VRChat Chatbox OSC > This message sended by VRChat-OSC-Shell";
    } else {
        size_t msg_size = 0;
        bool first_argv = true;

        while (arg_now < argc) {
            msg_size += strlen(argv[arg_now]) + 1 + sizeof('\n'); // \n may not a char but a int
            message = realloc(message, msg_size);

            // the first line don't need \n at start
            if (first_argv == true) {
                first_argv = false;
            } else {
                strncat(message, "\n", msg_size);
            }
            strncat(message, argv[arg_now], msg_size);

            arg_now++;
        }
    }

    // VRChat chatbox only allow message of less than 144 characters
    int message_len = strlen(message);
    if (message_len > 144) {
        printf("VRChat limited chatbox characters to 144, but the input is %d\n", message_len);
        exit(kExitErrorVrchatLimit);
    }

    // VRChat OSC server socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Socket create error");
        exit(kExitErrorStd);
    }

    struct sockaddr_in des_addr = {0};
    des_addr.sin_family = AF_INET;
    des_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    des_addr.sin_port = htons(9000);

    char buffer[256] = {0};
    int len = tosc_writeMessage(buffer, sizeof(buffer), "/chatbox/input", "sT", message);

    if (sendto(fd, buffer, len, 0, (struct sockaddr *)&des_addr, sizeof(des_addr)) == -1) {
        perror("Send message error");
        exit(kExitErrorSocket);
    }
    printf("Sended message to localhost:9000 <-|\n%s\n", message);
    for (int i = 0; i < 24; i++)
        putchar('-');
    putchar('\n');

    close(fd);

    exit(kExitSuccess);
    return;
}

void SubcommandTyping(int argc, char *argv[], int arg_now)
{
    enum {
        kTypingOn,
        kTypingOff,
    } typing_status;

    arg_now++;
    if (arg_now + 1 != argc) {
        printf("Too many or too few arguments\n");
        exit(kExitErrorGetFlag);
    }

    if (strcmp(argv[arg_now], "on") == 0) {
        typing_status = kTypingOn;
    } else if (strcmp(argv[arg_now], "off") == 0) {
        typing_status = kTypingOff;
    } else {
        printf("Unknown typing status: %s\n", argv[arg_now]);
        exit(kExitErrorGetFlag);
    }

    // VRChat OSC server socket
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        perror("Socket create error");
        exit(kExitErrorStd);
    }

    struct sockaddr_in des_addr = {0};
    des_addr.sin_family = AF_INET;
    des_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    des_addr.sin_port = htons(9000);

    char buffer[256] = {0};
    char *format;
    switch (typing_status) {
    case kTypingOn:
        format = "T";
        break;
    case kTypingOff:
        format = "F";
        break;
    }
    int len = tosc_writeMessage(buffer, sizeof(buffer), "/chatbox/typing", format);

    if (sendto(fd, buffer, len, 0, (struct sockaddr *)&des_addr, sizeof(des_addr)) == -1) {
        perror("Send message error");
        exit(kExitErrorSocket);
    }
    printf("Sended typing status with format: %s\n", format);

    close(fd);

    exit(kExitSuccess);
    return;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("No arguments, run with help/--help/-h get more information\n");
        exit(kExitErrorStd);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "help") == 0 or strcmp(argv[i], "--help") == 0 or strcmp(argv[i], "-h") == 0) {
            printf("Usage: vrchat-osc <MODE> [Mode args] [--help | -h]\n\n");
            printf("MODE:\n");
            printf("\thelp\t\t\t\tShow help info, --help also will do this\n");
            printf("\tchatbox [strings] [...]\t\tSend chatbox message. Each argument will converted into a separate line\n");
            printf("\ttyping <on/off>\t\t\tControl the typing indicator on or off\n");
            exit(kExitSuccess);
        } else if (strcmp(argv[i], "chatbox") == 0) {
            SubcommandChatbox(argc, argv, i);
        } else if (strcmp(argv[i], "typing") == 0) {
            SubcommandTyping(argc, argv, i);
        } else {
            printf("Unknown subcommand \"%s\"\n", argv[i]);
            exit(kExitErrorGetFlag);
        }
    }

    return kExitSuccess;
}
