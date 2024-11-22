#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "net.h"

void send_error(int nfd, const int status, const char *message)
{
   char buf[10];
   sprintf(buf, "%d", status);
   send_message(nfd, "GET", "HTTP/1.0", buf, strlen(message), message);
}

void send_message(int nfd, const char *type, const char *version, const char *version2, const int content_length, const char *content)
{
   dprintf(nfd, "%s %s\r\n", version, version2);
   dprintf(nfd, "Content-Type: text/html\r\n");
   dprintf(nfd, "Content-Length: %d\r\n", content_length);
   dprintf(nfd, "\r\n");
   if (strcmp(type, "GET") == 0)
   {
      if (write(nfd, content, content_length) == -1)
      {
         perror("write");
      }
   }
}

void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      close(nfd);
      return;
   }

   while ((num = getline(&line, &size, network)) >= 0)
   {
      printf("Message received from client: %s\n", line);
      char type[10];
      char raw_file[100];
      char file[100];
      char version[10];
      char *version2;
      struct stat file_stat;

      int parse = sscanf(line, "%s %s %s", type, raw_file, version);
      strcpy(file, &raw_file[1]);
      if (strcmp(raw_file, "/..") == 0)
      {
         send_error(nfd, 403, "<html><body><h1>Permission Denied</h1></body></html>");
      }
      else if (strcmp(type, "GET") != 0 && strcmp(type, "HEAD") != 0)
      {
         send_error(nfd, 400, "<html><body><h1>Bad Request</h1></body></html>");
      }
      else if (stat(file, &file_stat) != 0)
      {
         send_error(nfd, 404, "<html><body><h1>Not Found</h1></body></html>");
      }
      else
      {
         int content_length = file_stat.st_size;
         char content[content_length];
         FILE *fp = fopen(file, "r");
         fread(content, sizeof(char), content_length, fp);

         if (strcmp(type, "GET") == 0 || strcmp(type, "HEAD") == 0)
         {
            version2 = "200 OK";
            send_message(nfd, type, version, version2, content_length, content);
         }
      }
   }

   free(line);
   fclose(network);
}

void run_service(int fd)
{
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
         printf("Connection established\n");
         handle_request(nfd);
         printf("Connection closed\n");
         break;
      }
   }
}

int main(int argc, char const *argv[])
{
   int port = atoi(argv[1]);
   int fd = create_service(port);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", port);
   run_service(fd);
   close(fd);

   return 0;
}
