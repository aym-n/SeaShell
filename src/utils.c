#include "utils.h"

#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char *readLine(void)
{
  int bufsize = LINE_BUF_SIZE;
  int position = 0;
  char *buffer = malloc(bufsize * sizeof(char));

  if (!buffer)
  {
    fprintf(stderr, "ERROR: Line Buffer memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  int c;
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
        fprintf(stderr, "ERROR: Line Buffer memory allocation failed\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **splitLine(char *line)
{
  int bufsize = TOK_BUFSIZE;
  int position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *buffer = malloc(LINE_BUF_SIZE * sizeof(char));
  int buffer_index = 0;
  int in_quote = 0;  // 0 -> not in quote, 1 -> inside double quote, 2 -> inside single quote
  int escaped = 0;

  if (!tokens || !buffer)
  {
    fprintf(stderr, "ERROR: Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; line[i]; i++)
  {
    char current_char = line[i];
    if (escaped)
    {
      buffer[buffer_index++] = current_char;
      escaped = 0;
      continue;
    }
    if (current_char == '\\')
    {
      escaped = 1;
      continue;
    }

    if (current_char == '"' && in_quote != 2)
    {
      in_quote = (in_quote == 1) ? 0 : 1;
      continue;
    }

    if (current_char == '\'' && in_quote != 1)
    {
      in_quote = (in_quote == 2) ? 0 : 2;
      continue;
    }
    if (current_char == ' ' && !in_quote)
    {
      if (buffer_index > 0)
      {
        buffer[buffer_index] = '\0';
        tokens[position] = strdup(buffer);
        position++;
        buffer_index = 0;

        if (position >= bufsize)
        {
          bufsize += TOK_BUFSIZE;
          tokens = realloc(tokens, bufsize * sizeof(char *));
          if (!tokens)
          {
            fprintf(stderr, "ERROR: Memory allocation failed\n");
            exit(EXIT_FAILURE);
          }
        }
      }
    }
    else
    {
      buffer[buffer_index++] = current_char;
    }
  }

  if (buffer_index > 0)
  {
    buffer[buffer_index] = '\0';
    tokens[position] = strdup(buffer);
    position++;
  }

  tokens[position] = NULL;
  free(buffer);
  return tokens;
}

char **globTokens(char **tokens)
{
  glob_t glob_result;
  int glob_flags = GLOB_NOCHECK | GLOB_TILDE;
  int glob_index = 0;
  char **new_tokens = malloc(TOK_BUFSIZE * sizeof(char *));
  int new_tokens_size = TOK_BUFSIZE;

  for (int i = 0; tokens[i] != NULL; i++)
  {
    if (strpbrk(tokens[i], "*?[") != NULL)
    {
      glob(tokens[i], glob_flags, NULL, &glob_result);

      for (size_t j = 0; j < glob_result.gl_pathc; j++)
      {
        if (glob_index >= new_tokens_size)
        {
          new_tokens_size += TOK_BUFSIZE;
          new_tokens = realloc(new_tokens, new_tokens_size * sizeof(char *));
        }
        new_tokens[glob_index++] = strdup(glob_result.gl_pathv[j]);
      }
      globfree(&glob_result);
    }
    else
    {
      if (glob_index >= new_tokens_size)
      {
        new_tokens_size += TOK_BUFSIZE;
        new_tokens = realloc(new_tokens, new_tokens_size * sizeof(char *));
      }
      new_tokens[glob_index++] = strdup(tokens[i]);
    }
  }

  new_tokens[glob_index] = NULL;
  return new_tokens;
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
