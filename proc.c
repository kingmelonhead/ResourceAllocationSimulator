#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;

int main() {

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