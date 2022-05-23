#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_MESSAGE_SIZE 500

int connectToServer(char *argv[]) {
  int domain, addressSize, sockfd;

  struct sockaddr *address;
  struct sockaddr_in addressv4;
  struct sockaddr_in6 addressv6;

  addressv4.sin_family = AF_INET;
  addressv4.sin_port = htons(atoi(argv[2]));

  addressv6.sin6_family = AF_INET6;
  addressv6.sin6_port = htons(atoi(argv[2]));

  if (inet_pton(AF_INET, argv[1], &addressv4.sin_addr) > 0) {
    domain = AF_INET;
    addressSize = sizeof(addressv4);
    address = (struct sockaddr *)&addressv4;
  }

  if (inet_pton(AF_INET6, argv[1], &addressv6.sin6_addr) > 0) {
    domain = AF_INET6;
    addressSize = sizeof(addressv6);
    address = (struct sockaddr *)&addressv6;
  }

  if ((sockfd = socket(domain, SOCK_STREAM, IPPROTO_TCP)) == 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }

  if (connect(sockfd, address, addressSize) == -1) {
    perror("Could not connect to server");
    exit(EXIT_FAILURE);
  }

  return sockfd;
}

int main(int argc, char *argv[]) {
  int sockfd = connectToServer(argv);

  while (1) {
    char buffer[MAX_MESSAGE_SIZE];

    fflush(stdin);
    scanf("%[^\n]%*c", buffer);

    int isKill = strcmp(buffer, "kill") == 0;
    
    int bytesSent = send(sockfd, buffer, strlen(buffer), 0);

    if (bytesSent == 0) {
      perror("Could not communicate to server\n");
      exit(EXIT_FAILURE);
    }

    if(isKill) {
      exit(EXIT_SUCCESS);
    }

    memset(buffer, 0, sizeof(buffer));
    int bytesRead = read(sockfd, buffer, MAX_MESSAGE_SIZE - 1);

    if (bytesRead == 0) {
      perror("Could not communicate to server\n");
      exit(EXIT_FAILURE);
    }

    printf("%s", buffer);
  }

  close(sockfd);

  return 0;
}