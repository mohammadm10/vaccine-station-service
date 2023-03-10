/*
 * UVic CSC 360, Summer 2021
 * This code copyright 2021: Roshan Lasrado, Mike Zastre
 *
 * Assignment 2: Task 1
 * --------------------
 * Simulate a Vaccination Center with `1` Registration Desk and `N` 
 * Vaccination Stations.
 * 
 * Input: Command Line args
 * ------------------------
 * ./vaccine <num_vaccination_stations `N`> <input_test_case_file>
 * e.g.
 *      ./vaccine 10 test1.txt
 * 
 * Input: Test Case file
 * ---------------------
 * Each line corresponds to person arrive for vaccinationn 
 * and is formatted as:
 *
 * <person_id>:<arrival_time>,<service_time>
 * 
 * NOTE: All times represented in `Tenths of a Second`.
 * 
 * Expected Sample Output:
 * -----------------------
 * Person 1: Arrived at 3.
 * Person 1: Added to the queue.
 * Vaccine Station 1: START Person 1 Vaccination.
 * Vaccine Station 1: FINISH Person 1 Vaccination.
 * 
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>


/*
 * A queue structure provided to you by the teaching team. This header
 * file contains the function prototypes; the queue routines are
 * linked in from a separate .o file (which is done for you via
 * the `makefile`).
 */
#include "queue.h"


/* 
 * Some compile-time constants related to assignment description.
 */
#define MAX_VAC_STATIONS 10
#define MAX_INPUT_LINE 100
#define TENTHS_TO_SEC 100000


/*
 * Here are variables that are available to all threads in the
 * process. Given these are global, you need not pass them as
 * parameters to functions. However, you must properly initialize
 * the queue, the mutex, and the condition variable.
 */
Queue_t *queue;
pthread_mutex_t queue_mutex;
pthread_cond_t queue_condvar;
unsigned int num_vac_stations;
int num_people = 1; //keeps track of the number of people to enqueue at the reg_desk
int flag = 0; 
int num_stations; //number of vaccination threads
int cnt = 0; //counter
int queue_count = 0; //increments each time something is added to the queue

/*
 * Function: reg_desk
 * ------------------
 *  Registration Desk Thread.
 *  Reads the input file and adds the vaccination persons to the
 *  queue as per their arrival times.
 * 
 *  arg: Input file name
 *  
 *  returns: null
 */

/* This function takes in no parameters and check whether everyone has been added to the queue
 * Once everyone has been added, a broadcast is sent out
 * Returns 1 if everyone has been added, 0 otherwise
 */

int isRegComplete(){
	if(queue_count == num_people){
		pthread_cond_broadcast(&queue_condvar);
		return 1;
	}
	return 0;
}

/* This function take in the the vaccination time, station and id for each person
 * Once the vaccination is over, a broadcast is given to signal that that station is done
 * Cnt is decremented so someone else can use the station
 */

void isVacComplete(int vac, int station, int id){
    usleep(vac*TENTHS_TO_SEC);
    printf("Vaccination Station %d: FINISH Person %d vaccination.\n", station, id);
    pthread_cond_broadcast(&queue_condvar);
    cnt--;
}

void *reg_desk(void *arg) {
    char *filename = (char *)arg;

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        fprintf(stderr, "File \"%s\" does not exist.\n", filename);
        exit(1);
    }

    char line[MAX_INPUT_LINE];
    unsigned int current_time = 0;

    while (fgets(line, sizeof(line), fp)) {
        int person_id;
        int person_arrival_time;
        int person_service_time;

        int vars_read = sscanf(line, "%d:%d,%d", &person_id, 
            &person_arrival_time, &person_service_time);

        if (vars_read == EOF || vars_read != 3) {
            fprintf(stderr, "Error reading from the file.\n");
            exit(1);
        }

        if (person_id < 0 || person_arrival_time < 0 || 
            person_service_time < 0)
        {
            fprintf(stderr, "Incorrect file input.\n");
            exit(1);
        }

        int arrival_time = person_arrival_time;

        // Sleep to simulate the persons arrival time.
        usleep((arrival_time - current_time) * TENTHS_TO_SEC);
        fprintf(stdout, "Person %d: arrived at time %d.\n", 
            person_id, arrival_time);

        // Update the current time based on simulated time elapsed.
        current_time = arrival_time;

	PersonInfo_t *person = new_person(); //make new person

        person->id = person_id;			    //add
        person->arrival_time = person_arrival_time; //person
        person->service_time = person_service_time; //info
	
	pthread_mutex_lock(&queue_mutex);
	while(queue_size(queue) == MAX_INPUT_LINE){ //while the queue is full
		pthread_cond_wait(&queue_condvar, &queue_mutex); //wait for signal
	}
	enqueue(queue, person); //add person to the queue
	queue_count++; //Increments each time someone is enqueued
	pthread_mutex_unlock(&queue_mutex);
        printf("Person %d: Added to queue.\n", person->id);
             
	pthread_cond_signal(&queue_condvar); //Once someone is put into the queue, signal to start their vaccination
    }

    fclose(fp);

    flag = isRegComplete(); //check if evreyone has registered at the desk

    return NULL;
}


