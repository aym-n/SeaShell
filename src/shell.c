#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int cd(char **args);
int help(char **args);
int exitShell(char **args);

char *builtin[] = {"cd", "help", "exit"};

int (*builtin_func[])(char **) = {&cd, &help, &exitShell};
int num_builtin() { return sizeof(builtin) / sizeof(char *); }

int cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "USAGE: cd <name>");
    return 1;
  }

  if (chdir(args[1]) != 0)
    perror("seaSH");

  return 1;
}

int help(char **args)
{
  int i;
  printf("seaSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < num_builtin(); i++) printf("  %s\n", builtin[i]);

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int exitShell(char **args) { return 0; }

#define LINE_BUF_SIZE 1024
char *readLine(void)
{
  int bufsize = LINE_BUF_SIZE;
  int position = 0;
  char *buffer = malloc(bufsize * sizeof(char));

  int c;
  if (!buffer)
  {
    fprintf(stderr, "ERR0R: Line Buffer memory allocation failed");
    exit(EXIT_FAILURE);
  }

  while (1)
  {
    c = getchar();
    if (c == EOF || c == '\n')
    {
      buffer[position] = '\0';
      return buffer;
    }

    buffer[position] = c;
    position++;

    if (position >= bufsize)
    {
      bufsize += LINE_BUF_SIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        fprintf(stderr, "ERR0R: Line Buffer memory allocation failed");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a"
char **splitLine(char *line)
{
  int bufsize = TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(sizeof(char *) * bufsize);
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "ERROR: Token Buffer memory allocation failed");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= bufsize)
    {
      bufsize += TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize);
      if (!tokens)
      {
        fprintf(stderr, "ERROR: Token Buffer memory allocation failed");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int launch(char **args)
{
  pid_t pid, wid;
  int status;

  pid = fork();
  if (pid == 0)
  {
    if (execvp(args[0], args) == -1)
      perror("seaSH");

    exit(EXIT_FAILURE);
  }
  else if (pid < 0)
  {
    perror("seaSH");
  }
  else
  {
    do
    {
      wid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int executePipes(char **args, int num_pipes)
{
  int num_commands = num_pipes + 1;
  char ***commands = malloc(num_commands * sizeof(char **));

  int cmd_index = 0;
  int arg_index = 0;
  commands[cmd_index] = &args[arg_index];

  for (int i = 0; args[i] != NULL; i++)
  {
    if (strcmp(args[i], "|") == 0)
    {
      args[i] = NULL;
      cmd_index++;
      commands[cmd_index] = &args[i + 1];
    }
  }

  // Generate pipes
  int pipefd[2 * num_pipes];
  for (int i = 0; i < num_pipes; i++)
  {
    if (pipe(pipefd + 2 * i) == -1)
    {
      perror("seaSH");
      return 1;
    }
  }

  for (int i = 0; i < num_commands; i++)
  {
    pid_t pid = fork();
    if (pid == 0)
    {
      // Child process
      if (i > 0)
        dup2(pipefd[2 * (i - 1)], STDIN_FILENO);

      if (i < num_pipes)
        dup2(pipefd[2 * i + 1], STDOUT_FILENO);

      for (int j = 0; j < 2 * num_pipes; j++) close(pipefd[j]);

      if (execvp(commands[i][0], commands[i]) == -1)
      {
        perror("seaSH");
        exit(EXIT_FAILURE);
      }
    }
    else if (pid < 0)
    {
      perror("seaSH");
      return 1;
    }
  }

  // Parent process
  for (int i = 0; i < 2 * num_pipes; i++) close(pipefd[i]);
  for (int i = 0; i < num_commands; i++) wait(NULL);

  free(commands);

  return 1;
}

int execute(char **args)
{
  if (args[0] == NULL)
    return 1;

  for (int i = 0; i < num_builtin(); i++)
    if (strcmp(args[0], builtin[i]) == 0)
      return (*builtin_func[i])(args);

  int num_pipes = 0;
  for (int i = 0; args[i] != NULL; i++)
    if (strcmp(args[i], "|") == 0)
      num_pipes++;

  if (num_pipes == 0)
    return launch(args);
  return executePipes(args, num_pipes);
}

void loop()
{
  char *line;
  char **args;
  int status = 1;

  do
  {
    printf("seaSH ~> ");
    line = readLine();
    args = splitLine(line);
    status = execute(args);

    free(line);
    free(args);

  } while (status);
}

int main(int argc, char *argv[])
{
  loop();
  return EXIT_SUCCESS;
}
