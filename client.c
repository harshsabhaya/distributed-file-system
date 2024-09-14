#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define IP "127.0.0.1"

int PORT = 2486;

// int readPortNumber()
// {
//   FILE *file = fopen("port_smain.txt", "r");
//   if (file == NULL)
//   {
//     printf("Error opening file for reading.\n");
//     return 1;
//   }

//   fscanf(file, "%d", &PORT);
//   fclose(file);

//   printf("PORT Number is %d\n", PORT);
//   return 0;
// }

char *extractFilename(const char *full_path)
{
  return strrchr(full_path, '/') + 1;
}

void handle_display_command(int server_sock)
{
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);

  while (recv(server_sock, buffer, BUFFER_SIZE, 0) > 0)
  {

    if (strcmp(buffer, "EOF") == 0)
      break;

    printf("%s", buffer);
    memset(buffer, 0, BUFFER_SIZE);
  }
}

int main(int argc, char *argv[])
{
  // read smain port and set global
  // readPortNumber();

  int server_sock;
  struct sockaddr_in serv_addr;
  char command[BUFFER_SIZE];
  char filename[BUFFER_SIZE];
  char dest_path[BUFFER_SIZE];
  char arg1[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Cannot create socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0)
  {
    perror("Invalid address/ Address not supported");
    exit(2);
  }

  if (connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("Connection failed");
    exit(3);
  }

  while (1)
  {
    printf("\x1b[1;33mclient24s$\x1b[0m ");
    memset(command, 0, BUFFER_SIZE);
    memset(buffer, 0, BUFFER_SIZE);
    memset(arg1, 0, BUFFER_SIZE);

    fgets(command, BUFFER_SIZE, stdin);
    command[strcspn(command, "\n")] = 0; // Remove trailing newline

    // extract arguments from the command
    sscanf(command, "%s %s", buffer, arg1);

    if (strcmp(buffer, "ufile") == 0 && (strstr(arg1, ".c") != NULL || strstr(arg1, ".txt") != NULL || strstr(arg1, ".pdf") != NULL))
    {

      // Send command to Smain
      send(server_sock, command, strlen(command), 0);

      sscanf(command, "%s %s %s", buffer, filename, dest_path);
      printf("buffer:: %s\n", buffer);
      printf("filename:: %s\n", filename);
      printf("dest_path:: %s\n", dest_path);

      FILE *fp = fopen(filename, "rb");
      if (!fp)
      {
        perror("File open failed");
        continue;
      }

      // Wait for server response
      memset(buffer, 0, BUFFER_SIZE);
      recv(server_sock, buffer, BUFFER_SIZE, 0);

      if (strcmp(buffer, "Ready to receive file") == 0)
      {
        int n;
        while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
        {
          if (send(server_sock, buffer, n, 0) != n)
          {
            perror("Send error");
            break;
          }
        }
        send(server_sock, "EOF", strlen("EOF"), 0);
        printf("File %s uploaded to Smain\n", filename);
      }

      fclose(fp);
    }
    else if (strcmp(buffer, "dfile") == 0 && (strstr(arg1, ".c") != NULL || strstr(arg1, ".txt") != NULL || strstr(arg1, ".pdf") != NULL))
    {

      // Send command to Smain
      send(server_sock, command, strlen(command), 0);

      // extracting buffer and filePath
      sscanf(command, "%s %s", buffer, dest_path);

      printf("buffer:: %s\n", buffer);
      printf("dest_path:: %s\n", dest_path);

      // Receive acknowledgment from the server
      memset(buffer, 0, BUFFER_SIZE);
      recv(server_sock, buffer, BUFFER_SIZE, 0);

      printf("buffer from smain -- %s\n", buffer);

      printf("strcmp(buffer, 'File exists') -- %d\n", strcmp(buffer, "File exists"));
      if (strcmp(buffer, "File exists") == 0)
      {

        printf("File Exist at Stext\n");
        // File exists on the server, create a file locally
        char *fName = extractFilename(dest_path);
        FILE *fp = fopen(fName, "wb");
        if (!fp)
        {
          perror("File open failed");
          return 0;
        }
        printf("File created at client side and fp is ::%d\n", fp);

        // Inform the server that the client is ready to receive the file
        send(server_sock, "Ready to receive file", strlen("Ready to receive file"), 0);
        printf("Sending server that I am ready\n");

        int n;
        while ((n = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0)
        {

          printf("Getting file from server\n");
          if (strstr(buffer, "EOF") != NULL)
            break;

          printf("Received %d bytes, \n %s...........\n", n, buffer);
          if (fwrite(buffer, 1, n, fp) != n)
          {
            perror("Write to file failed");
            break;
          }
        }

        fclose(fp);
        printf("File %s downloaded from Smain\n", fName);
      }
    }
    else if (strcmp(buffer, "rmfile") == 0 && (strstr(arg1, ".c") != NULL || strstr(arg1, ".txt") != NULL || strstr(arg1, ".pdf") != NULL))
    {

      // Send command to Smain
      send(server_sock, command, strlen(command), 0);

      memset(buffer, 0, BUFFER_SIZE);
      recv(server_sock, buffer, BUFFER_SIZE, 0);
      printf("%s\n", buffer);
    }
    else if (strcmp(buffer, "dtar") == 0 && (strcmp(arg1, ".c") == 0 || strcmp(arg1, ".txt") == 0 || strcmp(arg1, ".pdf") == 0))
    {

      // Send command to Smain
      send(server_sock, command, strlen(command), 0);

      memset(buffer, 0, BUFFER_SIZE);
      // recv(server_sock, buffer, BUFFER_SIZE, 0);
      printf("%s\n", buffer);

      // Receive the tar file
      remove("./download.tar");
      FILE *fp = fopen("./download.tar", "wb");
      if (fp == NULL)
      {
        perror("Error opening file");
        close(server_sock);
        return 0;
      }

      int n = 0;
      while ((n = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0)
      {
        ;
        printf("Getting file from server\n");
        if (strstr(buffer, "EOF") != NULL)
          break;

        printf("Received %d bytes, \n %s...........\n", n, buffer);
        if (fwrite(buffer, 1, n, fp) != n)
        {
          perror("Write to file failed");
          break;
        }
      }

      fclose(fp);
    }
    else if (strcmp(buffer, "display") == 0)
    {
      // Send command to Smain
      send(server_sock, command, strlen(command), 0);

      handle_display_command(server_sock);
    }
    else
    {
      printf("Invalid command run at client side\n");
    }
  }

  close(server_sock);
  return 0;
}
