#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void child(int n, int k, int r);
void print_processes(int k);

int main(int argc, char const* argv[]) {
  int n, k, r;
  printf("n k r: ");
  scanf("%d %d %d", &n, &k, &r);

  child(n, k, r);

  return 0;
}

void child(int n, int k, int r) {
  int p2child[2], c2parent[2];
  pipe(p2child);
  pipe(c2parent);

  if (fork() == 0) {
    close(c2parent[0]);
    close(p2child[1]);

    bool keepPrinting = true;
    while (keepPrinting) {
      for (int i = 0; i < r; i++) {
        print_processes(k);
        if (i < r - 1) {
          sleep(n);
        }
      }
      // signal parent to prompt user for pid to kill
      write(c2parent[1], "\0", sizeof(char));

      // read pid from parent
      int kill_pid;
      read(p2child[0], &kill_pid, sizeof(kill_pid));

      switch (kill_pid) {
        case -2: {
          keepPrinting = false;
        } break;

        case -1: {
          printf("Not killing any process\n");
          continue;
        }

        // don't kill processes with pid 0, 1, or 2 (kernel processes)
        case 0:
        case 1:
        case 2: {
          printf("Cannot kill pid %d\n", kill_pid);
        } break;

        default: {
          printf("Killing process with pid: %d\n", kill_pid);
          if (kill(kill_pid, SIGKILL) == -1) {
            printf("Kill denied\n");
          }
        } break;
      }
      printf("\n");
    }
    printf("Stopped child loop\n");

    close(c2parent[1]);
    close(p2child[0]);

  } else {
    close(c2parent[1]);
    close(p2child[0]);

    while (true) {
      char buf;
      if (read(c2parent[0], &buf, sizeof(buf)) <= 0) {
        break;
      };

      int kill_pid;
      printf("Enter pid to kill: ");
      scanf("%d", &kill_pid);

      // send pid to child
      write(p2child[1], &kill_pid, sizeof(kill_pid));
    }
    printf("Stopped parent loop\n");

    close(c2parent[0]);
    close(p2child[1]);
  }

  wait(NULL);
}

void print_processes(int k) {
  k++;
  int fd[2];
  pipe(fd);

  if (fork() == 0) {
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    close(fd[1]);

    char* args[] = {"ps", "ouser,pid,%mem,time,cmd", "k-%mem", NULL};
    // char* args[] = {"ps", "ouser,pid,%mem,time,cmd", "k-pid", NULL};
    execvp("ps", args);
  }

  if (fork() == 0) {
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);

    int length = snprintf(NULL, 0, "%d", k);
    if (length < 0) {
      exit(1);
    }

    char k_str[length + 1];
    snprintf(k_str, length + 1, "%d", k);

    char* args[] = {"head", "-n", k_str, NULL};
    execvp("head", args);
  }

  wait(NULL);
  wait(NULL);
}