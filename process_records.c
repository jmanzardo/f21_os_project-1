#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "report_record_formats.h"
#include "queue_ids.h"
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
int totalRecords = 0;       //KEEPS TRACK OF NUM RECORDS
int totalReports = 0;       //KEEPS TRACK OF NUM REPORTS

pthread_cond_t condVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t s;


typedef struct {
    int recordsSent;
} counts_t;

//overrides signint and changes condition variable, triggering the thread that prints the status report
void handle_sigint(int sig) {
    pthread_cond_signal(&condVar);
    signal(SIGINT, handle_sigint);
    
}

//pthread to print out status report
void *srThread (void *arg) {
    int val;

    int *repSentArray = (int *)arg;

    sem_getvalue(&s, &val);
    
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&condVar, &mutex);

    do {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condVar, &mutex);

        printf("\n***Report***\n");
        printf("%d records read for %d reports\n", totalRecords, totalReports);
        int i;
        for (i = 0; i < totalReports; i++) {
            printf("Records sent for report index %d: %d\n", i + 1, repSentArray[0]);
        }
        pthread_mutex_unlock(&mutex);
        sem_getvalue(&s, &val);

    } while (val == 1);
}

int main(int argc, char**argv)
{
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    report_request_buf rbuf;
    report_record_buf sbuf;
    size_t buf_length;


    //RECEIVE MESSAGE FROM JAVA PROGRAM

    key = ftok(FILE_IN_HOME_DIR,QUEUE_NUMBER);
    if (key == 0xffffffff) {
        fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
        return 1;
    }


    if ((msqid = msgget(key, msgflg)) < 0) {
        int errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("(msgget)");
        fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
    }
    else
        fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);

    // msgrcv to receive report request
    int ret;
    do {
      ret = msgrcv(msqid, &rbuf, sizeof(rbuf), 1, 0);//receive type 1 message

      int errnum = errno;
      if (ret < 0 && errno !=EINTR){
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
      }
    } while ((ret < 0 ) && (errno == 4));
    //fprintf(stderr,"msgrcv error return code --%d:$d--",ret,errno);
    
    fprintf(stderr,"process-msgrcv-request: msg type-%ld, Record %d of %d: %s ret/bytes rcv'd=%d\n", rbuf.mtype, rbuf.report_idx,rbuf.report_count,rbuf.search_string, ret);



    report_request_buf recordsRead[rbuf.report_count];
    recordsRead[0] = rbuf;
    totalReports = recordsRead[0].report_count;
    int k;
    for (k = 1;  k < rbuf.report_count; k++) {
        do {
            ret = msgrcv(msqid, &recordsRead[k], sizeof(rbuf), 1, 0);//receive type 1 message

            int errnum = errno;
            if (ret < 0 && errno !=EINTR){
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("Error printed by perror");
                fprintf(stderr, "Error receiving msg: %s\n", strerror( errnum ));
            }
            } while ((ret < 0 ) && (errno == 4));
            //fprintf(stderr,"msgrcv error return code --%d:$d--",ret,errno);

            fprintf(stderr,"process-msgrcv-request: msg type-%ld, Record %d of %d: %s ret/bytes rcv'd=%d\n", rbuf.mtype, rbuf.report_idx,rbuf.report_count,rbuf.search_string, ret);
    }

    //Hook signaler now that all messages have been read in
    signal(SIGINT, handle_sigint);

    //Create int array to store how many records have been sent back to ReportingSystem
    int repSentArray[rbuf.report_count];
    int j = 0;
    for (j = 0; j < rbuf.report_count; j++) {
        repSentArray[j] = 0;
    }

    //CREATE THREAD THAT WILL BE USED TO SEND STATUS REPORT
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, srThread, (void *) repSentArray);

    //sleep(5);


    char field[RECORD_FIELD_LENGTH];        //Field for reading in each record

    while (fgets(field, RECORD_FIELD_LENGTH, stdin) != NULL) {
        if (strcmp(field, "\n") != 0) {
            pthread_mutex_lock(&mutex);
            totalRecords++;
            pthread_mutex_unlock(&mutex);

        }
        if (totalRecords % 10 == 0) {
            sleep(5);
        }

        int i;
        for (i = 0; i < rbuf.report_count; i++) {   //checks if current record contains any of the search strings

            //SEND MESSAGE TO JAVA PROGRAM

            key = ftok(FILE_IN_HOME_DIR, recordsRead[i].report_idx);
            if (key == 0xffffffff) {
                fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
                return 1;
            }


            if ((msqid = msgget(key, msgflg)) < 0) {
                int errnum = errno;
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("(msgget)");
                fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
            }
            else
                fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);


                //PARSES records.mini IN ORDER TO SEND CORRECT VALUES
            if (strstr(field, recordsRead[i].search_string) != NULL) {        //CHEKCS IF SEARCH STRING IS IN EACH RECORD             //if (strstr(field, rbuf.search_string) != NULL)
                //SEND TO CORRESPONDING MESSAGE QUEQUE
                sbuf.mtype = 2;
                    strcpy(sbuf.record,field); 
                    buf_length = strlen(sbuf.record) + sizeof(int)+1;//struct size without
                    // Send a message.
                    if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
                        int errnum = errno;
                        fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
                        perror("(msgsnd)");
                        fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
                        exit(1);
                    }
                    else
                        fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);
                        repSentArray[i]++;
            } 

        }

    }
  

    //Now that all matching records have been sent, send zero length message to each running thread
    for (k = 0; k < recordsRead[0].report_count; k++) {

            key = ftok(FILE_IN_HOME_DIR, recordsRead[k].report_idx);
            if (key == 0xffffffff) {
                fprintf(stderr,"Key cannot be 0xffffffff..fix queue_ids.h to link to existing file\n");
                return 1;
            }


            if ((msqid = msgget(key, msgflg)) < 0) {
                int errnum = errno;
                fprintf(stderr, "Value of errno: %d\n", errno);
                perror("(msgget)");
                fprintf(stderr, "Error msgget: %s\n", strerror( errnum ));
            }
            else
                fprintf(stderr, "msgget: msgget succeeded: msgqid = %d\n", msqid);


        // Sends the zero legnth message to let ReportingSystem know no more records will be sent
        sbuf.mtype = 2;
        strcpy(sbuf.record, "");
        buf_length = strlen(sbuf.record) + sizeof(int)+1;//struct size without
        // Send a message.
        if((msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT)) < 0) {
            int errnum = errno;
            fprintf(stderr,"%d, %ld, %s, %d\n", msqid, sbuf.mtype, sbuf.record, (int)buf_length);
            perror("(msgsnd)");
            fprintf(stderr, "Error sending msg: %s\n", strerror( errnum ));
            exit(1);
        }
        else
            fprintf(stderr,"msgsnd-report_record: record\"%s\" Sent (%d bytes)\n", sbuf.record,(int)buf_length);

    }


    //Print the final records report and then exit program
    printf("***Report***\n");
    printf("%d records read for %d reports\n", totalRecords, totalReports);
    for (k = 0; k < totalReports; k++) {
        printf("Records sent for report index %d: %d\n", k + 1, repSentArray[k]);
    }
    
    sem_wait(&s);
    pthread_cond_signal(&condVar);
    pthread_join(thread_id, NULL);

    exit(0);
}