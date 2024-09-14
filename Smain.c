#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

// int SMAIN_PORT = 2486;
// int STEXT_PORT = 2486;
// int SPDF_PORT = 2486;

#define SMAIN_PORT 2486
#define STEXT_PORT 2487
#define SPDF_PORT 2488
#define BUFFER_SIZE 1024
#define LOCAL_HOST "127.0.0.1"

// reading port and store in variable
// int readPortNumber()
// {
//   FILE *fTxt = fopen("port_stext.txt", "r");
//   if (fTxt == NULL)
//   {
//     printf("Error opening fTxt for reading.\n");
//     return 1;
//   }

//   fscanf(fTxt, "%d", &STEXT_PORT);
//   fclose(fTxt);

//   FILE *fPdf = fopen("port_spdf.txt", "r");
//   if (fPdf == NULL)
//   {
//     printf("Error opening fPdf for reading.\n");
//     return 1;
//   }

//   fscanf(fPdf, "%d", &SPDF_PORT);
//   fclose(fPdf);

//   printf("STEXT_PORT Number is %d\n", STEXT_PORT);
//   printf("SPDF_PORT Number is %d\n", SPDF_PORT);
//   return 0;
// }

// int setPort()
// {
//   srand(time(NULL));
//   int random_number = 1000 + rand() % 9000;

//   FILE *file = fopen("port_smain.txt", "w");
//   if (file == NULL)
//   {
//     printf("Error opening file for writing.\n");
//     return 1;
//   }

//   SMAIN_PORT = random_number;
//   fprintf(file, "%d\n", random_number);
//   fclose(file);

//   return 0;
// }

void createDirectory(char *dest_path)
{
  char mkdir_cmd[BUFFER_SIZE];
  snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", dest_path);
  system(mkdir_cmd);
}

// create connection with smain and stext and Ip will be localhost
int createConnectionWithOtherServer(const char *server_ip, int server_port)
{
  int sock;
  struct sockaddr_in serv_addr;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Socket creation failed");
    return 0;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(server_port);

  if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
  {
    perror("Invalid address/ Address not supported");
    return 0;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("Connection to Stext server failed");
    return 0;
  }

  return sock;
}

void replaceSubstring(char *str, const char *oldSubstring, const char *newSubstring)
{
  char buffer[1024]; // Buffer to store the modified string
  char *position;

  // Find the first occurrence of oldSubstring
  position = strstr(str, oldSubstring);

  if (position == NULL)
  {
    // oldSubstring not found, no replacement needed
    return;
  }

  // Copy characters from the start of str up to the position of oldSubstring into buffer
  strncpy(buffer, str, position - str);
  buffer[position - str] = '\0';

  // Append newSubstring to buffer
  strcat(buffer, newSubstring);

  // Append the rest of the original string after oldSubstring
  strcat(buffer, position + strlen(oldSubstring));

  // Copy the modified string back to the original string
  strcpy(str, buffer);
}

void send_file_to_server(const char *server_ip, int server_port, int client_sock, const char *filename, const char *client_string)
{

  char buffer[BUFFER_SIZE];
  int sock = createConnectionWithOtherServer(server_ip, server_port);

  // Send the destination path first
  // char full_path[BUFFER_SIZE];
  // snprintf(full_path, sizeof(full_path), "./%s/%s", client_string, filename);
  // printf("full_path::: %d \n", full_path);
  send(sock, client_string, strlen(client_string), 0);

  memset(buffer, 0, BUFFER_SIZE);
  recv(sock, buffer, BUFFER_SIZE, 0);

  int n;
  if (strcmp(buffer, "Ready to receive file") == 0)
  {
    send(client_sock, "Ready to receive file", strlen("Ready to receive file"), 0);

    printf("filename %s\n", filename);
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
      perror("File open failed");
      close(sock);
      return;
    }

    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
    {
      if (send(sock, buffer, n, 0) != n)
      {
        perror("Send error");
        break;
      }
    }

    fclose(fp);
  }

  close(sock);
  memset(buffer, 0, BUFFER_SIZE);
  printf("File %s sent to server %s:%d\n", filename, server_ip, server_port);
}

