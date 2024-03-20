#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char *getpath(char *req);
int getpaths(char *fp, char *paths[24]);
char *substr(char *str, int start);

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  int new_sock;
  ssize_t valread;
  char buffer[1024] = {0};

  // You can use print statements as follows for debugging, they'll be visible
  // when running tests.
  printf("Logs from your program will appear here!\n");

  // Uncomment this block to pass the first stage
  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // // Since the tester restarts your program quite often, setting REUSE_PORT
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEPORT failed: %s \n", strerror(errno));
    return 1;
  }
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)},
  };
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);

  if ((new_sock = accept(server_fd, (struct sockaddr *)&client_addr,
                         &client_addr_len)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  valread = read(new_sock, buffer, 1024 - 1);

  char header_tokens[1024][1024];
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
    printf("HOLA");
    char *res = "HTTP/1.1 200 OK\r\n\r\n";
    send(new_sock, res, strlen(res), 0);
  } else if (strcmp(path_tokens[0], "echo") == 0) {
    char *res;
    char *echo_cmd = substr(header_tokens[1], 6);
    printf("sub: %s\n", echo_cmd);
    int size = asprintf(&res,
                        "HTTP/1.1 200 OK\r\nContent-Type: "
                        "text/plain\r\nContent-Length: %ld\r\n\r\n%s",
                        strlen(echo_cmd), echo_cmd);

    send(new_sock, res, strlen(res), 0);
  } else if (strcmp(path_tokens[0], "user-agent") == 0) {
    char *res;
    int size = asprintf(&res,
                        "HTTP/1.1 200 OK\r\nContent-Type: "
                        "text/plain\r\nContent-Length: %ld\r\n\r\n%s",
                        strlen(header_tokens[6]), header_tokens[6]);

    send(new_sock, res, strlen(res), 0);
  } else {
    char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
    send(new_sock, res, strlen(res), 0);
  }

  printf("Client connected\n");

  close(server_fd);

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
