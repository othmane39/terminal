#include "Shell.h"
#include "Evaluation.h"
#include "Commandes_Internes.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

struct sigaction zombie_action = {
  .sa_handler = SIG_DFL,
  .sa_flags = SA_NOCLDWAIT
};


int
evaluer_expr(Expression *e)
{
  int ret;
  pid_t pid;
  
  switch(e->type){
  case SIMPLE:
    ret = simple_comm(e);
    break;
  case SEQUENCE:
    ret = evaluer_expr(e->gauche);
    ret = evaluer_expr(e->droite);
    break;
  case SEQUENCE_ET:
    ret = evaluer_expr(e->gauche);
    if(ret == 0) //si e->gauche VRAI
      ret = evaluer_expr(e->droite);

    break;
  case SEQUENCE_OU: //le || est un OU exclusif
    ret = evaluer_expr(e->gauche);
    if(ret != 0)
      ret = evaluer_expr(e->droite);
    break;
  case BG:
    pid = fork();
    if(pid == 0){
      ret = evaluer_expr(e->gauche);
      //sleep(20);
      exit(0);
    }
    else{
    printf("%d\n", pid);

    sigaction(SIGCHLD, &zombie_action, NULL);
    //wait(NULL);
    }
    break;
  case PIPE:
    ret = pipe_expr(e);
    break;
  case REDIRECTION_I:
    redirect_input(e);
    break;
  case REDIRECTION_O:
    redirect_output(e, O_CREAT | O_WRONLY | O_TRUNC, 1, -1);
    break;
  case REDIRECTION_A:
    redirect_output(e, O_CREAT | O_WRONLY | O_APPEND, 1, -1);
    break;
  case REDIRECTION_E:
    redirect_output(e, O_CREAT | O_WRONLY | O_TRUNC, 2, -1);
    break;
  case REDIRECTION_EO:
    redirect_output(e, O_CREAT | O_WRONLY | O_TRUNC, 1, 0);
    break;
  default:
    fprintf(stderr, "evaluation error\n");
  case VIDE:
    break;
  }
  
  return ret;
}

int pipe_expr(Expression* e){

  
  int fd[2];
  if(pipe(fd) == -1) {
    fprintf(stderr, "pipe error");
    return -1;
  }
   
  
  if(fork() == 0)        
    {
      close(1);          
      dup2(fd[1], 1); 
      close(fd[0]);  
      close(fd[1]);

      evaluer_expr(e->gauche);
     
      exit(1);
    }
  
  if(fork() == 0)        
    {
      close(0);          
      dup2(fd[0], 0);    
      close(fd[1]);  
      close(fd[0]);

      evaluer_expr(e->droite);

      exit(1);
    }
  
  close(fd[0]);
  close(fd[1]);
  wait(NULL);
  wait(NULL);
  return 0;
}

int redirect_input(Expression* e){
  int fd[2];
  
  if(pipe(fd) == -1){
    fprintf(stderr, "pipe error\n");
  }
  
  if(fork() == 0){
    int fd_f;
    char c;
   
    fd_f = open(e->arguments[0], O_RDONLY, 0444);
    close(0);
    while(read(fd_f, &c, 1) > 0){    
      write(fd[1], &c, 1);
    }
    close(fd[0]);
    close(fd[1]);
    close(fd_f);
    
    exit(1);
  }

  if(fork() == 0){

    close(0);
    dup2(fd[0], 0);
    close(fd[1]);
    close(fd[0]);
    
    evaluer_expr(e->gauche);
    exit(1);
  }

  close(fd[0]);
  close(fd[1]);
  wait(NULL);
  wait(NULL);
  
}

int redirect_output(Expression* e, int flags, int output, int EO){
  int fd[2];
  
  if(pipe(fd) == -1){
    fprintf(stderr, "pipe error\n");
  }
  
  if(fork() == 0){
    int fd_f;
    char c;
   
    fd_f = open(e->arguments[0], flags, 0666);
    
    close(fd[1]);
    while(read(fd[0], &c, 1) > 0){    
      write(fd_f, &c, 1);
    }
    close(fd[0]);
    close(fd[1]);
    close(fd_f);
    
    exit(1);
    
  }

  if(fork() == 0){
    if(EO == 0)
      close(1);
    close(fd[0]);
    dup2(fd[1], output);
    if(EO == 0)
      dup2(fd[1], 2);
    close(fd[0]);
    close(fd[1]);
    
    evaluer_expr(e->gauche);
    exit(1);
  }

  close(fd[0]);
  close(fd[1]);
  wait(NULL);
  wait(NULL);
  
}





