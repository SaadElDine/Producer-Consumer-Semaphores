#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define N 5                 // Number of mCounter threads
#define BUFFER_SIZE 6       // Buffer size
#define INTERVAL_SIZE 3     // Time interval for mMonitor

#define KRED  "\x1B[31m"    //
#define KGRN  "\x1B[32m"    //
#define KYEL  "\x1B[33m"    // define colors
#define KBLU  "\x1B[34m"    //
#define KNRM  "\x1B[0m"     //


sem_t counter_mutex, buffer_mutex, full_sem, empty_sem;

int counter = 0;
int buffer[BUFFER_SIZE];
int buffer_index = 0;       //buffer_index is  to keep track  the next available position in the buffer

void *mCounter(void *arg) {
    int id = *(int *)arg;

    while (1) {
        srand(time(NULL));  // Simulate random arrival time for messages
        // Pick random number from [0,1,2,3,4,5] +1 will shift the interval by 1 [1,2,3,4,5]
        sleep(rand() % 5 + 1); 
        printf("%sCounter thread %d: received a message\n",KYEL, id); 
        printf("%sCounter thread %d: waiting to write\n",KYEL, id);
        sem_wait(&counter_mutex);
        counter++;
        printf("%sCounter thread %d: now adding to counter, counter value=%d\n",KGRN, id, counter);
        sem_post(&counter_mutex);
    }

}

void *mMonitor(void *arg) {
    while (1) {
        // Simulate random time interval
        srand(time(NULL));
        sleep(INTERVAL_SIZE);

        sem_wait(&counter_mutex);
        int count_value = counter;
        printf("%sMonitor thread: reading a count value of %d\n", KBLU,count_value);
        counter = 0;
        sem_post(&counter_mutex);

        if (buffer_index < BUFFER_SIZE) {
            sem_wait(&empty_sem);
            sem_wait(&buffer_mutex);
            buffer[buffer_index++] = count_value;
            //write the count_value to the buffer at the current buffer_index position and then increments buffer_index.
            //Explanation: buffer_index is incremented after writing to ensure the next value will be written to 
            //the next position in the buffer. 
            // you can write in 2 seperate lines
            // buffer[buffer_index] = count_value;
            //buffer_index++;
            printf("%sMonitor thread: writing to buffer at position %d\n",KGRN, buffer_index - 1);
        } else {
            printf("%sMonitor thread: Buffer full!!\n",KRED);
        }
        sem_post(&buffer_mutex);
        sem_post(&full_sem);
    }

}

void *mCollector(void *arg) {
    while (1) {

        if (buffer_index > 0) {
            sem_wait(&full_sem);
            sem_wait(&buffer_mutex);
            int collected_value = buffer[--buffer_index];
            //reads a value from the buffer at the current buffer_index position and then decrements buffer_index.
            // explenation:buffer_index is decremented after reading to ensure the next value will be read 
            //from the correct position in the buffer
            // you can write in 2 seperate lines
            // int collected_value = buffer[buffer_index];
            //buffer_index--;
            printf("%sCollector thread: reading from the buffer at position %d\n", KGRN,buffer_index);
        } else {
            printf("%sCollector thread: nothing is in the buffer!\n",KRED);
        }
        sem_post(&buffer_mutex);
        sem_post(&empty_sem);
        srand(time(NULL));
        sleep(rand() % 5 + 3);//pick random number from [0,1,2,3,4,5] +3 will shift the interval by 1 [3,4,5,6,7]
    }


}
void intHandler(int dummy) {
	// set the noramal color back
    printf("%sExit\n", KNRM);
	// Destroy the semaphore 
	sem_destroy(&counter_mutex);
    sem_destroy(&buffer_mutex);
    sem_destroy(&full_sem);
    sem_destroy(&empty_sem);

	exit(0);
}


int main() {

    signal(SIGINT, intHandler);
    //sem_init(address Of Semaphor variable ,0 shared between threads else between process ,initial value (size of semaphore))
    sem_init(&counter_mutex, 0, 1); 
    sem_init(&buffer_mutex, 0, 1);
    sem_init(&full_sem, 0, 0);// inithially th
    sem_init(&empty_sem, 0, BUFFER_SIZE);
    
    pthread_t mCounter_threads[N];
    pthread_t mMonitor_thread, mCollector_thread;

    int mCounter_ids[N];
    for (int i = 0; i < N; i++) {
        // give each thread id 
        mCounter_ids[i] = i;
        // creating each thread 
        pthread_create(&mCounter_threads[i], NULL, mCounter, &mCounter_ids[i]);
    }

    pthread_create(&mMonitor_thread, NULL, mMonitor, NULL);
    pthread_create(&mCollector_thread, NULL, mCollector, NULL);

    for (int i = 0; i < N; i++) {
        // wait for mcounter_thread to finish
        pthread_join(mCounter_threads[i], NULL);
    }
    // wait for mMonitor_thread , mCollector_thread to finish
    pthread_join(mMonitor_thread, NULL);
    pthread_join(mCollector_thread, NULL);

    return 0;


}

