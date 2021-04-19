#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int proc_count = 0;
int num_proc = 0;
char buffer[200];

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
void kill_pids();
void print_pid_table();
void print_res();

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

	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
		exit(0);
	}

	//printf("semaphores createed!\n");

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
		exit(0);
	}

	//printf("shared memory created!\n");

	initialize_sems();

	//printf("semaphores initialized!\n");

	initialize_shm();

	print_res();

	//printf("shared memory initialized!\n");

	file_ptr = fopen("logfile.log", "a");

	//printf("created the logfile!\n");

	//main loop
	while (1) {
		//printf("process count %d\n ", num_proc);
		if (num_proc == 0) {
			next_fork_nano = rand() % 500000000;
			fork_proc();
			//then fork and exec
		}
		else if (num_proc < MAX_PROC) {
			if (time_to_fork()) {
				fork_proc();
				//generate new fork time
				next_fork_nano = shm_ptr->nanoseconds + (rand() % 500000000);
				normalize_fork();
			}
		}

		sem_wait(SEM_RES_ACC);

		check_finished();
		handle_releases();
		handle_allocations();
		check_deadlock();

		sem_signal(SEM_RES_ACC);


		sem_wait(SEM_CLOCK_ACC);
		//INCREMENT LOGICAL CLOCK
		nano_change = 1 + ( rand() % 100000000);
		shm_ptr->nanoseconds += nano_change;
		shm_ptr->ms += nano_change / 1000000; 
		normalize_clock();
		sem_signal(SEM_CLOCK_ACC);

		//sleep(1);
	
	}

	return 0;
}

void kill_pids() {
	int i;

	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->pids_running[i] != 0) {
			kill(shm_ptr->pids_running[i], SIGKILL);
		}
	}
}

bool time_to_fork() {
	//find if clock has passed the fork time

	//sprintf(buffer, "Next time to fork: %ld sec %ld ns\n", next_fork_sec, next_fork_nano);
	//log_string(buffer);

	//sprintf(buffer, "Current clock : %d sec %d ns\n", shm_ptr->seconds, shm_ptr->nanoseconds);
	//log_string(buffer);

	if (next_fork_sec == shm_ptr->seconds) {
		if (next_fork_nano <= shm_ptr->nanoseconds) {
			//log_string("time has passed should fork\n");
			return true;
			
		}
	}
	else if (next_fork_sec < shm_ptr->seconds) {
		//log_string("time has passed should fork\n");
		return true;
	}
	//log_string("time has not passed should not fork\n");
	return false;
}
void check_finished() {
	//printf("checking for finished processes...\n");
	int i, j;
	for (i = 0; i < MAX_PROC; i++) {
		//goes through all processes
		if (shm_ptr->finished[i] == EARLY) {
			//here if early termination
			for (j = 0; j < MAX_RESOURCES; j++) {
				//goes through all the resources
				if (shm_ptr->resources[j].allocated[i] > 0) {
					//here is early terminated process has stuff allocated

					//gives the resources back
					shm_ptr->resources[j].instances_remaining += shm_ptr->resources[j].allocated[i];

					//initialize the shm location for this process to be used by a later process
					shm_ptr->resources[j].allocated[i] = 0;
					shm_ptr->resources[j].requests[i] = 0;
					shm_ptr->resources[j].releases[i] = 0;
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

				//gives the resources back
				shm_ptr->resources[j].instances_remaining += shm_ptr->resources[j].allocated[i];

				shm_ptr->waiting[i] = false;
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

					//sees if there is enough of resource 
					if (shm_ptr->resources[j].requests[i] <= shm_ptr->resources[j].instances_remaining) {
						//instances avaliable
						shm_ptr->resources[j].instances_remaining -= shm_ptr->resources[j].requests[i];
						shm_ptr->resources[j].allocated[i] = shm_ptr->resources[j].requests[i];
						shm_ptr->resources[j].requests[i] = 0;
						shm_ptr->sleep_status[i] = 0;
						shm_ptr->waiting[i] = false;
						printf("P%d has recieved access to R%d\n", i, j);
					}
					else {
						//not avaliable
						printf("P%d is going to sleep. Requested R%d but there is not enough instances\n", i, j);
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
		printf("R%d   Instances: %d   Instances Free: %d\n", i, shm_ptr->resources[i].instances, shm_ptr->resources[i].instances_remaining);
	}
}

void check_deadlock() {

}
void cleanup() {
	if (file_ptr != NULL) {
		fclose(file_ptr);
	}
	kill_pids();
	sleep(1);
	shmdt(shm_ptr);
	shmctl(shm_id, IPC_RMID, NULL);
	semctl(sem_id, 0, IPC_RMID, NULL);

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
	//printf("forking a process\n");
	//printf("pid table prior to forking:\n");
	//print_pid_table();
	int i;
	int pid;
	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->pids_running[i] == 0) {
			num_proc++;
			pid = fork();

			if (pid != 0) {
				//printf("P%d being forked (PID:%d)\n", i, pid);
				shm_ptr->pids_running[i] = pid;
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
	exit(0);
}

void child_handler(int sig) {
	pid_t pid;
	while ((pid = waitpid((pid_t)(-1), 0, WNOHANG)) > 0) {

	}

}

void initialize_sems() {
	semctl(sem_id, SEM_RES_ACC, SETVAL, 1);
	semctl(sem_id, SEM_CLOCK_ACC, SETVAL, 1);
}

void initialize_shm() {
	int i, j;
	for (i = 0; i < MAX_RESOURCES; i++) {
		if ((i + 1) % 10 == 0) {
			shm_ptr->resources[i].shared = true;
		}
		else {
			shm_ptr->resources[i].shared = false;
		}
		shm_ptr->resources[i].taken = false;
		shm_ptr->resources[i].instances = 1 + (rand() % MAX_INSTANCES);
		shm_ptr->resources[i].instances_remaining = shm_ptr->resources[i].instances;
		for (j = 0; j < MAX_PROC; j++) {
			shm_ptr->resources[i].requests[j] = 0;
			shm_ptr->resources[i].allocated[j] = 0;
			shm_ptr->resources[i].releases[j] = 0;
			shm_ptr->resources[i].max[j] = 0;
		}
	}
	for (i = 0; i < MAX_PROC; i++) {
		shm_ptr->pids_running[i] = 0;
		shm_ptr->cpu_time[i] = 0;
		shm_ptr->wait_time[i] = 0;
		shm_ptr->sleep_status[i] = 0;
		shm_ptr->finished[i] = 0;
		shm_ptr->waiting[i] = false;
	}
	shm_ptr->seconds = 0;
	shm_ptr->nanoseconds = 0;
	shm_ptr->ms = 0;
}

void find_and_remove(int pid) {
	int i, j;
	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->pids_running[i] == pid) {
			shm_ptr->pids_running[i] = 0;
			shm_ptr->cpu_time[i] = 0;
			shm_ptr->wait_time[i] = 0;
			shm_ptr->sleep_status[i] = 0;
			sem_wait(SEM_RES_ACC);
			for (j = 0; j < MAX_RESOURCES; j++) {
				//move alocated back to instances avaliable
			}
		}
	}
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

void log_string(char* s) {
	//log the passed string to the log file
	fputs(s, file_ptr);
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