void download_file_from_smain(int client_sock, char *client_string)
{
  char command[BUFFER_SIZE];
  char dest_path[BUFFER_SIZE];

  printf("client_string %s\n", client_string);

  sscanf(client_string, "%s %s", command, dest_path);

  replaceSubstring(dest_path, "~", "./");

  printf("dest_path after replace:: %s\n", dest_path);

  FILE *fp = fopen(dest_path, "rb");
  printf("file descriptor of %s: %d\n", dest_path, fp);
  if (!fp)
  {
    perror("File open failed");
    send(client_sock, "File not found", strlen("File not found"), 0);
    return;
  }

  // Send acknowledgment to client
  send(client_sock, "File exists", strlen("File exists"), 0);

  // Wait for the client to send "Ready to receive file"
  char buffer[BUFFER_SIZE];
  memset(buffer, 0, BUFFER_SIZE);
  recv(client_sock, buffer, BUFFER_SIZE, 0);
  if (strcmp(buffer, "Ready to receive file") != 0)
  {
    printf("Client not ready to receive file\n");
    fclose(fp);
    return;
  }

  int n;
  printf("buffer >> %s\n", buffer);
  while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
  {
    printf("buffer >>> %s\n", buffer);
    if (send(client_sock, buffer, n, 0) != n)
    {
      perror("Send error");
      break;
    }
    memset(buffer, 0, BUFFER_SIZE);
  }

  send(client_sock, "EOF", strlen("EOF"), 0);

  fclose(fp);
  printf("File %s sent to client\n", dest_path);
}

