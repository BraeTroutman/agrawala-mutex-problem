#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <wait.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define REPLY 1
#define REQUEST 2
#define END_COND 20

// structure defining messages between two nodes (replies and requests)
typedef struct msgbuff {
  long dest;     // the target node id * 10 for REQUEST, id * 10 + 1 for REPLY
  int src;       // the source node id
  int snum;      // the seqnum of the requesting node
} msgbuff;

// structure defining messages from nodes to the print server
typedef struct print_msgbuff {
  long dest;
  char text[255];
} print_msgbuff;

// 3 processes running on each node: handle requests to send, handle replying
// to other nodes, and handle mutual exclusion of the shared medium
int request_handler();
int reply_handler();
int mutual_exclusion();

// my node id
int me;
// total number of nodes
int n;

// shared memory (shared between processes on this node, but not between nodes)
int* request_number;		// what is the most recent request number
int* highest_request_number;	// what is the highest request number encountered
int* outstanding_reply;		// check how many replies whe're waiting on at any given time
int* request_CS;		// whether this node is requesting the shared medium or not
int* reply_deferred;		// keep track of which requesting nodes we haven't sent confirmation to yet
int* exit_flag;			// unused, added for compatibility with other student's code

// semaphores
int mutex;			// mutex for protecting shared memory
int wait_sem;			// keep track of when to wait on the shared medium

// our message queue id
int qid;

char filename[128];

int main(int argc, char* argv[]) {
  // process ids for request, reply, main, and mutual exlusion processes
  int reqh_pid, reph_pid, me_pid, msg_pid;
  
  me = atoi(argv[1]);
  n = atoi(argv[2]);
  
  key_t q_key = 123;

  // global variable/shared memory initializations
  int shid = shmget(me, (n+5)*sizeof(int), IPC_CREAT | 0660);
  
  qid = msgget(q_key, IPC_CREAT | 0660); 

  printf("[BRAE] %d\n", qid);
  request_number = shmat(shid, (void*) 0, 0);
  (*request_number) = 0;

  printf("[BRAE] attached shm\n");
  highest_request_number = request_number+1;
  (*highest_request_number) = 0;
  outstanding_reply = request_number+2;
  request_CS = request_number+3;
  (*request_CS) = 0;
  exit_flag = request_number+4;
  (*exit_flag) = 0;
  reply_deferred = request_number+5;

  for (int i = 0; i < n; i++) {
    reply_deferred[i] = 0;
  }

  mutex = semget(me, 1, IPC_CREAT | 0660);
  wait_sem = semget(me+999, 1, IPC_CREAT | 0660);
  semctl(mutex, 0, SETVAL, 1);
  semctl(wait_sem, 0, SETVAL, 1); 

  // fork three child processes
  if ((reqh_pid = fork()) == 0) {
    request_handler();
  }

  if ((reph_pid = fork()) == 0) {
    reply_handler();
  }

  if ((me_pid = fork()) == 0) {
    mutual_exclusion();
  }
  
  if (reqh_pid == -1 || reph_pid == -1 || me_pid == -1 || msg_pid == -1) 
    perror("Node subprocess creation failure");

  // wait on all my children
  int status;
  while (wait(&status) != -1);

  // detach shared mem 
  shmdt(request_number);
  shmctl(shid,IPC_RMID,NULL);

  return 0;
}

int request_handler() {
  printf("[BRAE] node %d's reqhandler process\n", me);
  msgbuff buff;
  int k, j;

  while (1) {
    // get the first REQUEST message out of the message queue 
    if (msgrcv(qid, &buff, sizeof(msgbuff), me*10, 0) < 0) perror("msg rcv error");

    printf("[BRAE] node %d recieved a REQUEST from node %d with seqnum %d\n", me, buff.src, buff.snum);

    j =  buff.src;
    k =  buff.snum;

    // update the highest request number based on the current requester 
    if (k > *highest_request_number) (*highest_request_number) = k;

    // create a sembuf for our P operation 
    struct sembuf mutex_sb;
    mutex_sb.sem_num = 0;
    mutex_sb.sem_op = -1;
    mutex_sb.sem_flg = 0;
   
    // P(mutex) 
    if (semop(mutex, &mutex_sb, 1) < 0) perror("semop 1 error");
    printf("[BRAE] node %d's seqnum is %d, node %d's seqnum is %d, node %d %s requesting the cs\n", me, *request_number, j, k, me, *request_CS == 1 ? "is" : "isn't");
    // decide whether we will defer the request or send back a reply
    int defer_it = (*request_CS) && ((k > *request_number) || (k == *request_number && j > me));
   
    // V(mutex) 
    mutex_sb.sem_op = 1;
    if (semop(mutex, &mutex_sb, 1) < 0) perror("semop 2 error"); 
    
    if (defer_it) {
      // note that we have deferred a request from node j 
      printf("[BRAE] node %d is deferring a REQUEST from node %d\n", me, j);
      reply_deferred[j-1] = 1;
    } else {
      // send a REPLY message to node j
      msgbuff buff;
      buff.src = me;
      buff.snum = *request_number;
      buff.dest = j*10 + 1;
      printf("[BRAE] node %d is sending a REPLY to node %d\n", me, j);
      if (msgsnd(qid, &buff, sizeof(msgbuff), 0) < 0) perror("msg snd error");
    }
  }
}

