#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int my_pid; 
int current_index;


int main() {

	int resources_used;
	my_pid = getpid();

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


	//main loop
	while (1) {

		//RANDOMLY CHECK FOR TERMINATION EVERY 0 - 250 MS
		//IF TERMINATE RELEASE RESOURCES

		//ONLY EARLY TERMINATE IF PROCESS HAS BEEN RUNNING FOR AT LEAST 1 SECOND OF LOGICAL CLOCK

		//IF NOT TERMINATING THEN DETERMINE IF THE PROCESS IS GOING TO REQUEST OR RELEASE A RESOURCE

		//DO THAT


		break;
	}

	return 0;
}

void cleanup() {


}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(resource_info) * MAX_PROC) + sizeof(shm_container), IPC_EXCL)) == -1) {
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