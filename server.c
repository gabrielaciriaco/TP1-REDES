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
#include <time.h>
#include <unistd.h>

#define MAX_CONNECTIONS 2
#define PORT 3000

enum COMMAND_TYPE { ADD = 0, REMOVE = 1, LIST = 2, READ = 3 };

int initializeSocket(const char *ipVersion) {
  int domain, addressSize, serverfd, new_fd, yes = 1;
  struct sockaddr *address;
  struct sockaddr_in6 addressv6;
  struct sockaddr_in addressv4;

  addressv6.sin6_family = AF_INET6;
  addressv6.sin6_port = htons(PORT);
  addressv6.sin6_addr = in6addr_any;

  addressv4.sin_family = AF_INET;
  addressv4.sin_port = htons(PORT);
  addressv4.sin_addr.s_addr = htonl(INADDR_ANY);

  if (strcmp(ipVersion, "v4") == 0) {
    domain = AF_INET;
    addressSize = sizeof(addressv4);
    address = (struct sockaddr *)&addressv4;

  } else if (strcmp(ipVersion, "v6") == 0) {
    domain = AF_INET6;
    addressSize = sizeof(addressv6);
    address = (struct sockaddr *)&addressv6;
  }

  if ((serverfd = socket(domain, SOCK_STREAM, 0)) == 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,
                 sizeof(yes)) == -1) {
    perror("Could not set socket option");
    exit(EXIT_FAILURE);
  }

  if (bind(serverfd, address, addressSize) < 0) {
    perror("Could not bind port to socket");
    exit(EXIT_FAILURE);
  }

  if (listen(serverfd, MAX_CONNECTIONS) < 0) {
    perror("Could not listen on designeted address");
    exit(EXIT_FAILURE);
  }

  int addrSize = sizeof(address);
  if ((new_fd = accept(serverfd, (struct sockaddr *)&address,
                       (socklen_t *)&addrSize)) < 0) {
    perror("Could not accept connections in this server");
    exit(EXIT_FAILURE);
  }
  return new_fd;
}

typedef struct {
  int commandType;
  int equipmentId;
  int sensorsIds[15];
} Command;

void getSensorsIds(char **commandToken, Command *command) {
  *commandToken = strtok(NULL, " ");
  for (int i = 0; strcmp(*commandToken, "in") != 0; i++) {
    command->sensorsIds[i] = atoi(*commandToken) - 1;
    *commandToken = strtok(NULL, " ");
  }
}

