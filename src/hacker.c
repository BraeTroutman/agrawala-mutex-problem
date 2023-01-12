#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

typedef struct msg {
  long dest;
  char text[255];
} msg;

int main() {
  srand(time(0));
  int qid = msgget((key_t) 123, IPC_CREAT | 0660);

  while (1) {
    sleep(rand() % 3);
    msg buf;
    buf.dest = 99;
    strncpy(buf.text, "Hacker ja ja ja!", 17);
    if (msgsnd(qid, &buf, sizeof(msg), IPC_NOWAIT) < 0) perror("hacker msgsnd error");
  }
  return 0;
}
