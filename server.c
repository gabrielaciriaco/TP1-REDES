#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 3000

int main(void) {
  int serverfd, new_serverfd, valread;
  struct sockaddr_in address;
  int yes = 1;
  char buffer[1024] = {0};
  char *test_send = "Resposta Teste\n";

  if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,
                 sizeof(yes)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(serverfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(serverfd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  int addr_size = sizeof(address);
  while (1) {
    if ((new_serverfd = accept(serverfd, (struct sockaddr *)&address,
                               (socklen_t *)&addr_size)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
    }
    while (1) {
      valread = read(new_serverfd, buffer, 1024);
      char *command = strtok(buffer," ");
      if (strcmp(command, "kill")==0) {
        break;
      }
      printf("%s\n", buffer);
      send(new_serverfd, test_send, strlen(test_send), 0);
    }
  }
  return 0;
}