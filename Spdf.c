#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <dirent.h>

#define BUFFER_SIZE 1024
#define SPDF_PORT 2488
// int SPDF_PORT = 2487;

// set port
// int setPort()
// {
//   srand(time(NULL));
//   int random_number = 1000 + rand() % 9000;

//   FILE *file = fopen("port_spdf.txt", "w");
//   if (file == NULL)
//   {
//     printf("Error opening file for writing.\n");
//     return 1;
//   }

//   SPDF_PORT = random_number;
//   fprintf(file, "%d\n", random_number);
//   fclose(file);

//   return 0;
// }

// create Socket
int createSocket()
{
  int server_sock;
  struct sockaddr_in serv_addr;
  socklen_t addr_len;

  // Creating socket
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("Could not create socket");
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(SPDF_PORT);

  // Binding
  if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("Bind failed");
    exit(1);
  }

  // Listening
  listen(server_sock, 5);
  printf("Server listening on port %d...\n", SPDF_PORT);
  return server_sock;
}

// create directory
void createDirectory(char *dest_path)
{
  char mkdir_cmd[BUFFER_SIZE];
  snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", dest_path);
  system(mkdir_cmd);
}

int extractFilename(const char *full_path, char *path, char *filename)
{
  char *last_slash = strrchr(full_path, '/');

  // Extract path
  strncpy(path, full_path, last_slash - full_path);
  path[last_slash - full_path] = '\0';

  // Extract file
  strcpy(filename, last_slash + 1);

  printf("Path: %s\n", path);
  printf("File: %s\n", filename);

  return 0;
}

