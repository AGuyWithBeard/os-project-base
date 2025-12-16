#define _GNU_SOURCE 
#define READERS 4
#define MyQueue "/dev/myQueue"

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <time.h>
#include <stdatomic.h>

sem_t mutex, empty, full; // Semaphores for synchronization
int queue_fd; // Queue file descriptor
atomic_int counter = 0; // Declare counter as atomic for Queue
char input_string[256]; 
pthread_t writer, readers[READERS]; // Create threads
struct timespec start, end; 

// Writer thread function
void* writer_thread(void* arg) {
    char* input_string = (char*)arg;  // Input string passed to the thread
    size_t length = strlen(input_string);  // Length of the input string
    size_t index = 0;  // Tracks the current character in the string

    while (index < length) {  // Write until all characters are processed
        sem_wait(&empty);  // Wait for an empty slot
        sem_wait(&mutex);  // Acquire the mutex
        int y = 0;
        while(counter < 10 && index < length){ // wrtie tile there is a space in Queue or the string is ended
            (y == 0) ? y = 1 : sem_wait(&empty); //just to dont run the wait(empty) 2 time in first Declartion
            if (write(queue_fd, &input_string[index], 1) == -1) { // Write the current character to the queue
                perror("Write failed"); //error
            } else {
                //printf("Writer: Written '%c' to queue\n", input_string[index]); //succsfully wirted the char
            }

            index++;  // Move to the next character
            atomic_fetch_add(&counter, 1); //increase the number of char in Queue

            sem_post(&full); // Signal that the queue has a new character
        }  
        sem_post(&mutex);  // Release the mutex
    }

    // Write termination signals for each reader thread
    for (int i = 0; i < READERS; i++) {
        sem_wait(&empty);  // Wait for an empty slot
        sem_wait(&mutex);  // Acquire the mutex

        char termination_char = '\0';  // Use '\0' as a termination signal
        if (write(queue_fd, &termination_char, 1) == -1) {
            perror("Write failed");
        } else {
            // printf("Written termination signal\n");
        }

        atomic_fetch_add(&counter,1);
        sem_post(&mutex);// Release the mutex
        sem_post(&full); // Signal that the queue has a new character
    }

    return NULL;
}

void simulate_workload() {
    volatile long sum = 0;
    for (long i = 0; i < 1e8; i++) {
        sum += i;  // Simulated heavy computation
    }
}

// Reader thread function
void* reader_thread(void* arg) {
    char data; // the output data
    while (1) {  // Keep running until a termination signal is encountered
        sem_wait(&full); sem_wait(&full);  // Wait for a full slot
        sem_wait(&mutex); // Acquire the mutex

        // Read a character from the queue
        if (read(queue_fd, &data, 1) == -1) {
            perror("Read failed"); //error
        } else if (data == '\0') {  // Check for termination signal
            // printf("Reader received termination signal. Exiting.\n");
            sem_post(&mutex);  // Release the mutex
            sem_post(&empty);  // Signal an empty slot for completeness
            atomic_fetch_sub(&counter, 1);  // deceares the number of data in Queue
            break;  // Exit the thread
        } else {
            // printf("Reader [Thread 0x%lx]: Read '%c' from queue on CPU %d\n", pthread_self(), data, sched_getcpu());
            atomic_fetch_sub(&counter, 1);   // deceares the number of data in Queue
        }

        sem_post(&mutex);  // Release the mutex
        sem_post(&empty);  // Signal an empty slot

        simulate_workload();
    }

    return NULL;
}

// Set thread affinity
void set_affinity(pthread_t thread, int cpu) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(thread,sizeof(cpu_set_t), &cpuset) != 0) {
        perror("Failed to set affinity");
    }
}

// a function to run for muilti and single core
void runner(int type){

    int available_cpus = sysconf(_SC_NPROCESSORS_ONLN);

    if (type){
        printf("---- single core ----\n");
    } else printf("---- muilti core ----\n");

    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_create(&writer, NULL, writer_thread, input_string);

    for (int i = 0; i < READERS; i++) {
        pthread_create(&readers[i], NULL, reader_thread, NULL);
        set_affinity(readers[i], type ? 0 : i % available_cpus);
    }

    set_affinity(writer, type ? 0 : available_cpus - 1 );

    // Wait for threads to complete
    pthread_join(writer, NULL);
    for (int i = 0; i < READERS; i++) {
        pthread_join(readers[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    if (type){
        printf("time for single core: %.6f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);
    } else printf("time for muilti core : %.6f\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);

}

int main() {
    // Open the queue file
    queue_fd = open(MyQueue, O_RDWR);
    if (queue_fd == -1) {
        perror("Failed to open /dev/myQueue");
        return EXIT_FAILURE;
    }

    // Initialize semaphores
    sem_init(&mutex, 0, 1); 
    sem_init(&empty, 0, 10); // Queue size
    sem_init(&full, 0, 0);

    // Get input string from the user
    printf("Enter a string to write to the queue: ");
    fgets(input_string, sizeof(input_string), stdin);
    input_string[strcspn(input_string, "\n")] = '\0';   // Remove the newline character at the end of the string

    if (strlen(input_string) == 0) {
        fprintf(stderr, "Input string cannot be empty.\n");
        return EXIT_FAILURE;
    }

    runner(1); // single core
    runner(0); // muilti core

    // Destroy semaphores and close file
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);
    close(queue_fd);

    return EXIT_SUCCESS;
}
