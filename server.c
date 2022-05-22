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
#define MAX_MESSAGE_SIZE 500
#define INVALID_SENSOR_ID -2

enum COMMAND_TYPE {
  ADD = 0,
  REMOVE = 1,
  LIST = 2,
  READ = 3,
  KILL = 4
};

int initializeSocket(const char* ipVersion, const char* port) {
  int domain, addressSize, serverfd, sockfd, yes = 1;
  struct sockaddr* address;
  struct sockaddr_in6 addressv6;
  struct sockaddr_in addressv4;

  addressv4.sin_family = AF_INET;
  addressv4.sin_port = htons(atoi(port));
  addressv4.sin_addr.s_addr = htonl(INADDR_ANY);

  addressv6.sin6_family = AF_INET6;
  addressv6.sin6_port = htons(atoi(port));
  addressv6.sin6_addr = in6addr_any;

  if (strcmp(ipVersion, "v4") == 0) {
    domain = AF_INET;
    addressSize = sizeof(addressv4);
    address = (struct sockaddr*)&addressv4;

  } else if (strcmp(ipVersion, "v6") == 0) {
    domain = AF_INET6;
    addressSize = sizeof(addressv6);
    address = (struct sockaddr*)&addressv6;
  }

  if ((serverfd = socket(domain, SOCK_STREAM, IPPROTO_TCP)) == 0) {
    perror("Could not create socket");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) == -1) {
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
  if ((sockfd = accept(serverfd, (struct sockaddr*)&address, (socklen_t*)&addrSize)) < 0) {
    perror("Could not accept connections in this server");
    exit(EXIT_FAILURE);
  }

  return sockfd;
}

typedef struct {
  int commandType;
  int equipmentId;
  int sensorsIds[3];
} Command;

void getSensorsIds(char** commandToken, Command* command) {
  *commandToken = strtok(NULL, " ");
  for (int i = 0; strcmp(*commandToken, "in") != 0; i++) {
    if (atoi(*commandToken) < 1 || atoi(*commandToken) > 4) {
      command->sensorsIds[i] = INVALID_SENSOR_ID;
    } else {
      command->sensorsIds[i] = atoi(*commandToken) - 1;
    }
    *commandToken = strtok(NULL, " ");
  }
}

