#include "shell.h"

#include <glob.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "utils.h"

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

int execute(char **args)
{
  if (args[0] == NULL)
    return 1;

  for (int i = 0; i < num_builtin(); i++)
    if (strcmp(args[0], builtin[i]) == 0)
      return (*builtin_func[i])(args);

  char **globbed_args = globTokens(args);

  int num_pipes = 0;
  for (int i = 0; globbed_args[i] != NULL; i++)
    if (strcmp(globbed_args[i], "|") == 0)
      num_pipes++;

  if (num_pipes == 0)
    return launch(globbed_args);
  return executePipes(globbed_args, num_pipes);
}
