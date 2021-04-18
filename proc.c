#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int my_pid; 
int current_index;
char buffer[500];
unsigned int start_ns, start_sec, wait_for_ns, wait_for_sec, rand_time;

bool check_time_passed();


int main() {

	//signal handlers
	signal(SIGKILL, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);

	int temp;
	int i, x, r, n;

	

	int resources_used;
	my_pid = getpid();

	srand(my_pid);

	//file_ptr = fopen("logfile", "a");

	//gets the current index of the process in the process table based off of its PID
	current_index = get_index(my_pid);

	//create the shared memory
	if (get_shm() == -1) {
		cleanup();
		exit(0);
	}

	//create the semaphores
	if (get_sem() == -1) {
		cleanup();
		exit(0);
	}

	//determines what resources will attempt to be acquired and then potentially released after use
	//as well as the max number of instances of that resource can be got
	for (i = 0; i < MAX_RESOURCES; i++) {
		temp = rand() % 12;
		if (temp % 3 == 0) { // should produce about a 25% chance of resource being used

			//then if a resource does end up being used then the process comes up with a number of instances 
			//of that particular resource that it is going to try to get
			//this number is between 1 and the number of instances of that resource
			shm_ptr->resources[i].max_resource[current_index] = 1 + (rand() % shm_ptr->resources[i].instances);

			sprintf(buffer, "Process #%d can claim a max of %d instances of resource %d", current_index, shm_ptr->resources[i].max_resource[current_index], i);

			//log this info to the logfile (for now it will go to std out)
			printf(buffer);
		}
	}

	//initialized the cpu and wait times for the process 
	shm_ptr->cpu_time[current_index] = 0;
	shm_ptr->wait_time[current_index] = 0;
	//snapshot of the clock at the time that the process starts
	start_ns = shm_ptr->nanoseconds;
	start_sec = shm_ptr->seconds;

	//initialize the wait for times
	wait_for_ns = start_ns;
	wait_for_sec = start_sec;

	//set the first time to wait for
	rand_time = rand() % 250000000;
	wait_for_ns += rand_time;
	if (wait_for_ns >= 1000000000) {
		wait_for_ns -= 1000000000;
		wait_for_sec++;
	}


	//main loop
	while (1) {

		//waits for time to pass
		while (1) {
			if (check_time_passed()) {
				break;
			}
		}

		//if process has been running for at least a full second possibly terminate early
		if (shm_ptr->seconds - start_sec > 1) {
			if (shm_ptr->nanoseconds > start_ns) {
				//do like a 15% chance of terminating early or something
			}
		}

		//IF NOT TERMINATING THEN DETERMINE IF THE PROCESS IS GOING TO REQUEST OR RELEASE A RESOURCE

		//DO THAT


		break;
	}

	return 0;
}

void cleanup() {
	if (file_ptr != NULL) {
		fclose(file_ptr);
	}
	shmdt(file_ptr);
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(resource) * MAX_PROC) + sizeof(shm_container), IPC_EXCL)) == -1) {
		perror("proc.c: shmget failed:");
		return -1;
	}
	//tries to attach shared memory


	if ((shm_ptr = (shm_container*)shmat(shm_id, 0, 0)) == (shm_container*)-1) {
		perror("proc.c: shmat failed:");
		return -1;
	}

	return 0;
}

int get_sem() {
	//creates the sysv semaphores (or tries to at least)
	key_t key = ftok(".", 'a');
	//gets chared memory
	if ((sem_id = semget(key, NUM_SEMS, 0)) == -1) {
		perror("proc.c: semget failed:");
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

void get_index(int pid) {
	int i;
	for (i = 0; i < MAX_PROC; i++) {
		if (pid == shm_ptr->pids_running[i]) {
			return i;
		}
	}
	return -1;
}

void signal_handler() {
	cleanup();
}

bool check_time_passed() {
	if (shm_ptr->seconds > wait_for_sec) {
		return true;
	}
	else if (shm_ptr->nanoseconds > wait_for_ns) {
		return true;
	}
	return false;
}