Command interpretCommand(char* buffer) {
  char* commandToken = strtok(buffer, " ");
  Command command;

  for (int i = 0; i < 3; i++) {
    command.sensorsIds[i] = -1;
  }

  if (strcmp(commandToken, "add") == 0 && strcmp(commandToken = strtok(NULL, " "), "sensor") == 0) {
    command.commandType = ADD;
    getSensorsIds(&commandToken, &command);
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else if (strcmp(commandToken, "remove") == 0 && strcmp(commandToken = strtok(NULL, " "), "sensor") == 0) {
    command.commandType = REMOVE;
    getSensorsIds(&commandToken, &command);
    commandToken = strtok(NULL, " ");
    command.equipmentId = atoi(commandToken) - 1;
  }

  else if (strcmp(commandToken, "list") == 0 && strcmp(commandToken = strtok(NULL, " "), "sensors") == 0 && strcmp(commandToken = strtok(NULL, " "), "in") == 0) {
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
    command.commandType = KILL;
  }

  return command;
}

void addSensors(Command command, float equipments[4][4], int* remainingSensors, char* commandOutput) {
  int addedSensors[3] = {-1, -1, -1};
  int existentSensors[3] = {-1, -1, -1};
  int addedSensorsIndex = 0, existentSensorsIndex = 0;

  int addedSensorsCount;
  for (addedSensorsCount = 0; command.sensorsIds[addedSensorsCount] != -1;
       addedSensorsCount++) {
  }
  if (addedSensorsCount > *remainingSensors) {
    commandOutput = "limit exceeded";
    return;
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

  int bufferIndex = 0;
  if (addedSensors[0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "sensor");
    for (int i = 0; addedSensors[i] != -1 && i < 3; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", addedSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " added");
  }

  if (existentSensors[0] != -1) {
    if (addedSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " ");
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], "%02d", existentSensors[0] + 1);
    for (int i = 1; existentSensors[i] != -1 && i < 3; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", existentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " already exists in %02d", command.equipmentId + 1);
  }
}

void removeSensors(Command command, float equipments[4][4], int* remainingSensors, char* commandOutput) {
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

  int bufferIndex = 0;
  if (removedSensors[0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "sensor");
    for (int i = 0; removedSensors[i] != -1 && i < 3; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", removedSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " removed");
  }

  if (nonExistentSensors[0] != -1) {
    if (removedSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " ");
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], "%02d", nonExistentSensors[0] + 1);

    for (int i = 1; nonExistentSensors[i] != -1 && i < 3; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", nonExistentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " does not exist in %02d", command.equipmentId + 1);
  }
}

void listSensors(Command command, float equipments[4][4], char* commandOutput) {
  int equipmentId = command.equipmentId;
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
    sprintf(commandOutput, "none");
  }
}

void readSensors(Command command, float equipments[4][4], char* commandOutput) {
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

  int bufferIndex = 0;
  if (availableSensors[0] != -1) {
    bufferIndex += sprintf(&commandOutput[bufferIndex], "%.2f", equipments[command.equipmentId][availableSensors[0]]);
    for (int i = 1; availableSensors[i] != -1 && i < 4; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %.2f", equipments[command.equipmentId][availableSensors[i]]);
    }
  }

  if (nonExistentSensors[0] != -1) {
    if (availableSensors[0] != -1) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " and ");
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], "%02d", nonExistentSensors[0] + 1);

    for (int i = 1; nonExistentSensors[i] != -1 && i < 4; i++) {
      bufferIndex += sprintf(&commandOutput[bufferIndex], " %02d", nonExistentSensors[i] + 1);
    }
    bufferIndex += sprintf(&commandOutput[bufferIndex], " not installed");
  }
}

int executeCommand(Command command, float equipments[4][4], int* remainingSensors, int sockfd, char* commandOutput) {
  if (command.commandType != KILL && (command.equipmentId < 0 || command.equipmentId > 3)) {
    sprintf(commandOutput, "invalid equipment");
    return 0;
  }

  if (command.commandType != KILL) {
    for (int i = 0; i < 3; i++) {
      if (command.sensorsIds[i] == INVALID_SENSOR_ID) {
        sprintf(commandOutput, "invalid sensor");
        return 0;
      }
    }
  }

  switch (command.commandType) {
    case ADD:
      addSensors(command, equipments, remainingSensors, commandOutput);
      break;
    case REMOVE:
      removeSensors(command, equipments, remainingSensors, commandOutput);
      break;
    case LIST:
      listSensors(command, equipments, commandOutput);
      break;
    case READ:
      readSensors(command, equipments, commandOutput);
      break;
    case KILL:
      close(sockfd);
      return -1;
    default:
      break;
  }

  return 0;
}

int main(int argc, char const* argv[]) {
  srand(time(NULL));
  float equipments[4][4] = {
      {-1, -1, -1, -1},
      {-1, -1, -1, -1},
      {-1, -1, -1, -1},
      {-1, -1, -1, -1}};
  int remainingSensors = 15;

  while (1) {
    int sockfd = initializeSocket(argv[1], argv[2]);

    while (1) {
      char buffer[MAX_MESSAGE_SIZE] = {0};
      int bytesRead = read(sockfd, buffer, MAX_MESSAGE_SIZE - 1);

      if (bytesRead < 1) {
        close(sockfd);
        break;
      }

      printf("%s\n", buffer);

      Command command = interpretCommand(buffer);
      char* commandOutput = malloc(sizeof(char) * 256);

      if (executeCommand(command, equipments, &remainingSensors, sockfd, commandOutput) == -1) {
        break;
      }

      sprintf(&commandOutput[strlen(commandOutput)], "\n");
      send(sockfd, commandOutput, strlen(commandOutput), 0);
    }
  }

  return 0;
}