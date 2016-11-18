#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

struct thread_info {    /* Used as argument to thread_start() */
  pthread_t thread_id;        /* ID returned by pthread_create() */
  int       thread_num;       /* Application-defined thread # */
  char     *argv_string;      /* From command-line argument */
};

static void *
thread_start(void *arg)
{
  struct thread_info *tinfo = arg;
  char *uargv, *p;

  printf("Thread %d: top of stack near %p; argv_string=%s\n",
	 tinfo->thread_num, &p, tinfo->argv_string);
  return NULL;
}

int main(int argc, char *argv[]) {
  struct thread_info *tinfo;
  int num_threads = atoi(argv[1]);
  // pthread_attr_t attr;
  int s;
  tinfo = calloc(num_threads, sizeof(struct thread_info));
  if (tinfo == NULL) exit(1);

    int tnum=0;
    for (tnum = 0; tnum < num_threads; tnum++) {
      tinfo[tnum].thread_num = tnum + 1;
      tinfo[tnum].argv_string = argv[1];

    /* The pthread_create() call stores the thread ID into
       corresponding element of tinfo[] */

      s = pthread_create(&tinfo[tnum].thread_id, NULL,
		       &thread_start, &tinfo[tnum]);
    if (s != 0)
      perror("pthread error");

    }

    void *res;
    for (tnum = 0; tnum < num_threads; tnum++) {
      s = pthread_join(tinfo[tnum].thread_id, &res);
      if (s != 0)
	perror("pthread_join");

      printf("Joined with thread %d; returned value was %s\n",
	     tinfo[tnum].thread_num, (char *) res);
      free(res);      /* Free memory allocated by thread */
    }
  

    return 0;
}
