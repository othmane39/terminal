#define _GNU_SOURCE
#include "Commandes_Internes.h"
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <readline/history.h>
#include <signal.h>
#include <sys/wait.h>

extern char** environ;

char* jours[] = {"dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"};
char* mois[] = {"janvier", "fevrier", "mars", "avril", "mai", "juin", "juillet", "aout", "septembre", "octobre",
		"novembre", "decembre"};
int p1[2]; //Pipe pour le remote shell
int p2[2];

struct remote_shell {
	char * name;
	int pid;
};


struct remote_shell shell[10];
int nb_shell = 0;

int echo(int argc, char** argv){ //vtest sans prise en compte de parametres

  for(int i=1; i<argc; i++)
    printf("%s ", argv[i]);
  
  printf("\n");
  return 0;
}

int date(int argc, char** argv){ //vtest sans prise en compte de parametres
  struct tm *time_s = NULL;
  time_t atime = time(NULL);
  time_s = localtime(&atime);
  printf("%s %d %s %d, %02d:%02d:%02d\n", jours[time_s->tm_wday], time_s->tm_mday, mois[time_s->tm_mon], 1900 + time_s->tm_year, time_s->tm_hour, time_s->tm_min, time_s->tm_sec); 

}

int cd(int argc, char** argv){
  int c;
  char wd[1024];
  if(argc <= 1)
    c = chdir(getenv("HOME"));
  else{
    c = chdir(argv[1]);
  }
  
  if(c == -1)
    printf("bash: cd: %s: aucun fichier ou dossier de ce type\n", argv[1]);
  
  return c;
}
  
int pwd(int argc, char** argv){

  char wd[1024];
  getcwd(wd, sizeof(wd));
  printf("%s\n", wd);
  
  return 0;
}

int history(int argc, char** argv){
  
  HIST_ENTRY** h_entry = history_list();
  HISTORY_STATE* h_state = history_get_history_state();
 
  int size;

  //if using_history boolean?
  if(argc <=1)
    for(int i=0; i<h_state->length; i++)
      printf("%d %s\n", i+1, h_entry[i]->line);//, (char*) (*h_entry+i)->data);
  else
    if(size = atoi(argv[1])){
      if(h_state->length-size <= 0)
	size = h_state->length;
      for(int i = h_state->length - size; i<h_state->length; i++)
	printf("%d %s\n", i+1, h_entry[i]->line);
    }

  return 0;
  
}

int hostname(int argc, char** argv){
  char name[100];
  if(argc<=1){
    if(gethostname(name, sizeof(name)) == 0){
      printf("%s\n", name);
      return 0;
    }
    
  }else{
    for(int i=1; i<argc; i++)
      if(argv[i][0] != '-'){ //hostname et non parametre
	strcpy(name, argv[i]);
	break;
      }
    if(sethostname(name, sizeof(name)) == 0)
      return 0;
  }
  return -1;
}

int kill_(int argc, char** argv){
  if(argc <= 1){
    printf("kill : utilisation : kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... ou kill -l [sigspec]\n");
    return -1;
  }

  int n_pid=0;
  int n_param=0;
  for(int i=1; i<argc; i++){
    if(argv[i][0] == '-')
      n_param++;
    else n_pid++;
  }
    
  int* pid = malloc(n_pid*sizeof(int));
  char* param = malloc(n_param*sizeof(int));
  
  int c_pid = 0, c_param = 0;
  for(int i=1; i<argc; i++){
    if(argv[i][0] == '-'){
      param[c_param] = argv[i][1];
      c_param++;
    }
    else{
      pid[c_pid] = atoi(argv[i]);
      c_pid++;
    }
  }

  if(c_pid+c_param != n_pid+n_param && n_pid+n_param != argc-1)
    printf("ERROR PARSING\n");


  if(n_param > 0 && param[0] == 'l'){ //Liste des signaux
    if(n_pid > 0){
      for(int i=0; i<n_pid; i++)
	printf("%d) %s\n", pid[i], strsignal(pid[i]));
      return 0;
    }
    else{   
      for(int i = 0 ; i < NSIG ; i++)
	printf("%d) %s\n",i, strsignal(i));
      return 0;
    }
  }
  
  int sig;
  if(n_param == 1){
    sig = param[0];
    for(int i = 0; i<n_pid; i++)
      if(kill(pid[i], sig) != 0)
	printf("error pid %d\n", pid[i]);
  }else if (n_param == 0){
    for(int i = 0; i<n_pid; i++)
      if(kill(pid[i], 15) != 0) //SIGTERM
	printf("error pid %d\n", pid[i]);
  }
    
    
}

