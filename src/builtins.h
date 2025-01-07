#ifndef BUILTINS_H
#define BUILTINS_H

int cd(char **args);
int help(char **args);
int exitShell(char **args);
int pwd(char **args);
int env(char **args);
int num_builtin();

extern char *builtin[];
extern int (*builtin_func[])(char **);
int num_builtin();

#endif
