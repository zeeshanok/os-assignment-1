#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CMD_ERROR(...)            \
  do {                            \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
  } while (0)

const char PROMPT[] = "station-controller$ ";
const char DELIMITER[] = (" \n");
const int INITIAL_FREQUENCY = 800;  // MHz

volatile sig_atomic_t frequency = INITIAL_FREQUENCY;
volatile sig_atomic_t editing = false;

typedef enum {
  CMD_QUIT,
  CMD_COMPLETE,
  CMD_UNKNOWN,
} CommandResult;

typedef struct {
  char* name;
  char** argv;
  int argc;
  int arg_capacity;
} Command;

Command* command_init(char* name);
void command_free(Command* cmd);
void command_append_arg(Command* cmd, char* arg);

CommandResult command_run(Command* cmd);

const char message[] = "\n[WARNING] Carrier Interrupt Signal Received\n";

void handle_sigint(int sig) {
  write(STDERR_FILENO, message, sizeof(message) - 1);
  frequency = INITIAL_FREQUENCY;
  if (editing) {
    write(STDERR_FILENO, PROMPT, sizeof(PROMPT) - 1);
  }
}

int main(int argc, char const* argv[]) {
  signal(SIGINT, handle_sigint);

  bool running = true;

  while (running) {
    char* context;
    printf("%s", PROMPT);

    char* line = NULL;
    size_t n = 0;

    editing = true;
    ssize_t size = getline(&line, &n, stdin);
    editing = false;

    char* name = strtok_r(line, DELIMITER, &context);
    if (name == NULL) {
      free(line);
      continue;
    }

    Command* cmd = command_init(name);

    char* token;
    for (int arg = 0; (token = strtok_r(NULL, DELIMITER, &context)) != NULL; arg++) {
      command_append_arg(cmd, token);
    }

    CommandResult result = command_run(cmd);

    command_free(cmd);
    if (line != NULL) free(line);

    switch (result) {
      case CMD_QUIT: {
        running = false;
      } break;

      case CMD_UNKNOWN: {
        CMD_ERROR("%s: command not found", cmd->name);
      } break;
    }
  }
  return EXIT_SUCCESS;
}

Command* command_init(char* name) {
  Command* cmd = malloc(sizeof(Command));

  cmd->name = name;
  cmd->arg_capacity = 2;
  cmd->argv = malloc(sizeof(char*) * cmd->arg_capacity);
  cmd->argc = 1;
  cmd->argv[0] = name;

  return cmd;
}

void command_free(Command* cmd) {
  free(cmd->argv);
  free(cmd);
}

void command_append_arg(Command* cmd, char* arg) {
  if (cmd->argc >= cmd->arg_capacity) {
    cmd->arg_capacity *= 2;
    cmd->argv = realloc(cmd->argv, sizeof(char*) * cmd->arg_capacity);
  }

  cmd->argv[cmd->argc] = arg;
  cmd->argc++;
}

void create_child_command(Command* cmd) {
  if (fork() == 0) {
    // move child to new process group so that it doesn't receive SIGINT from parent
    setpgid(0, 0);

    execvp(cmd->name, cmd->argv);
    perror("command failed");
    exit(EXIT_FAILURE);
  }

  wait(NULL);
}

CommandResult command_run_external(Command* cmd) {
  if (strcmp(cmd->name, "top") == 0) {
    Command* top_cmd = command_init("ps");
    command_append_arg(top_cmd, "aux");
    command_append_arg(top_cmd, NULL);

    create_child_command(top_cmd);

    command_free(top_cmd);

    return CMD_COMPLETE;
  }

  if (strcmp(cmd->name, "ping") == 0) {
    if (cmd->argc < 2) {
      CMD_ERROR("Usage: ping <host>");
      return CMD_COMPLETE;
    }

    Command* ping_cmd = command_init("ping");
    command_append_arg(ping_cmd, "-c4");
    command_append_arg(ping_cmd, cmd->argv[1]);
    command_append_arg(ping_cmd, NULL);

    create_child_command(ping_cmd);

    command_free(ping_cmd);

    return CMD_COMPLETE;
  }

  return CMD_UNKNOWN;
}

CommandResult command_run(Command* cmd) {
  if (strcmp(cmd->name, "quit") == 0 || strcmp(cmd->name, "exit") == 0) {
    return CMD_QUIT;
  }

  if (strcmp(cmd->name, "get_freq") == 0) {
    printf("%d MHz\n", frequency);
    return CMD_COMPLETE;
  }

  if (strcmp(cmd->name, "set_freq") == 0) {
    if (cmd->argc < 2) {
      CMD_ERROR("Usage: set_freq <frequency>");
      return CMD_COMPLETE;
    }

    int new_freq = atoi(cmd->argv[1]);
    if (new_freq <= 0) {
      CMD_ERROR("Invalid frequency %s", cmd->argv[1]);
      return CMD_COMPLETE;
    }

    frequency = new_freq;
    printf("Frequency set to %d MHz\n", new_freq);

    return CMD_COMPLETE;
  }

  return command_run_external(cmd);
}