Command interpretCommand(char *buffer) {
  char *commandToken = strtok(buffer, " ");
  Command command;

  for (int i = 0; i < 15; i++) {
    command.sensorsIds[i] = -1;
  }

  if (strcmp(commandToken, "add") == 0 &&
      strcmp(commandToken = strtok(NULL, " "), "sensor") == 0) {
    command.commandType = ADD;
    getSensorsIds(&commandToken, &command);
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else if (strcmp(commandToken, "remove") == 0 &&
           strcmp(commandToken = strtok(NULL, " "), "sensor") == 0) {
    command.commandType = REMOVE;
    getSensorsIds(&commandToken, &command);
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else if (strcmp(commandToken, "list") == 0 &&
           strcmp(commandToken = strtok(NULL, " "), "sensors") == 0 &&
           strcmp(commandToken = strtok(NULL, " "), "in") == 0) {
    command.commandType = LIST;
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else if (strcmp(commandToken, "read") == 0) {
    command.commandType = READ;
    getSensorsIds(&commandToken, &command);
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else {
    command.commandType = -1;
  }
  return command;
}

char *addSensors(Command command, float equipments[4][4],
                 int *remainingSensors) {
  int addedSensors[3] = {-1, -1, -1};
  int existentSensors[3] = {-1, -1, -1};
  int addedSensorsIndex = 0, existentSensorsIndex = 0;

  int addedSensorsCount;
  for (addedSensorsCount = 0; command.sensorsIds[addedSensorsCount] != -1;
       addedSensorsCount++) {
  }
  if (addedSensorsCount > *remainingSensors) {
    return "limit exceeded";
  }
  for (int i = 0; command.sensorsIds[i] != -1; i++) {
    if (equipments[command.equipmentId][command.sensorsIds[i]] != -1) {
      existentSensors[existentSensorsIndex] = command.sensorsIds[i];
      existentSensorsIndex++;
    } else {
      addedSensors[addedSensorsIndex] = command.sensorsIds[i];
      addedSensorsIndex++;
      float sensorReading = 10 * ((float)rand() / RAND_MAX);
      equipments[command.equipmentId][command.sensorsIds[i]] = sensorReading;
      (*remainingSensors)--;
    }
  }

  char *commandOutput = malloc(256 * sizeof(char));
  int bufferIndex = 0;
  if (addedSensors[0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "sensor");
    for (int i = 0; addedSensors[i] != -1 && i < 3; i++) {
      bufferIndex +=
          sprintf(&commandOutput[bufferIndex], " %02d", addedSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " added");
  }

  if (existentSensors[0] != -1) {
    if (addedSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " ");
    }
    bufferIndex +=
        sprintf(&commandOutput[bufferIndex], "%02d", existentSensors[0] + 1);
    for (int i = 1; existentSensors[i] != -1 && i < 3; i++) {
      bufferIndex +=
          sprintf(&commandOutput[bufferIndex], " %02d", existentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex],
                           " already exists in %02d", command.equipmentId + 1);
  }
  return commandOutput;
}

char *removeSensors(Command command, float equipments[4][4],
                    int *remainingSensors) {
  int removedSensors[3] = {-1, -1, -1};
  int nonExistentSensors[3] = {-1, -1, -1};
  int removedSensorsIndex = 0, nonExistentSensorsIndex = 0;

  for (int i = 0; command.sensorsIds[i] != -1; i++) {
    if (equipments[command.equipmentId][command.sensorsIds[i]] == -1) {
      nonExistentSensors[nonExistentSensorsIndex] = command.sensorsIds[i];
      nonExistentSensorsIndex++;
    } else {
      removedSensors[removedSensorsIndex] = command.sensorsIds[i];
      removedSensorsIndex++;
      equipments[command.equipmentId][command.sensorsIds[i]] = -1;
      (*remainingSensors)++;
    }
  }

  char *commandOutput = malloc(256 * sizeof(char));
  int bufferIndex = 0;
  if (removedSensors[0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "sensor");
    for (int i = 0; removedSensors[i] != -1 && i < 3; i++) {
      bufferIndex +=
          sprintf(&commandOutput[bufferIndex], " %02d", removedSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " removed");
  }

  if (nonExistentSensors[0] != -1) {
    if (removedSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " ");
    }
    bufferIndex +=
        sprintf(&commandOutput[bufferIndex], "%02d", nonExistentSensors[0] + 1);

    for (int i = 1; nonExistentSensors[i] != -1 && i < 3; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d",
                             nonExistentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex],
                           " does not exist in %02d", command.equipmentId + 1);
  }
  return commandOutput;
}

char *listSensors(Command command, float equipments[4][4]) {
  int equipmentId = command.equipmentId;
  char *commandOutput = malloc(256 * sizeof(char));
  int bufferIndex = 0;

  if (equipments[equipmentId][0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "%02d", 1);
  }

  for (int i = 1; i < 4; i++) {
    if (equipments[equipmentId][i] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", i + 1);
    }
  }

  if (strlen(commandOutput) == 0) {
    return "none";
  }

  return commandOutput;
}

char *readSensors(Command command, float equipments[4][4]) {
  int availableSensors[4] = {-1, -1, -1, -1};
  int nonExistentSensors[4] = {-1, -1, -1, -1};
  int availableSensorsIndex = 0, nonExistentSensorsIndex = 0;

  for (int i = 0; command.sensorsIds[i] != -1; i++) {
    if (equipments[command.equipmentId][i] == -1) {
      nonExistentSensors[nonExistentSensorsIndex] = command.sensorsIds[i];
      nonExistentSensorsIndex++;
    } else {
      availableSensors[availableSensorsIndex] = command.sensorsIds[i];
      availableSensorsIndex++;
    }
  }

  char *commandOutput = malloc(256 * sizeof(char));
  int bufferIndex = 0;
  if (availableSensors[0] != -1) {
    bufferIndex +=
        sprintf(&commandOutput[bufferIndex], "%.2f",
                equipments[command.equipmentId][availableSensors[0]]);
    for (int i = 1; availableSensors[i] != -1 && i < 4; i++) {
      bufferIndex +=
          sprintf(&commandOutput[bufferIndex], " %.2f",
                  equipments[command.equipmentId][availableSensors[i]]);
    }
  }

  if (nonExistentSensors[0] != -1) {
    if (availableSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " and ");
    }
    bufferIndex +=
        sprintf(&commandOutput[bufferIndex], "%02d", nonExistentSensors[0] + 1);
    for (int i = 1; nonExistentSensors[i] != -1 && i < 4; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d",
                             nonExistentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " not installed");
  }

  return commandOutput;
}

char *executeCommand(Command command, float equipments[4][4],
                     int *remainingSensors) {

  if (command.equipmentId < 0 || command.equipmentId > 3) {
    char *invalidEquipment = "invalid equipment";
    return invalidEquipment;
  }
  switch (command.commandType) {
    case ADD:
      return addSensors(command, equipments, remainingSensors);
    case REMOVE:
      return removeSensors(command, equipments, remainingSensors);
    case LIST:
      return listSensors(command, equipments);
    case READ:
      return readSensors(command, equipments);
    default:
      break;
  }
}

int main(int argc, char const *argv[]) {
  srand(time(NULL));
  int valread;

  char buffer[1024] = {0};
  float equipments[4][4] = {
      {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}};
  int remainingSensors = 15;

  while (1) {
    int new_fd = initializeSocket(argv[1]);
    while (1) {
      valread = read(new_fd, buffer, 1024);
      Command command = interpretCommand(buffer);
      if (command.commandType == -1) {
        close(new_fd);
        break;
      }
      char *commandOutput =
          executeCommand(command, equipments, &remainingSensors);
      send(new_fd, commandOutput, strlen(commandOutput), 0);
    }
  }

  return 0;
}