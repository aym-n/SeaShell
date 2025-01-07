#include "builtins.h"

#include <stdio.h>
#include <unistd.h>

char *builtin[] = {"cd", "help", "exit", "pwd", "env"};

int (*builtin_func[])(char **) = {&cd, &help, &exitShell, &pwd, &env};
int num_builtin() { return sizeof(builtin) / sizeof(char *); }

int cd(char **args)
{
  if (args[1] == NULL)
  {
    fprintf(stderr, "USAGE: cd <name>\n");
    return 1;
  }

  if (chdir(args[1]) != 0)
    perror("seaSH");

  return 1;
}

int help(char **args)
{
  printf("seaSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (int i = 0; i < num_builtin(); i++) printf("  %s\n", builtin[i]);

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int exitShell(char **args) { return 0; }

int pwd(char **args)
{
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
    printf("%s\n", cwd);
  else
    perror("seaSH");

  return 1;
}

int env(char **args)
{
  extern char **environ;
  for (char **env = environ; *env != NULL; env++) printf("%s\n", *env);

  return 1;
}
