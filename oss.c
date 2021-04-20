#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int proc_count = 0;
int num_proc = 0;
char buffer[200];
int line_count = 0;
int fork_count = 0;

//stats
int immediately = 0;
int after_waiting = 0;
int killed_by_deadlock = 0;
int detection_run = 0;


unsigned long next_fork_sec = 0;
unsigned long next_fork_nano = 0;

unsigned long nano_change;

void child_handler(int);
void initialize_sems();
void initialize_shm();
void handle_releases();
void handle_allocations();
void check_deadlock();
bool time_to_fork();
void normalize_fork();
void normalize_clock();
void fork_proc();
void find_and_remove(int);
void check_finished();
void print_pid_table();
void print_res();
void report();
void print_allocation();

int main() {

	//signal handlers
	signal(SIGKILL, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);

	//signal handling specifically for the death of children processes
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = child_handler;
	sigaction(SIGCHLD, &sa, NULL);

	//open logfile
	file_ptr = fopen("logfile.log", "a");
	log_string("DEBUG STUFF:\n\n");
	log_string("created the logfile!\n");

	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
	}
	log_string("semaphores createed!\n");

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
	}
	log_string("shared memory created!\n");

	initialize_sems();
	log_string("semaphores initialized!\n");

	initialize_shm();
	log_string("shared memory initialized!\n");

	log_string("\nResources in their initialized state:\n\n");
	print_res();

	alarm(5);
	log_string("\nalarm for 5 real clock seconds has been set\n");

	log_string("\nLogical clock and main oss.c loop starting..\n\n");
	//main loop
	while (1) {

		sem_wait(SEM_RES_ACC);

		if (num_proc == 0) {
			//if no processes then fork
			fork_proc();
		}
		else if (num_proc < MAX_PROC) {
			//if there is room in process table
			if (time_to_fork()) {
				//if enough time has passed then fork
				fork_proc();
			}
		}

		if (num_proc > 0) {
			check_finished();
			handle_releases();
			handle_allocations();
			check_deadlock();
			detection_run++;
			print_pid_table();
			//print_allocation();
		}
		sem_signal(SEM_RES_ACC);

		sem_wait(SEM_CLOCK_ACC);
		//INCREMENT LOGICAL CLOCK
		nano_change = 1 + ( rand() % 100000000);
		shm_ptr->nanoseconds += nano_change;
		normalize_clock();
		sem_signal(SEM_CLOCK_ACC);

		//usleep(10000);
	
	}

	return 0;
}

void print_allocation() {
	fputs("\n\nResource allocation chart at end of runtime:\n\n", file_ptr);
	int i, j;
	fputs("    P0  P1  P2  P3  P4  P5  P6  P7  P8  P9  P10  P11  P12  P13  P14  P15  P16  P17  P18  P19\n", file_ptr);
	for (i = 0; i < MAX_RESOURCES; i++) {
		sprintf(buffer, "R%d   ", i);
		fputs(buffer, file_ptr);
		for (j = 0; j < MAX_PROC; j++) {
			sprintf(buffer, "%d    ", shm_ptr->resources[i].allocated[j]);
			fputs(buffer, file_ptr);
		}
		fputs("\n", file_ptr);
	}

}

void report() {
	double temp;
	sprintf(buffer, "\n\nREPORT:\n\n");
	fputs(buffer, file_ptr);
	sprintf(buffer, "Instantly granted resources: %d times\n", immediately);
	fputs(buffer, file_ptr);
	sprintf(buffer, "Processes were granted resources after waiting %d times\n", after_waiting);
	fputs(buffer, file_ptr);
	sprintf(buffer, "Deadlock detection ran %d times\n", detection_run);
	fputs(buffer, file_ptr);
	fputs("NOTE: The count for deadlock detection runs is obtained counting how many times the actual method runs\n", file_ptr);
	fputs("It may look high, this is because it quickly just scans for deadlock every loop of the main loop\n", file_ptr);
	sprintf(buffer, "%d processes were killed by deadlock\n", killed_by_deadlock);
	fputs(buffer, file_ptr);
	temp = (double)killed_by_deadlock / (double)detection_run;
	temp *= 100;
	sprintf(buffer, "%.8f percent of time deadlock detection was run, deadlock was detected and a process had to be killed\n", temp);
	fputs(buffer, file_ptr);
	fputs("0 percent indicates that there was never deadlock in the system (it was never in an unsafe state)\n", file_ptr);

}

