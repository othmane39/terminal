#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "Shell.h"

extern int evaluer_expr(Expression *e);

int pipe_expr(Expression* e);

int redirect_input(Expression* e);

int redirect_output(Expression* e, int flags, int output, int EO);



#endif
