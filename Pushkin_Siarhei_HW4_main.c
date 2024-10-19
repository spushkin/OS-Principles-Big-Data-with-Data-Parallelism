/**************************************************************
* Class::  CSC-415-01 Summer 2024
* Name:: Siarhei Pushkin
* GitHub-Name:: spushkin
* Project:: Assignment 4 – Word Blast
*
* File:: Pushkin_Siarhei_HW4_main.c
*
* Description:: Small program to read the Word & Peace and,
* using threads, count/tally each word that is six or more 
* characters long.
**************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

char *delimiters = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

typedef struct {
    char word[100];
    int freq;
} WordEntry;

WordEntry wordList[10000];
int totalWords = 0, fileDescriptor;
size_t segmentSize, totalFileSize;
pthread_mutex_t wordListMutex;

// initialize word list with zeros
void initializeWordList() {
    memset(wordList, 0, sizeof(wordList));
}

void updateWordList(char *word) {
    // lock the mutex
    pthread_mutex_lock(&wordListMutex);  
    for (int i = 0; i < totalWords; i++) {
        if (strcasecmp(wordList[i].word, word) == 0) {
            // increment frequency if word already exists
            wordList[i].freq++;  
            pthread_mutex_unlock(&wordListMutex);
            return;
        }
    }
    if (totalWords < 10000) {
        // add new word to the list
        strncpy(wordList[totalWords].word, word, 99);
        wordList[totalWords++].freq = 1;
    }
    // unlock the mutex
    pthread_mutex_unlock(&wordListMutex);
}

int compareWordEntries(const void *a, const void *b) {
    return ((WordEntry *)b)->freq - ((WordEntry *)a)->freq;
}

void *processSegment(void *arg) {
    int segmentIndex = *(int *)arg;
    size_t start = segmentIndex * segmentSize;
    size_t end = (segmentIndex == 10000 - 1) ? totalFileSize : start + segmentSize;
    char c;

    // adjust start and end
    if (start > 0) {
        pread(fileDescriptor, &c, 1, start - 1);
        while (!strchr(delimiters, c) && start < end) start++, pread(fileDescriptor, &c, 1, start - 1);
    }
    if (end < totalFileSize) {
        pread(fileDescriptor, &c, 1, end);
        while (!strchr(delimiters, c) && end < totalFileSize) end++, pread(fileDescriptor, &c, 1, end);
    }
    // allocate buffer for the segment
    char *buffer = malloc(end - start + 1);
    if (!buffer) {
        perror("Memory allocation error");
        pthread_exit(NULL);
    }
    pread(fileDescriptor, buffer, end - start, start);
    buffer[end - start] = '\0';

    // tokenize and update word list
    char *token, *context;
    for (token = strtok_r(buffer, delimiters, &context); token; token = strtok_r(NULL, delimiters, &context))
        if (strlen(token) >= 6) updateWordList(token);

    free(buffer);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) 
    {
    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************

    int threadCount = atoi(argv[2]);
    fileDescriptor = open(argv[1], O_RDONLY);
    if (fileDescriptor < 0) {
        perror("File opening error");
        exit(EXIT_FAILURE);
    }

    // determine the total file size
    totalFileSize = lseek(fileDescriptor, 0, SEEK_END);
    if (totalFileSize == (off_t)-1) {
        perror("Error determining file size");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }
    lseek(fileDescriptor, 0, SEEK_SET);
    // calculate segment size for each thread
    segmentSize = totalFileSize / threadCount;

    initializeWordList();
    if (pthread_mutex_init(&wordListMutex, NULL) != 0) {
        perror("Mutex initialization error");
        close(fileDescriptor);
        exit(EXIT_FAILURE);
    }

    // create and start threads
    pthread_t threads[threadCount];
    int threadIndices[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threadIndices[i] = i;
        if (pthread_create(&threads[i], NULL, processSegment, &threadIndices[i]) != 0) {
            perror("Thread creation error");
            close(fileDescriptor);
            exit(EXIT_FAILURE);
        }
    }

    // wait for all threads to finish
    for (int i = 0; i < threadCount; i++)
        pthread_join(threads[i], NULL);

    // sort the word list by frequency
    qsort(wordList, totalWords, sizeof(WordEntry), compareWordEntries);
    printf("Word Frequency Count on %s with %d threads\n", argv[1], threadCount);
    printf("Printing top %d words 6 characters or more.\n", 10);
    for (int i = 0; i < 10 && i < totalWords; i++)
        printf("Number %d is %s with a count of %d\n", i + 1, wordList[i].word, wordList[i].freq);

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    //Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
        {
        --sec;
        n_sec = n_sec + 1000000000L;
        }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // clean up
    pthread_mutex_destroy(&wordListMutex);
    close(fileDescriptor);

    return 0;
}
