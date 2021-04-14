#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;

void child_handler();
void initialize_sems();
void initialize_shm();

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

	//main loop
	while (1) {
		break;
	}

	return 0;
}
void cleanup() {


}


void signal_handler() {
	cleanup();
	exit(0);
}

void child_handler() {


}

void initialize_sems() {

}

void initialize_shm() {


}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(resource_info) * MAX_PROC) + sizeof(shm_container), IPC_CREAT | 0666)) == -1) {
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