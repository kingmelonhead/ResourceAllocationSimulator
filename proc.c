#include "shared.h"

//globals
int shm_id;
int sem_id;
shm_container* shm_ptr;
int my_pid; 
int current_index;
char buffer[500];
unsigned int start_ns, start_sec, wait_for_ns, wait_for_sec, rand_time;
int rand_res;
int rand_inst;

bool check_time_passed();

int main() {

	int resources_used, temp, i;

	my_pid = getpid();
	srand(my_pid * 12);

	//signal handlers
	signal(SIGKILL, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGSEGV, signal_handler);

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

	//gets the current index of the process in the process table based off of its PID
	current_index = get_index(my_pid);

	//initialize elements in shm that are now for this process
	shm_ptr->wants[current_index] = -1;
	shm_ptr->waiting[current_index] = false;
	shm_ptr->finished[current_index] = NO; 
	shm_ptr->sleep_status[current_index] = 0;

	for (i = 0; i < MAX_RESOURCES; i++) {
		shm_ptr->resources[i].allocated[current_index] = 0;
		shm_ptr->resources[i].requests[current_index] = 0;
		shm_ptr->resources[i].releases[current_index] = 0;
	}

	//snapshot of the clock at the time that the process starts
	sem_wait(SEM_CLOCK_ACC);

	start_ns = shm_ptr->nanoseconds;
	start_sec = shm_ptr->seconds;

	sem_signal(SEM_CLOCK_ACC);

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
	while (shm_ptr->finished[current_index] != KILLED_BY_OSS) {

		//waits for time to pass
		while (1) {
			if (check_time_passed()) {
				break;
			}
		}
		//if code reaches here then system clock has progressed past the point where it checks to see if it got its resource
		//and if so it is now able to finish early, grab a new resource or dropo a resource

		if (shm_ptr->sleep_status[current_index] == 1 || shm_ptr->waiting[current_index] == true) {
			//basically if waiting
			//do nothing and skip main body of loop

		}
		else {
			//if not waiting
		
			//if process has been running for at least a full second possibly terminate early
			if (shm_ptr->seconds - start_sec > 1) {
				if (shm_ptr->nanoseconds > start_ns) {
					if (rand() % 10 == 1) {
						//very low chance of terminating early
						sem_wait(SEM_RES_ACC);
						shm_ptr->finished[current_index] = EARLY;
						sem_signal(SEM_RES_ACC);
						cleanup();
						exit(0);
						break;
					}
				}
			}
			

			//if doesnt terminate early then pick a resource to either request or drop
			rand_res = rand() % MAX_RESOURCES;

			sem_wait(SEM_RES_ACC);
			//IF NOT TERMINATING THEN DETERMINE IF THE PROCESS IS GOING TO REQUEST OR RELEASE A RESOURCES
			if (shm_ptr->resources[rand_res].allocated[current_index] > 0) {
				//if resource is already allocated
				if (rand() % 10 < 5) {
					//about a 50 percent chance of dropping the resource
					//printf("P%d is putting in a request to release R%d \n", current_index, rand_res);
					shm_ptr->resources[rand_res].releases[current_index] = shm_ptr->resources[rand_res].allocated[current_index];
					shm_ptr->waiting[current_index] = true;
				}
			}
			else {
				//if resource isnt allocated
				if (rand() % 10 < 5) {
					//about a 50 percent chance of requesting that resource
					if (rand() % 10 < 5) {
						rand_inst = 1 + (rand() % shm_ptr->resources[rand_res].instances);
						if (rand_inst > 0) {
							//printf("P%d is putting in a request to get R%d (%d instances)\n", current_index, rand_res, rand_inst);
							shm_ptr->resources[rand_res].requests[current_index] = rand_inst;
							shm_ptr->waiting[current_index] = true;
						}
					}

				}
			}
			sem_signal(SEM_RES_ACC);


			sem_wait(SEM_CLOCK_ACC);

			//GENERATE WAIT TIME BEFORE NEXT REQUEST/RELEASE
			rand_time = rand() % 250000000;
			wait_for_ns = shm_ptr->nanoseconds;
			wait_for_ns += rand_time;
			if (wait_for_ns >= 1000000000) {
				wait_for_ns -= 1000000000;
				wait_for_sec++;
			}

			sem_signal(SEM_CLOCK_ACC);
		}

	}
	cleanup();

	return 0;
}

void cleanup() {
	shmdt(shm_ptr);
}

int get_shm() {
	//creates the shared memory and attaches the pointer for it (or tries to at least)
	key_t key = ftok("Makefile", 'a');
	//gets chared memory
	if ((shm_id = shmget(key, (sizeof(resource) * MAX_PROC) + sizeof(shm_container), IPC_EXCL)) == -1) {
		//perror("proc.c: shmget failed:");
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

int get_index(int pid) {
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
	exit(0);
}

bool check_time_passed() {
	if (shm_ptr->seconds > wait_for_sec) {
		return true;
	}
	else if (shm_ptr->seconds == wait_for_sec){
		if (shm_ptr->nanoseconds > wait_for_ns) {
			return true;
		}
	}
	return false;
}