#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#define TRUE 1

char *getpath(char *req);
int getpaths(char *fp, char *paths[24]);
char *substr(char *str, int start);

void handle_request(int sock, char buffer[1024], int argc, char *argv[]);

int main(int argc, char *argv[]) {
  setbuf(stdout, NULL);
  int master_socket, new_socket, max_clients = 30, client_socks[30], activity,
                                 i, sd, client_addr_len;
  ssize_t valread;
  int max_sd;
  char buffer[1024] = {0};

  fd_set readsfs, sockets;

  for (i = 0; i < max_clients; i++) {
    client_socks[i] = 0;
  }

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");

  // Uncomment this block to pass the first stage

  master_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (master_socket == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEPORT, &reuse,
                 sizeof(reuse)) < 0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
    return 1;
  }
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };
  if (bind(master_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) !=
      0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(master_socket, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");

  FD_ZERO(&readsfs);
  FD_SET(master_socket, &readsfs);

  while (TRUE) {
    sockets = readsfs;

    activity = select(FD_SETSIZE, &sockets, NULL, NULL, NULL);
    if (activity < 0) {
      printf("select error\n");
    }

    for (i = 0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(i, &sockets)) {
        if (i == master_socket) {
          struct sockaddr_in client_addr;
          client_addr_len = sizeof(client_addr);
          int client_socket;

          if ((client_socket =
                   accept(master_socket, (struct sockaddr *)&client_addr,
                          &client_addr_len)) < 0) {

            perror("accept");
            exit(EXIT_FAILURE);
          }
          FD_SET(client_socket, &readsfs);
        } else {
          valread = read(i, buffer, 1024);
          handle_request(i, buffer, argc, argv);
          puts("Response sent correctly");
          close(i);
          memset(buffer, 0, 1024);
          FD_CLR(i, &readsfs);
        }
      }
    }
  }

  close(master_socket);

  return 0;
}

char *substr(char *str, int start) {
  char *new_str = malloc(((strlen(str) - start) * sizeof(char)) + 1);
  char *s = new_str;
  int i;
  for (i = 0; str[i + start] != '\0'; *s = str[i + start], i++, s++)
    ;
  *s = '\0';

  return new_str;
}

void handle_request(int sock, char buffer[1024], int argc, char *argv[]) {
  char header_tokens[1024][1024];
  printf("%s\n---------------------\n", buffer);
  char *bodyp = strstr(buffer, "\r\n\r\n");
  char *body;
  if (bodyp != NULL) {
    body = malloc(strlen(bodyp) + 1);
    bodyp += 4;
    strcpy(body, bodyp);
  }
  printf("body: %s\n", body);
  char *header_token = strtok(buffer, " \r\n");

  int idx = 0;
  while (header_token != NULL) {
    strcpy(header_tokens[idx], header_token);
    header_token = strtok(NULL, " \r\n");
    idx++;
  }

  idx = 0;

  char path[strlen(header_tokens[1]) + 1];
  strcpy(path, header_tokens[1]);
  char path_tokens[1024][1024];

  char *path_token = strtok(path, "/");
  if (path_token == NULL) {
    strcpy(path_tokens[0], "/");
  }

  while (path_token != NULL) {
    strcpy(path_tokens[idx], path_token);
    path_token = strtok(NULL, "/");
    idx++;
  }

  if (strcmp(path_tokens[0], "/") == 0 || path_tokens[0] == NULL) {
    char *res = "HTTP/1.1 200 OK\r\n\r\n";
    send(sock, res, strlen(res), 0);
  } else if (strcmp(path_tokens[0], "echo") == 0) {
    char *res;
    char *echo_cmd = substr(header_tokens[1], 6);
    printf("sub: %s\n", echo_cmd);
    int size = asprintf(&res,
                        "HTTP/1.1 200 OK\r\nContent-Type: "
                        "text/plain\r\nContent-Length: %ld\r\n\r\n%s",
                        strlen(echo_cmd), echo_cmd);

    send(sock, res, strlen(res), 0);
    free(echo_cmd);
  } else if (strcmp(path_tokens[0], "files") == 0 &&
             strcmp(header_tokens[0], "POST") == 0) {
    if (argc == 3) {
      char *dir_path = argv[2];
      char *dir_arg;
      char *file_path = malloc(strlen(dir_path) + strlen(path_tokens[1]) + 2);
      if (file_path == NULL) {
        printf("ERROR ALLOCATING FILEPATH\n");
        return;
      }
      if (dir_path[strlen(dir_path) - 1] != '/') {
        snprintf(file_path, strlen(dir_path) + strlen(path_tokens[1]) + 2,
                 "%s/%s", dir_path, path_tokens[1]);
      } else {
        snprintf(file_path, strlen(dir_path) + strlen(path_tokens[1]) + 2,
                 "%s%s", dir_path, path_tokens[1]);
      }

      FILE *f = fopen(file_path, "w");

      if (f != NULL) {
        printf("body: %s\n", body);
        fprintf(f, "%s", body);
        fclose(f);
        char *res = "HTTP/1.1 201 CREATED\r\n\r\n";
        send(sock, res, strlen(res), 0);
      } else {
        char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        send(sock, res, strlen(res), 0);
        free(file_path);
        return;
      }
      free(file_path);
    }

  } else if (strcmp(path_tokens[0], "files") == 0 &&
             strcmp(header_tokens[0], "GET") == 0) {
    if (argc == 3) {
      char *dir_path = argv[2];
      char *dir_arg;
      char *file_path = malloc(strlen(dir_path) + strlen(path_tokens[1]) + 2);
      if (file_path == NULL) {
        printf("ERROR ALLOCATING FILEPATH\n");
        return;
      }
      if (dir_path[strlen(dir_path) - 1] != '/') {
        snprintf(file_path, strlen(dir_path) + strlen(path_tokens[1]) + 2,
                 "%s/%s", dir_path, path_tokens[1]);
      } else {
        snprintf(file_path, strlen(dir_path) + strlen(path_tokens[1]) + 2,
                 "%s%s", dir_path, path_tokens[1]);
      }

      char *file_buffer;
      long length;
      FILE *f = fopen(file_path, "r");

      if (f != NULL) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        file_buffer = malloc(length);
        printf("len %ld\n", length);
        if (file_buffer) {
          fread(file_buffer, 1, length, f);
        }
        fclose(f);
      } else {
        char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        send(sock, res, strlen(res), 0);
        free(file_path);
        free(file_buffer);
        return;
      }
      if (file_buffer) {
        char *res;
        int size = asprintf(
            &res,
            "HTTP/1.1 200 OK\r\nContent-Type: "
            "application/octet-stream\r\nContent-Length: %ld\r\n\r\n%s",
            strlen(file_buffer), file_buffer);
        send(sock, res, strlen(res), 0);
      }
      free(file_path);
      free(file_buffer);
    } else {
      char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
      send(sock, res, strlen(res), 0);
    }

  } else if (strcmp(path_tokens[0], "user-agent") == 0) {
    char *res;
    int size = asprintf(&res,
                        "HTTP/1.1 200 OK\r\nContent-Type: "
                        "text/plain\r\nContent-Length: %ld\r\n\r\n%s",
                        strlen(header_tokens[6]), header_tokens[6]);

    send(sock, res, strlen(res), 0);
  } else {
    char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
    send(sock, res, strlen(res), 0);
  }
}
