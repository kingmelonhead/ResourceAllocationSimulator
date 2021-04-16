#include "shared.h"

//globals
int shm_id;
int sem_id;
FILE* file_ptr;
shm_container* shm_ptr;
int proc_count = 0;

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

		//FORK PROCESSES RANDOMLY UP TO 18 PROCESSES TOTAL
		//DO RANDOMLY BY CRETING A RANDOM NEXT_FORK TIME AND SEEING IF THE LOGICAL CLOCK HAS PASSED IT YET OR NOT

		//IF A PROCESS IS FORKED THEN MAKE A NEXT_FORK TIME THAT IS 1 TO 500 MS AFTER THE CURRENT LOGICAL CLOCK

		//GO THROUGH EACH RESOURCE
		//CHECK THE REQUESTS FOR EACH RESOURCE

		//IF RESOURCE IS AVALIABLE, ALLOW THE PROCESS TO HAVE THE RESOURCE

		//WHEN/IF THERE COMES A TIME THAT A PROCESS WANTS A RESOURCE WHAT DOESNT HAVE ANY AVALIABLE 
		//THEN MOVE THE PID TO A SLEEPING QUEUE (THAT WAITS FOR THE REQUESTED RESOURCE TO OPEN UP)

		//NEXT LOOP HERE IS THE DEADLOCK DETECTION LOOP
		while (1) {

			//RUN DEADLOCK CHECK FUNCTION

			//IF DEADLOCK KILL TROUBLE PROCESS

			//IF NO DEADLOCK AT START OF LOOP THEN BREAK
			break;

			//IF THERE WAS DEADLOCK THEN LET LOOP RUN AGAIN TO SEE IF THERE IS ANOTHER
		}

		//ONCE GETTING OUT OF THAT LOOP IT MEANS DEADLOCK HAS BEEN RESOLVED

		//INCREMENT LOGICAL CLOCK
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