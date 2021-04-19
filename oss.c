#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int proc_count = 0;
int num_proc = 0;

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
void fork();
void find_and_remove(int);
void check_finished();

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

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
		exit(0);
	}

	initialize_sems();
	initialize_shm();

	//main loop
	while (1) {

		if (num_proc == 0) {
			next_fork_nano = rand() % 500000000;
			//then fork and exec
		}
		else if (num_proc < MAX_PROC) {
			if (time_to_fork()) {
				//fork and exec
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
		nano_change = 1 + ( rand() % 10000);
		shm_ptr->clock_nano += nano_change;
		shm_ptr->ms += nano_change / 1000000; 
		normalize_clock();
		sem_signal(SEM_CLOCK_ACC);
	
	}

	return 0;
}

bool time_to_fork() {
	//find if clock has passed the fork time
	if (next_fork_sec == shm_ptr->seconds) {
		if (next_fork_nano <= shm_ptr->nanoseconds) {
			return true;
		}
	}
	else if (next_fork_sec < shm_ptr->seconds) {
		return true;
	}
	return false;
}
void check_finished() {

}
void handle_releases() {

}
void handle_allocations() {


}

void check_deadlock() {

}
void cleanup() {


}

void fork() {
	int i;
	int pid;
	for (i = 0; i < MAX_PROC; i++) {
		if (shm_ptr->pids_running[i] == 0) {
			pid = fork();
		}
		if (pid != 0) {
			shm_ptr->pids_running[i]= pid;
		}
		else {
			execl("./user", "./user", (char*)0);
		}
		return;
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
	for (i = 0 : i < MAX_PROC; i++) {
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
		shm_ptr->clock_seconds += 1;
		shm_ptr->clock_nano -= 1000000000;
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