void cleanup() {
	fputs("\n\nUnless specified otherwise, OSS ending due to reaching 5 real clock seconds\n", file_ptr);
	report();
	print_allocation();
	if (file_ptr != NULL) {
		fclose(file_ptr);
	}
	system("killall proc");
	sleep(5);
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);
	semctl(sem_id, 0, IPC_RMID, NULL);
	exit(0);
}



void log_string(char* s) {
	line_count++;
	if (line_count <= 10000) {
		//log the passed string to the log file
		fputs(s, file_ptr);
	}
	else {
		printf("exiting because line count got over 10,000\n");
		fputs("OSS is terminating: 10,000 lines in logfile met\n\n", file_ptr);
		cleanup();
	}

}

bool time_to_fork() {
	//find if clock has passed the fork time
	if (next_fork_sec == shm_ptr->seconds) {
		if (next_fork_nano <= shm_ptr->nanoseconds) {
			//passed time
			return true;
		}
	} else if (next_fork_sec < shm_ptr->seconds) {
		//passed time
		return true;
	}
	//not passed time
	return false;
}

void check_finished() {
	//printf("checking for finished processes...\n");
	int i, j;
	for (i = 0; i < MAX_PROC; i++) {
		//goes through all processes
		if (shm_ptr->finished[i] == EARLY) {
			//here if early termination
			sprintf(buffer, "P%d decided to terminate early at time %d : %d  and will release its resources\n",i, shm_ptr->seconds, shm_ptr->nanoseconds);
			log_string(buffer);
			shm_ptr->pids_running[i] = 0;
			for (j = 0; j < MAX_RESOURCES; j++) {
				//goes through all the resources
				if (shm_ptr->resources[j].allocated[i] > 0) {
					//here is early terminated process has stuff allocated
				
					//gives the resources back
					shm_ptr->resources[j].instances_remaining += shm_ptr->resources[j].allocated[i];

					
				}
			}
			
		}
	}
	//printf("done checking for finished processes!\n");
}
void handle_releases() {
	//printf("checking for resource release requests...\n");
	int i, j;
	for (i = 0; i < MAX_PROC; i++) {
		//goes through all the processes

		for (j = 0; j < MAX_RESOURCES; j++) {
			//goes through all resoures

			if (shm_ptr->resources[j].releases[i] > 0) {

				//here if a process wants to release a resource
				sprintf(buffer, "P%d has released R%d at time %d : %d \n", i, j, shm_ptr->seconds, shm_ptr->nanoseconds);
				log_string(buffer);

				//gives the resources back
				shm_ptr->resources[j].instances_remaining += shm_ptr->resources[j].allocated[i];
				shm_ptr->resources[j].releases[i] = 0;
				shm_ptr->resources[j].allocated[i] -= shm_ptr->resources[j].allocated[i];
				shm_ptr->waiting[i] = false;
				num_proc--;
			}
		}
	}
	//printf("done checking for resource release requests!\n");
}
void handle_allocations() {
	//printf("entering allocation algorithm...\n");
	int i, j;
	for (i = 0; i < MAX_PROC; i++) {
		//goes through all the processes

		if (shm_ptr->sleep_status[i] == 0) {

			for (j = 0; j < MAX_RESOURCES; j++) {
				//goes through all resoures

				if (shm_ptr->resources[j].requests[i] > 0) {

					//here if a process wants to obtain a resource
					sprintf(buffer, "P%d wants access to R%d (instances: %d) \n", i, j, shm_ptr->resources[j].requests[i]);
					log_string(buffer);

					//sees if there is enough of resource 
					if (shm_ptr->resources[j].requests[i] <= shm_ptr->resources[j].instances_remaining) {

						//instances avaliable
						shm_ptr->resources[j].instances_remaining -= shm_ptr->resources[j].requests[i];
						shm_ptr->resources[j].allocated[i] = shm_ptr->resources[j].requests[i];
						shm_ptr->resources[j].requests[i] = 0;
						shm_ptr->sleep_status[i] = 0;
						shm_ptr->waiting[i] = false;
						immediately++;
						sprintf(buffer, "P%d has recieved access to R%d Time %d : %d\n", i, j, shm_ptr->seconds, shm_ptr->nanoseconds);
						log_string(buffer);
					}
					else {

						//not avaliable
						sprintf(buffer, "P%d is going to sleep at time %d : %d . Process requested R%d but there is not enough instances\n", i, shm_ptr->seconds, shm_ptr->nanoseconds, j);
						log_string(buffer);
						shm_ptr->wants[i] = j;
						shm_ptr->sleep_status[i] = 1;
					}

				}
			}
		}
	}
	//printf("allocation algorithm done!\n");
}

