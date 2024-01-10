/* CMPS 352, Fall 2023, Programming Assignment 5
FileName: scheduler-policarem2.c
Compile: cc -lpthread -o scheduler scheduler-policarem2.c 
Run: ./scheduler

Madison Policare
October 31st 2023

This is a c program designed to function as a shell with thread operations that are  
are manually scheduled, descheduled, and viewed by the user. 
*/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define RESET   "\033[0m"   
#define RED     "\033[31m"     
#define GREEN   "\033[32m"      
#define YELLOW  "\033[33m"    
#define MAGENTA "\033[35m"      
#define CYAN    "\033[36m"     

struct JOB {
   char *command[5]; 
   long submitTime; // domain = { positive long integer, seconds }
   long startTime;  // domain = { positive long integer, seconds}
   char cmdString[100];
};
typedef struct JOB Job;

struct NODE {
  void *item;
  struct NODE *next;
};

typedef struct NODE Node;

typedef struct LIST {
  Node *first;
  Node *last;
  int size;

  Node *cursor;
} List;

List *ListCreate() {
  List *list = (List *)malloc(sizeof(List));
  list->first = NULL;
  list->last = NULL;
  list->size = 0;

  list->cursor = NULL;
  return list;
}

Node *NodeCreate(void *item) {
  Node *node = (Node *)malloc(sizeof(Node));
  node->item = item;
  node->next = NULL;

  return node;
}

void tokenizeInput(char *input, char *args[]){
    char *token;
    int i = 0;
    while((token = strsep(&input, " \n")) != NULL){
        if(token[0] != '\0'){
            args[i] = token;
            i++;
        }
    }
    args[i] = NULL;
}

Job *JobCreate(){
    char cmdString[1024];
    char cmdStringb[1024];
    Job *job = (Job *)malloc(sizeof(Job));
    job->submitTime = time(NULL);
    printf(YELLOW"Enter start time: "RESET);
    fgets(cmdString, sizeof(cmdString), stdin);
    sscanf(cmdString, "%ld", &job->startTime);
    printf(YELLOW"Enter command: "RESET);
    fgets(job->cmdString, sizeof(job->cmdString), stdin);
    tokenizeInput(job->cmdString, job->command);
    return job; 
}

void ListFirst(List *this) { 
  this->cursor = this->first;
   }

void *ListGetNext(List *this) {
  void *result = NULL;
  if (this->cursor != NULL) {
    result = this->cursor->item;
    this->cursor = this->cursor->next;
  }

  return result;
}

void JobPrint(Job *job){
    for(int i=0; job->command[i] != NULL;i++){
        printf("%s\n", job->command[i]);
    }
}

void JobPrintList(List *this){
    if(this->size == 0){
        printf(GREEN"There are no jobs.\n"RESET);
    } else {
    ListFirst(this);
    Job *job = ListGetNext(this);
    while(job != NULL){
        JobPrint(job);
        job = ListGetNext(this);
        }
    }
}

void ListInsert(List *this, Job *job) {
    Node *node = NodeCreate(job);
    long execTime = job->submitTime + job->startTime;
    if(this->size == 0){
        this->first = node;
        this->last = node; 
    }else if(execTime < ((Job*)(this->first->item))->submitTime + ((Job*)(this->first->item))->startTime){
        node->next = this->first;
        this->first = node;
    }else if(execTime > ((Job*)(this->last->item))->submitTime + ((Job*)(this->last->item))->startTime){
        node->next = NULL;
        this->last->next = node;
        this->last = node;
    }else {
        Node *prev = this->first;
        Node *crnt = this->first->next;
        while(crnt != NULL && execTime > ((Job*)(crnt->item))->submitTime + ((Job*)(crnt->item))->startTime){
            prev = crnt;
            crnt = crnt->next;
        }
        prev->next = node;
        node->next = crnt; 
    }
    this->size++;
}

void *ListDelete(List *this) {
  void *result = NULL;
  if(this->size != 0){
  Node *temp = this->first;
  this->first = this->first->next; 
  result = temp->item;
  free(temp);
  this->size--;
  }
  return result;
}

void* worker_thread(void *arg){
    Job *job = (Job *) arg;
        if(strcmp(job->command[0],"chdir")==0 || strcmp(job->command[0],"cd")==0){
            chdir(job->command[1]);
        } else {
        int status;
        int cpid = fork();
        if(cpid < 0){
            printf("Fork failed.");
            exit(1);
        } else if (cpid == 0){
            execvp(job->command[0], job->command);
            printf("Error: %s\n", job->command[0]);
            exit(1);
        } else{
            waitpid(cpid,&status,0);
            pthread_exit(NULL);
            exit(1);
        }
    }
        free(job);
        return 0;
}

void job_is_ready(List *this, void *arg){
    int delay = 1;
    Job *job = (Job *) arg;
    long execTime = ((Job*)(this->first->item))->submitTime + ((Job*)(this->first->item))->startTime; 
    long currentTime = time(NULL);
        do {
            sleep(delay);
            currentTime = time(NULL);
        }
        while(execTime > currentTime);
}

void* server_thread(void *ptr){
    List *this = (List*) ptr;
    int delay = 1;
    while(1){
        if(this->size > 0){
            job_is_ready(this,ptr);
            pthread_t s_thread;
            void *argv = ListDelete(this);
            pthread_create(&s_thread, NULL, &worker_thread, (void*)argv);
        } else{
            sleep(delay);
        }
    }
}

int main(){
    List *Jobs = ListCreate();
    char cmdString[1024];
    pthread_t m_thread;
    pthread_create(&m_thread, NULL, &server_thread, (void*)Jobs);
    while(1){
        printf(CYAN"*+*+ To schedule a new job, enter '+'\n"RESET);
        printf(MAGENTA"+*+* To see a list of current jobs, enter 'p'\n"RESET);
        printf(GREEN"*+*+ To delete next job, enter '-'\n"RESET);
        fgets(cmdString,sizeof(cmdString),stdin);
        if(cmdString[0] == '+'){
            Job *jobs = JobCreate();
            ListInsert(Jobs, jobs);
        } else if(cmdString[0] == 'p'){
            JobPrintList(Jobs);
        } else if(cmdString[0] == '-'){
            ListDelete(Jobs);
        } else {
            printf(RED"Sir Clucks a Lot strikes again!\n"RESET);
        }
    }
    return 0;
}