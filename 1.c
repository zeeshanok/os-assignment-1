#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void parent(int x, int n);
void child(int x, int n);

int main(int argc, char const* argv[]) {
  srand(time(NULL));

  int arr[] = {3, 15, 4, 6, 7, 17, 9, 2};
  int arr_n = sizeof(arr) / sizeof(arr[0]);
  int visited[arr_n];
  for (int i = 0; i < arr_n; i++)
    visited[i] = 0;

  int n;
  printf("n: ");
  scanf("%i", &n);

  int visited_count = 0;

  while (visited_count < arr_n) {
    int i = rand() % arr_n;
    if (!visited[i]) {
      visited_count++;
    }

    visited[i] = 1;
    parent(arr[i], n);
  }

  return 0;
}

void parent(int x, int n) {
  printf("Parent: %i\n", x);

  int fd[2];
  pipe(fd);

  pid_t pid = fork();
  if (pid == -1) {
    exit(1);
  }

  if (pid == 0) {
    // child
    int x;
    read(fd[0], &x, sizeof(x));

    close(fd[0]);
    child(x, n);

    exit(0);
  } else {
    // parent
    // printf("Parent: spawned child %i\n", pid);
    write(fd[1], &x, sizeof(x));
    close(fd[1]);

    int seconds = x % n;
    // printf("Parent: sent %i to child, waiting %is\n", x, seconds);
    sleep(seconds);

    waitpid(pid, NULL, 0);
  }
}

void child(int x, int n) {
  printf("Child: factors of %i\n", x);
  double s = sqrt(x);

  for (int i = 1; i <= s; i++) {
    if (x % i == 0) {
      printf("  %i  %i\n", i, x / i);
    }
  }

  int seconds = time(NULL) % n;
  // printf("Child: done, sleeping for %is\n", seconds);
  sleep(seconds);
}