int exit_(int argc, char** argv){
  if(argc > 2){
    printf("bash: exit: trop d'arguments\n");
    return -1;
  }else if(argc <= 1)
    exit(0);
  else if(argc == 2)
    exit(argv[1][0]);
}

int remote(int argc, char ** argv){
	if(argv[1] == NULL)
		printf("Argument manquant \n");
	else if(strcmp(argv[1],"add") == 0)
		if(argv[2] == NULL)
			printf("Donner un nom à la machine \n");
		else{
			create_shell(strdup(argv[2]));
		}
	else if (strcmp(argv[1],"remove") == 0)
		remove_shell();
	else if (strcmp(argv[1],"list") == 0)
		list_shell();
	else if (strcmp(argv[1],"all") == 0)
		printf("Commande pour tous \n");
	else
		command_shell(argv);
	return 0;
}

int simple_comm(Expression* e){
  int ret;
  if( strcmp(e->arguments[0], "echo") == 0)
    ret = echo(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "date") == 0)
    ret = date(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "pwd") == 0)
    ret = pwd(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "cd") == 0)
    ret = cd(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "history") == 0)
    ret = history(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "hostname") == 0)
    ret = hostname(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "kill") == 0)
    ret = kill_(LongueurListe(e->arguments), e->arguments);
  else if(strcmp(e->arguments[0], "exit") == 0)
    ret = exit_(LongueurListe(e->arguments), e->arguments);
  else if (strcmp(e->arguments[0], "remote") == 0)
	ret = remote(LongueurListe(e->arguments),e->arguments);
  else{
    // printf("%s : commande introuvable\n", e->arguments[0]);
    extern_comm(e);
  }

  return ret;
}

char* strcat_Tab(int length, char** elem){
  char* c = malloc(length*50*sizeof(char));
  for(int i=0; i<length; i++){
    strcat(c, elem[i]);
    strcat(c, " ");
  }
  return c;
}

int extern_comm(Expression* e){
  char* path = getenv("PATH");
  //you should use path to get all the path and all the command je pense!!
  //printf("%s\n", path);
  if(fork()==0){
    // execlvp();
    int length = LongueurListe(e->arguments);
    // printf("%s\n", strcat_Tab(length, e->arguments));
    // system(strcat_Tab(length, e->arguments));
    //char *command = "/bin/ls";
    
    
    char ** arguments = AjouterArg(e->arguments, "str");
    arguments[length] = NULL;
    // for(int i = 0; i<length; i++){
       //if(arguments[i] != NULL){
    //	printf("%s\n", arguments[i]);
	//printf("11111");
	// }
	// else printf("NULL\n");
    //}
    if(execvp(arguments[0], arguments) != 0)
      fprintf(stderr, "%s : commande introuvable\n", arguments[0]);
    //free(c);
    //execlp("/bin/ls", "/bin/ls");
    exit(0);
  }else wait(NULL);
}

int create_shell (char * name){
	pipe(p1);
	pipe(p2);
	int pid;
	pid = fork();
	if(pid == 0){ // Fils
		
		
		close(p1[0]);
		close(p2[1]);
		dup2(p1[1],1);
		dup2(p2[0],0);
		close(p1[1]);
		close(p2[0]);
		execlp("./Shell","./Shell","remote","NULL");
		
		
		
	}
	else {
		close(p1[1]);
		close(p2[0]);
		shell[nb_shell].pid = pid;
		shell[nb_shell].name = name;
		nb_shell++;
		return 0;
	}
}

void list_shell(){
	int i;
	for(i = 0; i < nb_shell;i++){
		printf("Shell '%s', pid : %d \n",shell[i].name,shell[i].pid);
	}
}

void remove_shell(){
	int i;
	for(i = nb_shell;i !=0;i--){
		kill(shell[i-1].pid,SIGKILL);
		nb_shell--;
	}
	memset(shell,0,sizeof(shell));
}

void command_shell(char ** argv){
	int i;
	char buffer;
	for(i = 0; i < nb_shell;i++){
		if(strcmp(shell[i].name,argv[1]) == 0){
			printf("-----%s-----\n",shell[i].name);
			printf("Remote shell en maintenance ... \n");
			/*
			write(p2[1], argv[2], sizeof(argv[2]));
			while(read(p1[0],&buffer,1) == 1){
				write(1,&buffer,1);
			}*/
		}
	}
	
	
}