// send request to stext and spdf
void download_file_from_server(const char *server_ip, int server_port, int client_sock, const char *file_path)
{
  char buffer[BUFFER_SIZE];
  int sock = createConnectionWithOtherServer(server_ip, server_port);

  printf("Sock ------  %d\n", sock);

  printf("file_path -- %s\n", file_path);
  // Send the file_path to Stext
  send(sock, file_path, strlen(file_path), 0);

  // Receive acknowledgment from Stext
  memset(buffer, 0, BUFFER_SIZE);
  recv(sock, buffer, BUFFER_SIZE, 0);
  printf("buffer from stext -- %s\n", buffer);

  printf("strcmp(buffer, 'File exists') -- %d\n", strcmp(buffer, "File exists"));

  if (strcmp(buffer, "File exists") == 0)
  {
    // Sending client that file is exists
    send(client_sock, "File exists", strlen("File exists"), 0);

    // Wait for client acknowledgment
    memset(buffer, 0, BUFFER_SIZE);

    // waiting for receiving that client is ready
    recv(client_sock, buffer, BUFFER_SIZE, 0);

    if (strcmp(buffer, "Ready to receive file") == 0)
    {

      printf("sending stext that client is ready\n");
      send(sock, "Ready to receive file", strlen("Ready to receive file"), 0);
      printf("sent stext that client is ready");

      // Forward the file data directly to the client
      int n;

      printf("Waiting data from stext ");
      while ((n = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
      {
        printf("Receiving File to Smain :: %d\n", n);
        if (send(client_sock, buffer, n, 0) != n)
        {
          perror("Send error to client");
          break;
        }
        printf("Sending File to Client :: %d\n", n);
      }
    }
  }
  else
  {
    send(client_sock, "File not found", strlen("File not found"), 0);
  }

  close(sock);
}

// handle download request
void handle_dfile(int client_sock, char *client_string)
{
  if (strstr(client_string, ".c") != NULL)
  {
    // Process the request locally
    download_file_from_smain(client_sock, client_string);
  }
  else if (strstr(client_string, ".txt") != NULL)
  {
    printf("dest_path after replace for dfile :: %s\n", client_string);
    download_file_from_server(LOCAL_HOST, STEXT_PORT, client_sock, client_string);
  }
  else if (strstr(client_string, ".pdf") != NULL)
  {
    printf("dest_path after replace for dfile :: %s\n", client_string);
    download_file_from_server(LOCAL_HOST, SPDF_PORT, client_sock, client_string);
  }
  else
  {
    send(client_sock, "Invalid file type", strlen("Invalid file type"), 0);
  }
}

// remove file from smain server
void removeFromSmain(int client_sock, char *file_path)
{
  printf("delete =--- %s\n", file_path);
  if (remove(file_path) == 0)
  {
    printf("File %s deleted successfully\n", file_path);
    send(client_sock, "File deleted successfully", strlen("File deleted successfully"), 0);
  }
  else
  {
    perror("File deletion failed");
    send(client_sock, "File deletion failed", strlen("File deletion failed"), 0);
  }
}

// send req to stext and spdf for remove
void handelRemoveFromServer(const char *server_ip, int server_port, int client_sock, const char *client_string)
{
  char buffer[BUFFER_SIZE];
  int sock = createConnectionWithOtherServer(server_ip, server_port);

  printf("Sock ------  %d\n", sock);

  printf("file_path -- %s\n", client_string);
  // Send the file_path to Stext
  send(sock, client_string, strlen(client_string), 0);

  memset(buffer, 0, BUFFER_SIZE);

  // Receive message from stext or spdf that file is deleted or not
  recv(sock, buffer, BUFFER_SIZE, 0);

  // Send File is delete successfully or not to client
  send(client_sock, buffer, strlen(buffer), 0);

  return;
}

// handle initial remove request
void handleRemoveFile(int client_sock, char *client_string)
{
  char command[BUFFER_SIZE];
  char file_path[BUFFER_SIZE];
  char dest_path[BUFFER_SIZE];

  printf("strstr(client_string, ) %d", strstr(client_string, ".txt"));
  if (strstr(client_string, ".c") != NULL)
  {

    printf("client_string before ::::: %s\n", client_string);

    //* after replacing client_string will be "ufile x.txt /stext/f1/f2"
    replaceSubstring(client_string, "~", "./");

    printf("client_string after::::: %s\n", client_string);

    sscanf(client_string, "%s %s", command, dest_path);
    // snprintf(file_path, sizeof(file_path), ".%s", dest_path);

    printf("file_path for remove:: %s\n", dest_path);
    removeFromSmain(client_sock, dest_path);
  }
  else if (strstr(client_string, ".txt") != NULL)
  {
    handelRemoveFromServer(LOCAL_HOST, STEXT_PORT, client_sock, client_string);
  }
  else if (strstr(client_string, ".pdf") != NULL)
  {
    handelRemoveFromServer(LOCAL_HOST, SPDF_PORT, client_sock, client_string);
  }
  else
  {
    send(client_sock, "Invalid file type", strlen("Invalid file type"), 0);
  }
}

// handle tar file request
void requestTarFile(const char *server_ip, int server_port, int client_sock, const char *client_string)
{
  char buffer[BUFFER_SIZE];

  // connect smain to stext and spdf
  int sock = createConnectionWithOtherServer(server_ip, server_port);

  // Send the request to create tar file
  // snprintf(buffer, BUFFER_SIZE, "dtar %s", client_string);
  printf("client_string 6:: %s\n", client_string);
  send(sock, client_string, strlen(client_string), 0);

  int n;
  while ((n = recv(sock, buffer, BUFFER_SIZE, 0)) > 0)
  {
    printf("Receiving File to Smain :: %d\n", n);
    if (send(client_sock, buffer, n, 0) != n)
    {
      perror("Send error to client");
      break;
    }
    printf("Sending File to Client :: %d\n", n);
  }
}

// Function to send a tar file to the requesting server
void sendTarFile(int client_socket, const char *filePath)
{
  FILE *fp = fopen(filePath, "rb");
  if (fp == NULL)
  {
    perror("Error opening file");
    return;
  }

  // sending data to client
  char buffer[BUFFER_SIZE];
  int n = 0;
  while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
  {
    send(client_socket, buffer, n, 0);
  }
  send(client_socket, "EOF", strlen("EOF"), 0);

  fclose(fp);
}

// creating tar file in smain
void createCTarFile()
{
  system("find ./smain -type f \\( -name '*.c' \\) -print0 | tar --null -cvf ./smain/c.tar --files-from=-");
}

// handle initial dtar req
void handleDtarCommand(int client_sock, char *client_string)
{
  printf("client_string 1:: %s\n", client_string);
  if (strstr(client_string, ".c") != NULL)
  {
    printf("client_string:: %s\n", client_string);
    createCTarFile();
    sendTarFile(client_sock, "./smain/c.tar");
  }
  else if (strstr(client_string, ".txt") != NULL)
  {
    printf("client_string 3:: %s\n", client_string);
    requestTarFile(LOCAL_HOST, STEXT_PORT, client_sock, client_string);
  }
  else if (strstr(client_string, ".pdf") != NULL)
  {
    requestTarFile(LOCAL_HOST, SPDF_PORT, client_sock, client_string);
  }
  else
  {
    printf("Unsupported filetype\n");
  }
}

// handle display filename functionality
void handle_display_request(int client_sock, char *client_string)
{
  char buffer[BUFFER_SIZE];
  char dir_path[BUFFER_SIZE];

  //* Displaying files from Smain
  sscanf(client_string, "%s %s", buffer, dir_path);
  replaceSubstring(dir_path, "~", "./");

  // Finally, list files from Smain
  DIR *d;
  struct dirent *dir;
  d = opendir(dir_path);
  if (d)
  {
    send(client_sock, "-------------------\n C Filenames\n-------------------\n", strlen("-------------------\n C Filenames\n-------------------\n"), 0);
    while ((dir = readdir(d)) != NULL)
    {
      // DT_REG represent regular file type, for getting directory name use DT_DIR
      if (dir->d_type == DT_REG)
      {
        snprintf(buffer, sizeof(buffer), "%s\n", dir->d_name);
        send(client_sock, buffer, strlen(buffer), 0);
      }
    }
    closedir(d);
  }
  else
  {
    send(client_sock, "Directory not found", strlen("Directory not found"), 0);
  }
  // send(client_sock, "EOF", strlen("EOF"), 0); // End of file list

  //* Displaying files from Spdf
  int spdf_sock = createConnectionWithOtherServer(LOCAL_HOST, SPDF_PORT);

  if (spdf_sock > 0)
  {
    send(spdf_sock, client_string, strlen(client_string), 0);

    send(client_sock, "-------------------\n PDF Filenames\n-------------------\n", strlen("-------------------\n PDF Filenames\n-------------------\n"), 0);

    int n;
    while ((n = recv(spdf_sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
      printf("buffer >>> %s\n", buffer);
      if (send(client_sock, buffer, n, 0) != n)
      {
        perror("Send error");
        break;
      }
      memset(buffer, 0, BUFFER_SIZE);
    }

    close(spdf_sock);
  }

  //* Displaying files from Stext
  int stext_sock = createConnectionWithOtherServer(LOCAL_HOST, STEXT_PORT);
  if (stext_sock > 0)
  {
    send(stext_sock, client_string, strlen(client_string), 0);

    send(client_sock, "-------------------\n Text Filenames\n-------------------\n", strlen("-------------------\n Text Filenames\n-------------------\n"), 0);

    int n;
    while ((n = recv(stext_sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
      printf("buffer >>> %s\n", buffer);
      if (send(client_sock, buffer, n, 0) != n)
      {
        perror("Send error");
        break;
      }
      memset(buffer, 0, BUFFER_SIZE);
    }

    close(stext_sock);
  }
}

void prcClient(int client_sock)
{
  char client_string[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  char filename[BUFFER_SIZE];
  char dest_path[BUFFER_SIZE];

  printf("prcClient Called\n");
  while (1)
  {
    memset(client_string, 0, BUFFER_SIZE);

    if (recv(client_sock, client_string, BUFFER_SIZE, 0) <= 0)
    {
      perror("Receive failed or client disconnected");
      break;
    }

    printf("prcClient buffer => %s\n", client_string);

    // extract first argument frm the command
    memset(command, 0, BUFFER_SIZE);
    memset(filename, 0, BUFFER_SIZE);
    sscanf(client_string, "%s %s", command, filename);

    memset(dest_path, 0, BUFFER_SIZE);
    if (strcmp(command, "ufile") == 0)
    {
      sscanf(client_string, "%s %s %s", command, filename, dest_path);
      printf("Received upload command: %s %s\n", filename, dest_path);

      if (strstr(filename, ".c") != NULL)
      {
        char full_path[BUFFER_SIZE];
        memmove(dest_path, dest_path + 1, strlen(dest_path));
        snprintf(full_path, sizeof(full_path), "./%s/%s", dest_path, filename);

        // create a directory at destination place if not exists
        createDirectory(dest_path);

        FILE *fp = fopen(full_path, "wb");
        if (!fp)
        {
          perror("File open failed");
          break;
        }

        send(client_sock, "Ready to receive file", strlen("Ready to receive file"), 0);

        int n;
        char fileBuffer[BUFFER_SIZE];
        while ((n = recv(client_sock, fileBuffer, BUFFER_SIZE, 0)) > 0)
        {
          if (strstr(fileBuffer, "EOF") != NULL)
            break;

          printf("Received %d bytes, %s...........\n", n, fileBuffer);
          if (fwrite(fileBuffer, 1, n, fp) != n)
          {
            perror("Write to file failed");
            break;
          }
        }

        printf("after while %d bytes, \n %s...........\n", n, fileBuffer);

        if (n < 0)
        {
          perror("Receive error");
        }

        fclose(fp);
        printf("File %s saved locally at %s\n", filename, full_path);
      }
      else if (strstr(filename, ".txt") != NULL || strstr(filename, ".pdf") != NULL)
      {
        // Forward file to Stext server
        if (strstr(filename, ".txt") != NULL)
        {
          // replaceSubstring(dest_path, "stext");
          // printf("dest_path after replace :: %s", dest_path);
          send_file_to_server(LOCAL_HOST, STEXT_PORT, client_sock, filename, client_string);
        }
        else
        {
          // replaceSubstring(dest_path, "spdf");
          send_file_to_server(LOCAL_HOST, SPDF_PORT, client_sock, filename, client_string);
        }
      }
      else
      {
        send(client_sock, "Invalid file type", strlen("Invalid file type"), 0);
      }
    }
    else if (strcmp(command, "dfile") == 0)
    {
      handle_dfile(client_sock, client_string);
    }
    else if (strcmp(command, "rmfile") == 0)
    {
      handleRemoveFile(client_sock, client_string);
    }
    else if (strcmp(command, "dtar") == 0)
    {
      handleDtarCommand(client_sock, client_string);
    }
    else if (strcmp(command, "display") == 0)
    {
      handle_display_request(client_sock, client_string);
    }
    else
    {
      printf("INSIDE SMAIN INVALID : command %s \n", command);

      send(client_sock, "Invalid command from Smain", strlen("Invalid command from Smain"), 0);
    }
  }
}

int main()
{
  // Set Smain port
  // setPort();

  // Stext and Spdf port read
  // readPortNumber();

  int server_sock, client_sock;
  struct sockaddr_in serv_addr;
  socklen_t addr_len;

  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Could not create socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(SMAIN_PORT);

  if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("Bind failed");
    exit(1);
  }

  listen(server_sock, 5);
  printf("Smain server listening on port %d...\n", SMAIN_PORT);

  while (1)
  {
    client_sock = accept(server_sock, NULL, NULL);
    if (client_sock < 0)
    {
      perror("Accept failed");
      continue;
    }

    if (fork() == 0)
    {
      close(server_sock);
      prcClient(client_sock);
      close(client_sock);
      exit(0);
    }
    else
    {
      close(client_sock);
    }

    while (waitpid(-1, NULL, WNOHANG) > 0)
      ;
  }

  close(server_sock);
  return 0;
}
