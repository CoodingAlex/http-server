#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

char *getpath(char *req);

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
  char *r = getpath(buffer);

  if (strcmp(r, "/") == 0) {
    char *res = "HTTP/1.1 200 OK\r\n\r\n";
    send(new_sock, res, strlen(res), 0);
  } else {
    char *res = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
    send(new_sock, res, strlen(res), 0);
  }
  printf("%s\n", r);

  printf("Client connected\n");

  close(server_fd);
  free(r);

  return 0;
}

char *getpath(char *req) {
  const char *start = req;

  while (*start && *start != '/') {
    start++;
  }

  const char *end = start;

  while (*end && *end != ' ') {
    end++;
  }

  size_t length = end - start;

  char *path = malloc(length);

  if (!path) {
    return NULL;
  }

  char *p = path;

  while (start < end) {
    *p++ = *start++;
  }
  *p = '\0';

  return path;
}
