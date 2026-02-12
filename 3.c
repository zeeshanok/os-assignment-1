#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char const* argv[]) {
  if (argc <= 2) {
    fprintf(stderr, "Usage: %s <pattern> <file1> [<file2> ...]\n", argv[0]);
    return EXIT_SUCCESS;
  }

  int fd[2];
  pipe(fd);

  if (fork() == 0) {
    dup2(fd[1], STDOUT_FILENO);
    close(fd[0]);
    close(fd[1]);

    char* args[argc + 2];
    memcpy(args + 2, argv + 1, (argc - 1) * sizeof(char*));

    args[0] = "grep";
    args[1] = "-nh";
    args[argc + 1] = NULL;

    execvp("grep", args);
  }

  wait(NULL);

  int fd1[2];
  pipe(fd1);

  if (fork() == 0) {
    dup2(fd[0], STDIN_FILENO);
    close(fd[0]);
    close(fd[1]);

    dup2(fd1[1], STDOUT_FILENO);
    close(fd1[0]);
    close(fd1[1]);

    char* args[] = {"cut", "-f1", "-d:", NULL};
    execvp("cut", args);
  }

  close(fd[0]);
  close(fd[1]);

  if (fork() == 0) {
    dup2(fd1[0], STDIN_FILENO);
    close(fd1[0]);
    close(fd1[1]);

    char* args[] = {"sort", "-un", NULL};
    execvp("sort", args);
  }

  close(fd1[0]);
  close(fd1[1]);

  while (wait(NULL) > 0);

  return EXIT_SUCCESS;
}