// replacing strings
void replaceSubstring(char *str, const char *oldSubstring, const char *newSubstring)
{
  char buffer[1024]; // Buffer to store the modified string
  char *position;

  // Find the first occurrence of oldSubstring
  position = strstr(str, oldSubstring);

  if (position == NULL)
    return;

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

void createTextTarFile()
{
  system("find ./spdf -type f \\( -name '*.pdf' \\) -print0 | tar --null -cvf ./spdf/pdfFiles.tar --files-from=-");
}

// upload file request
void handle_upload_request(int client_sock, char *file_path)
{
  char buffer[BUFFER_SIZE];

  printf("file_path ::::: %s\n", file_path);
  FILE *fp = fopen(file_path, "wb");
  if (!fp)
  {
    perror("File open failed");
    return;
  }

  send(client_sock, "Ready to receive file", strlen("Ready to receive file"), 0);

  int n;
  while ((n = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0)
  {
    if (n < BUFFER_SIZE)
    {
      fwrite(buffer, 1, n, fp); // Write all the bytes received
      break;
    }
    fwrite(buffer, 1, n, fp);
  }

  fclose(fp);
  printf("File saved locally at %s\n", file_path);
}

// download file request
void handle_download_request(int client_sock, char *file_path)
{
  char buffer[BUFFER_SIZE];

  FILE *fp = fopen(file_path, "rb");
  if (!fp)
  {
    send(client_sock, "File not found", strlen("File not found"), 0);
    perror("File not found");
    return;
  }

  printf("sending smain that File exists \n");

  // Send acknowledgment to Smain
  send(client_sock, "File exists", strlen("File exists"), 0);

  printf("waiting from smain that Ready to receive file \n");

  // Wait for the "Ready to receive file" message
  memset(buffer, 0, BUFFER_SIZE);
  recv(client_sock, buffer, BUFFER_SIZE, 0);

  if (strcmp(buffer, "Ready to receive file") != 0)
  {
    printf("Client not ready to receive file\n");
    fclose(fp);
    return;
  }

  // Send the file data
  int n;
  while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
  {
    printf("Sending File to Smain\n");
    if (send(client_sock, buffer, n, 0) != n)
    {
      perror("Send error");
      break;
    }
  }

  send(client_sock, "EOF", strlen("EOF"), 0);

  fclose(fp);
  printf("File %s sent to Smain\n", file_path);
}

// remove file
void handleRemoveFile(int client_sock, char *file_path)
{
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

// Function to send a tar file to the requesting server
void sendTarFile(int client_sock, const char *filePath)
{

  FILE *fp = fopen(filePath, "rb");
  if (fp == NULL)
  {
    perror("Error opening file");
    return;
  }
  printf("command  ::::: %d\n", fp);
  printf("filePath  ::::: %s\n", filePath);

  char buffer[BUFFER_SIZE];

  // Send the file data
  int n;
  while ((n = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
  {

    printf("Sending File to Smain :: %d\n", n);
    if (send(client_sock, buffer, n, 0) != n)
    {
      perror("Send error");
      break;
    }
  }

  send(client_sock, "EOF", strlen("EOF"), 0);

  fclose(fp);
  printf("File %s sent to Smain\n", filePath);
}

void handle_display_request(int client_sock, char *dir_path)
{
  char buffer[BUFFER_SIZE];
  DIR *d;
  struct dirent *dir;

  replaceSubstring(dir_path, "~smain", "./stext");

  printf("dir_path : %s\n", dir_path);
  printf("stext path match : %d\n", strcmp(dir_path, "./stext/f1/f2"));

  d = opendir(dir_path);
  if (d)
  {
    while ((dir = readdir(d)) != NULL)
    {
      if (dir->d_type == DT_REG)
      {
        snprintf(buffer, sizeof(buffer), "%s\n", dir->d_name);
        printf("File names : %s\n", buffer);
        send(client_sock, buffer, strlen(buffer), 0);
      }
    }
    closedir(d);
  }
  else
  {
    send(client_sock, "Directory not found", strlen("Directory not found"), 0);
  }
}

void handle_request(int client_sock)
{
  char client_string[BUFFER_SIZE];
  char command[BUFFER_SIZE];
  char file_path[BUFFER_SIZE];
  char filename[BUFFER_SIZE];
  char dest_path[BUFFER_SIZE];

  memset(client_string, 0, BUFFER_SIZE);
  recv(client_sock, client_string, BUFFER_SIZE, 0);

  //* after replacing client_string will be "ufile x.txt ./spdf/f1/f2"
  replaceSubstring(client_string, "~smain", "./spdf");

  sscanf(client_string, "%s", command);

  if (strcmp(command, "ufile") == 0)
  {
    sscanf(client_string, "%s %s %s", command, filename, dest_path);

    printf("command::::: %s\n", command);
    printf("filename::::: %s\n", filename);

    // making destination folder ""./spdf/f1/f2"
    createDirectory(dest_path);

    // Save the file
    snprintf(file_path, sizeof(file_path), "%s/%s", dest_path, filename);

    handle_upload_request(client_sock, file_path);
  }
  else if (strcmp(command, "dfile") == 0)
  {
    sscanf(client_string, "%s %s", command, dest_path);

    printf("dest_path:: %s\n", dest_path);

    handle_download_request(client_sock, dest_path);
  }
  else if (strcmp(command, "rmfile") == 0)
  {
    sscanf(client_string, "%s %s", command, dest_path);

    printf("file_path for remove:: %s\n", dest_path);

    handleRemoveFile(client_sock, dest_path);
  }
  else if (strcmp(command, "dtar") == 0)
  {
    char fileType[BUFFER_SIZE];

    createTextTarFile();
    printf("Tar created\n");

    // Sending tar file to client that is located at "./spdf/pdfFiles.tar"
    sendTarFile(client_sock, "./spdf/pdfFiles.tar");
  }
  else if (strcmp(command, "display") == 0)
  {
    sscanf(client_string, "%s %s", command, dest_path);

    printf("dest_path : %s\n", dest_path);
    handle_display_request(client_sock, dest_path);
  }
  else
  {
    printf("Invalid command run at spdf\n");
  }
}

int main()
{
  // setPort();
  int server_sock = createSocket();

  int client_sock;
  while (1)
  {
    client_sock = accept(server_sock, NULL, NULL);
    if (client_sock < 0)
    {
      perror("Accept failed");
      continue;
    }

    handle_request(client_sock);

    close(client_sock);
  }

  close(server_sock);
  return 0;
}
