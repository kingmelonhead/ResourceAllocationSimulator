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
#define NUM_SEMS 1

void cleanup();
void signal_handler();
void get_shm();
void get_sem();

typedef struct {

	bool shared;					//flag for stating if the resource is a shared resource or not

	int instances;					//number of instances of a given resource
	int instances_remaining;		//number of instances of a given resource that are still able to be allocated
	
	int pids_requesting[MAX_PROC];	//array to hold PIDs of child processes that request this resource
	int pids_allocated[MAX_PROC];	//array to hold PIDs of child processes that hold this resource
	int pids_released[MAX_PROC];	//array to hold PIDs of child processes that have released this resource

} resource_info;

typedef struct {
	resource_info[MAX_RESOURCES];	//array of resource information containers

	unsigned int seconds;			//holds the seconds component of the clock
	unsigned int nanoseconds;		//holds the nanoseconds component of the clock



} shm_container;