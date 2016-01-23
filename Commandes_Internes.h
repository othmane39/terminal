#ifndef COMINTERN_H
#define COMINTERN_H
#include "Shell.h"


int echo(int argc, char** argv);

int date(int argc, char** argv);

int cd(int argc, char** argv);

int pwd(int argc, char** argv);

int history(int argc, char** argv);

int hostname(int argc, char** argv);

int kill_(int argc, char** argv);

int exit_(int argc, char** argv);

int simple_comm(Expression* e);

int extern_comm(Expression* e);

int create_shell(char * name);

void list_shell();

void remove_shell();

void command_shell(char ** argv);
#endif