/*
 * Function: vac_station
 * ---------------------
 *  Vaccination Station Thread.
 *  Vaccinate the persons from the queue as per their service times.
 *i
 *  arg: Vaccination station number
 *
 *  returns: null
 *
 * Remember: When performing a vaccination, the vac_station 
 * must sleep for the period of time required to "service"
 * that "person". (This is part of the simulation). Assuming
 * the "person" to be serviced is a pointer to an instance of
 * PersonInfo, the sleep would be something like:
 *
 *      usleep(person->service_time * TENTHS_TO_SEC);
 *
 */
void *vac_station(void *arg) {

    int station_num = *((int *)arg) + 1;
    PersonInfo_t *person;
    pthread_mutex_lock(&queue_mutex);
    while(queue_size(queue) == 0){
        pthread_cond_wait(&queue_condvar, &queue_mutex); //wait until the first person has been enqueued
    }
    pthread_mutex_unlock(&queue_mutex);
    deallocate(arg);

    while (queue_size(queue) != 0) {
       if(cnt < num_stations){
	    pthread_mutex_lock(&queue_mutex);
            person = dequeue(queue); //remove person to start their vaccination
	    printf("Vaccination Station %d: START Person %d vaccination. \n", station_num, person->id);
  	    cnt++; //increment everytime someone is using a vaccination station
	    pthread_mutex_unlock(&queue_mutex);	    
	    isVacComplete(person->service_time, station_num, person->id);
        }
        pthread_mutex_lock(&queue_mutex); 
        while(cnt == num_stations && flag == 1){ //wait while all the stations are full and every person is in the queue
    	    pthread_cond_wait(&queue_condvar, &queue_mutex);
	}
	pthread_mutex_unlock(&queue_mutex);
    }
    return NULL;
}


/*
 * Function: validate_args
 * -----------------------
 *  Validate the input command line args.
 *
 *  argc: Number of command line arguments provided
 *  argv: Command line arguments
 */
void validate_args(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Invalid number of input args provided! "
                        "Expected 2, received %d\n", 
                        argc - 1);
        exit(1);
    }

    num_vac_stations = atoi(argv[1]);
    if ((num_vac_stations <= 0) || (num_vac_stations > MAX_VAC_STATIONS)) {
        fprintf(stderr, "Vaccine stations exceeded the MAX LIMIT.%d\n", 
            argc - 1);
        exit(1);
    }

}

/*
 * Function: initialize_vars
 * -------------------------
 *  Initialize the mutex, conditional variable and the queue.
 */
void initialize_vars() {
    
    pthread_mutex_init( &queue_mutex, NULL); //initialization of mutex
    pthread_cond_init( &queue_condvar, NULL); //initialization of cond var
    queue = (Queue_t*) malloc(sizeof(Queue_t));
    queue->start = NULL;
    queue->end = NULL;
}


/*
 * Function: main
 * -------------------------
 * argv[0]: Number of Vaccination Stations 
 * argv[1]: Input file/test case.
 */
int main(int argc, char *argv[]) {
    int i, status;

    char *filename = (char *)argv[2];

    FILE *fp = fopen(filename, "r");

    if (fp == NULL) {
        fprintf(stderr, "File \"%s\" does not exist.\n", filename);
        exit(1);
    }

    while(!feof(fp)){ // reference to https://stackoverflow.com/questions/12733105/c-function-that-counts-lines-in-file
            char temp = fgetc(fp);
            if(temp == '\n'){
                num_people++; //this will be used to create the amount of threads needed
            }
        }

    validate_args(argc, argv);

    initialize_vars();

    num_stations = atoi(argv[1]); //Holds the number of vaccination stations given by the user

    pthread_t vStation[num_people]; //initialize the vaccination threads, amount equal to the number of people
    for(i = 0; i < num_people; i++){
        int *arg = malloc(sizeof(*arg)); //reference to https://stackoverflow.com/questions/19232957/pthread-create-and-passing-an-integer-as-the-last-argument
        *arg = i;
        if(pthread_create(&vStation[i], NULL, &vac_station, arg) != 0){
	    printf("Error creating thread %d\n", i);
            return 1;
      }
    }

    pthread_t rDesk; //initialize single registration desk thread
    if(pthread_create(&rDesk, NULL, &reg_desk, (argv[2])) != 0){
	printf("Error creating thread");
        return 2;
    }

    for(i = 0; i < num_people; i++){
            if(pthread_join(vStation[i], NULL) != 0){
       		  return 3;
        }
    }
    
    if(pthread_join(rDesk, NULL) != 0){
        return 4;
    }

    free(queue);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_condvar);
      
}
