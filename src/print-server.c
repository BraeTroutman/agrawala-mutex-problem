#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>

typedef struct msg {
  long dest;
  char txt[255];
} msg;

int main() {
   msg buf;
   while (1) {
     	key_t q_key = 123;
	int qid = msgget(q_key, IPC_CREAT | 0660);
	size_t byt;
	if((byt = msgrcv(qid, &buf, sizeof(msg) , 99, 0)) > 0) {
//	  printf("%ld bytes copied into buf\n", buf.txt);
	  printf("%s\n", buf.txt); fflush(stdout);
	} else {
	  perror("reading error");
	}
   }
}