void print_res() {
	int i, j;
	for (i = 0; i < MAX_RESOURCES; i++) {
		sprintf(buffer, "R%d   Instances: %d   Instances Free: %d\n", i, shm_ptr->resources[i].instances, shm_ptr->resources[i].instances_remaining);
		log_string(buffer);
	}
}

void check_deadlock() {
	int i, j;
	int res;
	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->sleep_status[i] == 1) {
			//if here then an asleep process was found
			res = shm_ptr->wants[i];
			while (1) {
				for (j = 0; j < MAX_PROC; j++) {
					
					//goes through processes looking for sleeping processes that are holding the resource that the process wants
					//the processes are asleep and likely wont release the needed resorce for a long time if ever
					//so in this situation the processes are just killed
					if (j != i) {
						//keeps process from killing itself or trying to anyway

						if (shm_ptr->resources[res].allocated[j] > 0 && shm_ptr->sleep_status[j] == 1) {

							//trouble process found
							sprintf(buffer, "Deadlock Detected: P%d is being killed to free R%d for P%d\n", j, res, i);
							log_string(buffer);
							killed_by_deadlock++;
							//put resources back into the pool
							shm_ptr->resources[res].instances_remaining += shm_ptr->resources[res].allocated[j];

							shm_ptr->resources[res].releases[j] = 0;
							shm_ptr->resources[res].allocated[j] = 0;
							shm_ptr->resources[res].requests[j] = 0;

							//clear process from pid table
							shm_ptr->pids_running[j] = 0;

							shm_ptr->finished[j] = KILLED_BY_OSS;
							num_proc--;

							//see if the stalled process can now claim resource
							if (shm_ptr->resources[res].requests[i] <= shm_ptr->resources[res].instances_remaining) {
								//grant resource	
								shm_ptr->resources[res].instances_remaining -= shm_ptr->resources[res].requests[i];
								shm_ptr->resources[res].allocated[i] += shm_ptr->resources[res].requests[i];
								shm_ptr->resources[res].requests[i] = 0;

								sprintf(buffer, "P%d has been granted access to R%d at time %d : %d \n", i, res, shm_ptr->seconds, shm_ptr->nanoseconds);
								log_string(buffer);
								after_waiting++;

								//un sleep and un wait the process
								shm_ptr->sleep_status[i] = 0;
								shm_ptr->waiting[i] = false;
								return;
							}
						}
					}
				}
				break;
			}
		}
	}

}


void print_pid_table() {
	printf("[ ");
	int i;
	for (i = 0; i < MAX_PROC; i++) {
		printf("%d", shm_ptr->pids_running[i]);
		if (i != (MAX_PROC - 1)) {
			printf(", ");
		}
	}
	printf("]\n");
}

