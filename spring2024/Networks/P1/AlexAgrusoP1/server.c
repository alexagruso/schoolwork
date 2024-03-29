#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_STUDENTS 100
#define MAX_LENGTH 100

enum actions {
  ADD = 1,
  QUERY_ID = 2,
  QUERY_SCORE = 3,
  QUERY_ALL = 4,
  DELETE = 5,
  EXIT = 6,
};

typedef struct student {
  int id;
  char first_name[MAX_LENGTH];
  char last_name[MAX_LENGTH];
  int score;
} student;

student students[MAX_STUDENTS];
int student_count = 0;

void load_students(const char *file) {
  FILE *student_file = fopen(file, "r");

  int position = 0;
  char *line;
  size_t length;

  while (getline(&line, &length, student_file) != -1) {
    sscanf(line, "%d %s %s %d", &students[position].id,
           students[position].first_name, students[position].last_name,
           &students[position].score);

    position++;
  }

  fclose(student_file);

  student_count = position;
}

void write_students(const char *file) {
  FILE *student_file = fopen(file, "w+");

  for (int i = 0; i < student_count; i++) {
    fprintf(student_file, "%d %s %s %d\n", students[i].id,
            students[i].first_name, students[i].last_name, students[i].score);
  }
}

int main(int argc, char **args) {
  int welcome_socket = socket(PF_INET, SOCK_STREAM, 0);

  unsigned short port = (unsigned short)atoi(args[1]);

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

  int bind_status =
      bind(welcome_socket, (struct sockaddr *)&server_addr, sizeof server_addr);

  if (bind_status != 0) {
    printf("Bind Status: %d\n", bind_status);
    return EXIT_FAILURE;
  }

  int listen_status = listen(welcome_socket, 5);

  if (listen_status != 0) {
    printf("Listen Status: %d\n", listen_status);
    return EXIT_FAILURE;
  }

  printf("Successfully listening on port %d.\n", port);

  struct sockaddr_storage server_storage;
  socklen_t storage_size = sizeof server_storage;

  int new_socket =
      accept(welcome_socket, (struct sockaddr *)&server_storage, &storage_size);

  printf("Successfully connected to client.\n");

  uint32_t action;
  char message[64];

  load_students("students.txt");

  do {
    recv(new_socket, &action, sizeof action, 0);
    action = ntohl(action);

    if (action == ADD) {
      student new;
      recv(new_socket, &new, sizeof(student), 0);

      students[student_count] = new;
      student_count++;
    } else if (action == QUERY_ID) {
      int id;
      recv(new_socket, &id, sizeof(int), 0);

      student result;
      int found = 0;

      for (int i = 0; i < student_count; i++) {
        if (students[i].id == id) {
          result = students[i];
          found = 1;
        }
      }

      if (!found) {
        result.id = -1;
      }

      send(new_socket, &result, sizeof(student), 0);
    } else if (action == QUERY_SCORE) {
      int score;
      recv(new_socket, &score, sizeof(int), 0);

      student result[student_count];
      int found = 0;

      for (int i = 0; i < student_count; i++) {
        if (students[i].score == score) {
          result[found] = students[i];
          found++;
        }
      }

      send(new_socket, &found, sizeof(int), 0);

      for (int i = 0; i < found; i++) {
        send(new_socket, &result[i], sizeof(student), 0);
      }
    } else if (action == QUERY_ALL) {
      send(new_socket, &student_count, sizeof(int), 0);

      for (int i = 0; i < student_count; i++) {
        send(new_socket, &students[i], sizeof(student), 0);
      }
    } else if (action == DELETE) {
      int id;
      recv(new_socket, &id, sizeof(int), 0);

      student result;
      int found = 0;

      for (int i = 0; i < student_count; i++) {
        if (students[i].id == id) {
          result = students[i];
          found = 1;

          students[i] = students[student_count - 1];
          student_count--;
        }
      }

      if (!found) {
        result.id = -1;
      }

      send(new_socket, &result, sizeof(student), 0);
    }
  } while (action != EXIT);

  close(new_socket);
  close(welcome_socket);

  write_students("students.txt");

  printf("Exiting program.\n");

  return EXIT_SUCCESS;
}