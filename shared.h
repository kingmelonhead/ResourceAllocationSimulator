#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>

#define MAX_PROC 18
#define MAX_RESOURCES 20
#define MAX_INSTANCES 10
#define NUM_SEMS 2
#define SEM_RES_ACC 0	//semaphore for modifying values in resource
#define SEM_CLOCK_ACC 1 //semaphore for clock access
//finish states
#define NO 0 //not done
#define NORMAL 1
#define EARLY 2

void cleanup();
void signal_handler();
int get_shm();
int get_sem();
void log_string(char*);
void sem_wait(int);
void sem_signal(int);
int get_index(int);

typedef struct {

	bool shared;					//flag for stating if the resource is a shared resource or not

	int instances;					//number of instances of a given resource
	int instances_remaining;		//number of instances of a given resource that are still able to be allocated
	
	int requests[MAX_PROC];			//array that holds the # of the resource being requested [index would be the process #]
	int allocated[MAX_PROC];		//array that holds the # of resources allocated to process [index would be the process #]
	int releases[MAX_PROC];			//array that holds the # of the resource being released [index would be the process #]

} resource;

typedef struct {
	resource resources[MAX_RESOURCES];	//array of resource information containers

	int pids_running[MAX_PROC];		//array to hold PIDs of processes running
									//this pid list is also used to find the process number based off where pid is in array

	unsigned int seconds;			//holds the seconds component of the clock
	unsigned int nanoseconds;		//holds the nanoseconds component of the clock
	long double ms;

	long double cpu_time[MAX_PROC];
	long double wait_time[MAX_PROC];
	int sleep_status[MAX_PROC];
	int finished[MAX_PROC];
	bool waiting[MAX_PROC];
	int wants[MAX_PROC];

} shm_container;