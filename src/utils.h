#ifndef UTILS_H
#define UTILS_H

#include <glob.h>

#define LINE_BUF_SIZE 1024
#define TOK_BUFSIZE 64

char *readLine(void);
char **splitLine(char *line);
char **globTokens(char **tokens);

int launch(char **args);
int executePipes(char **args, int num_pipes);

#endif