int reply_handler() {
  msgbuff buff;
  printf("[BRAE] node %d's replyhandler process\n", me);
  while(1) {
    if (msgrcv(qid, &buff, sizeof(msgbuff), 10*me + 1, 0) < 0) perror("msg rcv error");
    
    printf("[BRAE] node %d recieved a REPLY from node %d\n", me, buff.src);

    // take note if we've recieved a reply
    (*outstanding_reply)--;
    struct sembuf wait_sb;
    wait_sb.sem_num = 0;
    wait_sb.sem_op = 1;
    wait_sb.sem_flg = 0;
    if (semop(wait_sem, &wait_sb, 1) < 0) perror("semop 3 error");
  }
}

int mutual_exclusion() {
  struct sembuf mutex_sb;
  printf("[BRAE] node %d's mutexclusion process\n", me);
  struct sembuf wait_sb;
  msgbuff req_msg;
  msgbuff rep_msg;

  while (1) { 
    mutex_sb.sem_num = 0;
    mutex_sb.sem_op = -1;
    mutex_sb.sem_flg = 0; 
  
    *request_CS = 1;
    *request_number = (*highest_request_number)+1;
  
    mutex_sb.sem_op = 1;
    if (semop(mutex, &mutex_sb, 1) < 0) perror("semop 4 error");

    *outstanding_reply = n - 1;
    
    req_msg.src = me;
    req_msg.snum = *request_number;
  
    for (int i = 1; i <= n; i++) {
      if (i != me) { 
        req_msg.dest = (long) 10*i;
        printf("[BRAE] node %d sending a REQUEST to node %d with seqnum %d\n", me, i, *request_number);
        if (msgsnd(qid, &req_msg, sizeof(msgbuff), 0) < 0) perror("msg snd error");
      }
    }

    wait_sb.sem_num = 0;
    wait_sb.sem_op = -1;
    wait_sb.sem_flg = 0;

    printf("[BRAE] node %d is waiting to enter the critical section\n", me);
    while (*outstanding_reply != 0) {
      if (semop(wait_sem, &wait_sb, 1) < 0) perror("semop 5 error");
    }
    mutex_sb.sem_op = -1;
    if (semop(mutex, &mutex_sb, 1) < 0) perror("semop 6 error");
    (*request_CS) = 0;
    mutex_sb.sem_op = 1;
    if (semop(mutex, &mutex_sb, 1) < 0) perror("semop 7 error");

    printf("[BRAE] node %d in crit sec\n", me);
    print_msgbuff line;
    line.dest = (long) 99;
    snprintf(line.text, 255, "########## Beginning of Node %d Output ##########", me);
    printf("[BRAE] %s\n", line.text);
    struct msqid_ds buf;
    msgctl(qid, IPC_STAT, &buf);
    printf("[BRAE] qid: %d\n", qid);
    printf("[BRAE] %d messages, %d/%d bytes in queue\n", buf.msg_qnum, buf.msg_cbytes, buf.msg_qbytes);
    if (msgsnd(qid, &line, sizeof(print_msgbuff), 0) < 0) perror("sending error");
    
    for (int i = 0; i < 5; i++) {
      snprintf(line.text, 255, "node %d, line %d", me, i+1);
      printf("[BRAE] %s\n", line.text);
      if (msgsnd(qid, &line, sizeof(print_msgbuff), 0) < 0) perror("msg snd error");
    }

    snprintf(line.text, 255, "---------- The End of Node %d's Output ----------", me);
    printf("[BRAE] %s\n", line.text);
    if (msgsnd(qid, &line, sizeof(print_msgbuff), 0) < 0) perror("msg snd error");

    for (int i = 0; i < n; i++) {
      if (reply_deferred[i]) {
        reply_deferred[i] = 0;
        rep_msg.src = me;
        rep_msg.snum = *request_number;
        printf("[BRAE] node %d is sending a deferred REPLY to node %d\n", me, i+1);
        rep_msg.dest = 10*(i+1) + 1;
        if (msgsnd(qid, &rep_msg, sizeof(msgbuff), 0) < 0) perror("msg snd error");
      }
    }
    
    sleep(me);
  }
}