void fork_proc() {

	if (fork_count >= 40) {
		printf("exiting becasue 40 chilren have been forked!\n");
		log_string("OSS exiting becasuse 40 children have been forked\n");
		cleanup();
	}
	int i;
	int pid;
	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->pids_running[i] == 0) {
			num_proc++;
			fork_count++;
			pid = fork();

			if (pid != 0) {
				sprintf(buffer, "P%d being forked (PID:%d) at time %d : %d \n", i, pid, shm_ptr->seconds, shm_ptr->nanoseconds);
				log_string(buffer);
				shm_ptr->pids_running[i] = pid;
				//generate new fork time
				log_string("generating next fork time\n");
				next_fork_nano = shm_ptr->nanoseconds + (rand() % 500000000);
				normalize_fork();
				return;
			}
			else {
				execl("./proc", "./proc", (char*)0);
			}
		}

	}
}

void signal_handler() {
	cleanup();
}

void child_handler(int sig) {
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {
		//doesnt really do anything
		//just prevents zombies
	}

}

void initialize_sems() {
	//initializes the 2 semaphores i use 
	semctl(sem_id, SEM_RES_ACC, SETVAL, 1);
	semctl(sem_id, SEM_CLOCK_ACC, SETVAL, 1);
}

void initialize_shm() {
	//initializes the shm container
	int i, j;
	for (i = 0; i < MAX_RESOURCES; i++) {
		if ((i + 1) % 10 == 0) {
			shm_ptr->resources[i].shared = true;
			sprintf(buffer, "R%d created as a sharable resource\n", i);
			log_string(buffer);
		}
		else {
			shm_ptr->resources[i].shared = false;
			sprintf(buffer, "R%d created as a non sharable resource\n", i);
			log_string(buffer);
		}
		shm_ptr->resources[i].instances = 1 + (rand() % MAX_INSTANCES);
		shm_ptr->resources[i].instances_remaining = shm_ptr->resources[i].instances;
		sprintf(buffer, "%d instances created for R%d\n", shm_ptr->resources[i].instances, i);
		log_string(buffer);
		for (j = 0; j < MAX_PROC; j++) {
			shm_ptr->resources[i].requests[j] = 0;
			shm_ptr->resources[i].allocated[j] = 0;
			shm_ptr->resources[i].releases[j] = 0;
		}
	}
	for (i = 0; i < MAX_PROC; i++) {
		//initialzie PID table
		shm_ptr->pids_running[i] = 0;
	}
	//initialize the logical clock
	shm_ptr->seconds = 0;
	shm_ptr->nanoseconds = 0;
}

void find_and_remove(int pid) {
	int i, j;

	
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(resource) * MAX_PROC) + sizeof(shm_container), IPC_CREAT | 0666)) == -1) {
		perror("oss.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory


	if ((shm_ptr = (shm_container*)shmat(shm_id, 0, 0)) == (shm_container*)-1) {
		perror("oss.c: shmat failed:");
		return -1;
	}

	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok(".", 'a');
	//gets chared memory
	if ((sem_id = semget(key, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
		perror("oss.c: semget failed:");
		return -1;
	}
	return 0;
}


void sem_wait(int sem_id) {
	//used to wait and decrement semaphore
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = -1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void sem_signal(int sem_id) {
	//used to increment semaphore
	struct sembuf op;
	op.sem_num = 0;
	op.sem_op = 1;
	op.sem_flg = 0;
	semop(sem_id, &op, 1);
}

void normalize_clock() {
	//keeps clock variable from overflowing
	unsigned long nano = shm_ptr->nanoseconds;
	if (nano >= 1000000000) {
		shm_ptr->seconds += 1;
		shm_ptr->nanoseconds -= 1000000000;
	}
}

void normalize_fork() {
	//keeps clock variable from overflowing
	unsigned long nano = next_fork_nano;
	if (nano >= 1000000000) {
		next_fork_sec += 1;
		next_fork_nano -= 1000000000;
	}
}