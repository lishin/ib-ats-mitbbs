#ifdef _WIN32
# include <Windows.h>
# define sleep( seconds) Sleep( seconds * 1000);
#else
# include <unistd.h>
#endif

#include "PosixTestClient.h"
#include "Contract.h"
#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <pthread.h>
#include <iostream>

#define NUM_OF_THREADS 2
using namespace std;

const unsigned MAX_ATTEMPTS = 5000;
const unsigned SLEEP_TIME = 1;
bool TICKBAR = true;

void signal_callback_handler(int signum){
    printf("Caught signal %d\n",signum);
    exit(signum);
}

/* 
void *backTesting(void *ptr){
    const char* host = "";
    unsigned int port = 7498;
    //int clientId = 0;
    int clientId;
    clientId = (int) ptr;
    //cout << clientId;
    
    unsigned int attempt = 0;
    printf("Start of EUR.USD automatic trading Test %d on client %d\n", attempt, clientId);
    //printf("Start of EUR.USD automatic trading Test %d on client\n", attempt);
    for (;;) {
        ++attempt;
        printf("Attempt %u of %u\n", attempt, MAX_ATTEMPTS);

        PosixTestClient client(clientId); 
        client.connect(host, port);

        if(TICKBAR){
            client.tickRecord();
        }
        else {
            client.barRecord();
        }

        while(client.isConnected()) {
            client.processMessages();
        }

        if(attempt >= MAX_ATTEMPTS) {
            break;
        }

        printf("Sleeping %u seconds before next attempt\n", SLEEP_TIME);
        // safe to sleep in thread?
        sleep(SLEEP_TIME);
    }
    return NULL;
}
*/

int main(int argc, char** argv)
{
    signal(SIGINT, signal_callback_handler);
   
    const char* host = "";
    unsigned int port = 7498;
    bool tflag = false;
    int c;
    int clientId;

    while ((c = getopt(argc, argv, "c:n:t:")) != -1){
        cout << "list of parameters: " << c << endl;
        switch (c)
        {
            case 'c':
                clientId = atoi(optarg);
                break;
            case 't':
                tflag = true;
                trail = atoi(optarg)*0.0001;
                break;
            case 'n':
                NUM_OF_TICKS = atoi(optarg);
                //cout << "optarg " << atoi(optarg) << endl;
                break;
            case '?':
                if(isprint(optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort ();
        }
    }
    
    unsigned int attempt = 0;
    printf("Start of EUR.USD automatic trading Test %d on client %d\n", attempt, clientId);
    //printf("Start of EUR.USD automatic trading Test %d on client\n", attempt);
    for (;;) {
        ++attempt;
        printf("Attempt %u of %u\n", attempt, MAX_ATTEMPTS);

        PosixTestClient client(clientId, tflag); 
        client.connect(host, port);

        if(TICKBAR){
            client.tickRecord();
        }
        else {
            client.barRecord();
        }

        while(client.isConnected()) {
            client.processMessages();
        }

        if(attempt >= MAX_ATTEMPTS) {
            break;
        }

        printf("Sleeping %u seconds before next attempt\n", SLEEP_TIME);
        // safe to sleep in thread?
        sleep(SLEEP_TIME);
    }
    /* 
    pthread_t thread[NUM_OF_THREADS];
    pthread_attr_t attr;
    int rc, t;
    void *status;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(t=0; t<NUM_OF_THREADS; ++t){
        printf("In main: creating thread %d\n", t);
        rc = pthread_create(&thread[t], NULL, backTesting, (void *)t);
        if (rc){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    pthread_attr_destroy(&attr);
    for(t=0; t<NUM_OF_THREADS; ++t) {
        rc = pthread_join(thread[t], &status);
        if (rc) {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
        printf("Main: completed join with thread %d having a status of %d\n",t,(int)status);
    }

    //pthread_join(thread0, NULL);
    //pthread_join(thread1, NULL);

    printf ("End of EUR.USD automatic trading Test\n");
    pthread_exit(NULL);
    */
    return EXIT_SUCCESS;
}
