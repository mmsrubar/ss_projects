/*
 * Project:     Implementation of Pipeline Merge Sort algorithm
 * Seminar:     Parallel and Distributed Algorithms
 * Author:      Michal Srubar, xsruba03@stud.fit.vutbr.cz
 * Date:        Sat Mar 14 16:22:02 CET 2015
 */

#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>
#include <time.h>
#include "pms.h"

/* This structure is send over the mpi interface */
typedef struct item {
  unsigned int val;   /* sorted value */ 
  unsigned int seq;   /* a sequence the value belongs to */
} MPI_Item;

/* This struct represents item in a process queue */
typedef struct qitem {
  TAILQ_ENTRY(qitem) entries;
  MPI_Item *item;
  int val;
} QItem;

TAILQ_HEAD(head, qitem) down;
TAILQ_HEAD(, qitem) up;
TAILQ_HEAD(, qitem) out;      /* only last process can write to it */
TAILQ_HEAD(, qitem) in;

/* This func creates item that can be send over MPI interface.
 * @val     The number.
 * @seq     Seqence the number belongs to.
 * @return  Pointer to the MPI_Item.
 */
MPI_Item *create_mpi_item(unsigned int val, unsigned int seq)
{
  MPI_Item *new;

  if ((new = (MPI_Item *) malloc(sizeof(MPI_Item))) == NULL) {
    perror("malloc()");
    return NULL;
  }

  new->val = val;
  new->seq = seq;

  return new;
}

/* Create queue item from sent/received item from MPI.
 * @item    An MPI_struct item.
 * @return  Pointer to a new queue item.
 */
QItem *create_qitem(MPI_Item *item)
{
  QItem *new;

  if ((new = (QItem *) malloc(sizeof(QItem))) == NULL) {
    perror("malloc()");
    return NULL;
  }

  if ((new->item = create_mpi_item(item->val, item->seq)) == NULL) {
    perror("malloc()");
    return NULL;
  }

  return new;
}

/* Put a received item into the corrent queue based on cur_up_down var.
 * @cur_up_down   Are we currently working with UP or DOWN queue?
 * @item          Received item from left processor.
 */
void queue_up(int cur_up_down, QItem *item)
{
  if (cur_up_down == UP) {
    TAILQ_INSERT_TAIL(&up, item, entries);
  } else {
    TAILQ_INSERT_TAIL(&down, item, entries);
  }
}

/* Print content of UP and DOWN queues. This is just for debug purposes. */
void queues_print(int id)
{
  QItem *tmp;

  DPRINT("\tP%d:\n", id);
  TAILQ_FOREACH_REVERSE(tmp, &up, head, entries) {
    DPRINT("|%d(%d)", tmp->item->val, tmp->item->seq);
  }
  DPRINT("|\n");

  TAILQ_FOREACH_REVERSE(tmp, &down, head, entries) {
    DPRINT("|%d(%d)", tmp->item->val, tmp->item->seq);
  }
  DPRINT("|\n");
}

/* Get received item and decide whether to put it into the UP or DOWN queue
 * based on current_up_down position and sequnce number.
 *
 * @recv  Received item.
 * @cur_up_down   Index of queue we work with (can be UP or DOWN).
 * @cur_seq       Current sequence number.
 * @last_seq      The sequence number of previous received item.
 * @new_seq       Is the received item from new sequence?
 */
void place_received_item(MPI_Item *recv, 
                         int *cur_up_down, 
                         unsigned int *cur_seq,
                         unsigned int *last_seq, 
                         bool *new_seq)
{
  QItem *new = create_qitem(recv);

  if (*cur_up_down == UP) {
    if (recv->seq == *cur_seq) {
      /* we are up and the receivec item is from the same sequnce as previous */
      queue_up(UP, new);
    } else {
      *last_seq = recv->seq;
      new->item->seq = *cur_seq;
      DPRINT("setting new item->seq=%d\n", *cur_seq);
      *cur_up_down = DOWN;
      queue_up(DOWN, new);
    }
  } else {
    if (recv->seq == *last_seq) {
      new->item->seq = *cur_seq;
      queue_up(DOWN, new);
    } else {
      *cur_up_down = UP;
      *last_seq = recv->seq;
      queue_up(UP, new);
      *new_seq = true;
      *cur_seq = recv->seq;
    }
  }
}

/* Compare first items of UP and DOWN queues and create send item from the
 * bigger one. The item will be used to send to next right processor. The bigger
 * item will be removed from the queue.
 */
MPI_Item *get_greater_item()
{
  MPI_Item *greater;

  QItem *first_up = TAILQ_FIRST(&up);
  QItem *first_down = TAILQ_FIRST(&down);

  if (first_up->item->val > first_down->item->val) {
    greater = create_mpi_item(first_up->item->val, first_up->item->seq);
    DPRINT("Value in UP queue is greater (removing val=%d\n)", first_up->item->val);
    TAILQ_FREE_ENTIRE_ITEM(up, first_up);
  } else {
    greater = create_mpi_item(first_down->item->val, first_down->item->seq);
    DPRINT("Value in DOWN queue is greater (removing val=%d\n)", first_down->item->val);
    TAILQ_FREE_ENTIRE_ITEM(down, first_down);
  }
  return greater;
}

/* Process can only work if this condition is true. Processor can compare two
 * values from its input queues if there is value in both DOWN and UP queue a 
 * the values are from the SAME sequence.
 */
bool compare_condition()
{
  QItem *first_up = TAILQ_FIRST(&up);
  QItem *first_down = TAILQ_FIRST(&down);

  if (!TAILQ_EMPTY(&up) && !TAILQ_EMPTY(&down) && (first_up->item->seq == first_down->item->seq))
    return true;
  else
    return false;
}

/**
 * example code from: 
 * http://www.guyrutenberg.com/2007/09/22/profiling-code-using-clock_gettime/
 */
void diff(struct timespec *start, struct timespec *end, struct timespec *temp) {
    if ((end->tv_nsec - start->tv_nsec) < 0) {
        temp->tv_sec = end->tv_sec - start->tv_sec - 1;
        temp->tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
    } else {
        temp->tv_sec = end->tv_sec - start->tv_sec;
        temp->tv_nsec = end->tv_nsec - start->tv_nsec;
    }
}

int main(int argc, char *argv[])
{
  int numprocs, n;      /* number of processes we work with */
  int myid;             /* ID of the processor */
  int c;                /* number from the file */
  int res;

  FILE *f;
  MPI_Status status; 
  MPI_Item *send, recv, *i;
  QItem *iterator, *tmp, *qi;
  struct head *queue;

  int up_len, down_len;
  int cur_up_down;  /* currently working with UP or DOWN queueu? */
  unsigned int cur_seq = 0;      /* what sequence does the process work with */
  unsigned int last_seq = 0;
  unsigned int tmp_seq;
  bool new_seq = false;
  bool measure = false;

  if (argc == 2 && (strcmp(argv[1], "-m") == 0)) {
    measure = true;
  }

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  /* create a new data type for struct MPI_Item */
  const int nitems=2;
  int blocklengths[2] = {1,1};
  MPI_Datatype types[2] = {MPI_UNSIGNED, MPI_UNSIGNED};
  MPI_Datatype mpi_qitem;
  MPI_Aint offsets[2];

  offsets[0] = offsetof(MPI_Item, val);
  offsets[1] = offsetof(MPI_Item, seq);

  MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_qitem);
  MPI_Type_commit(&mpi_qitem);

  if (myid == 0) {

    TAILQ_INIT(&in);

    if ((f = fopen(FILE_NAME, "r")) == NULL) {
      perror("fopen()");
      return 1;
    }

    /* read the input values and store them into the in queue */
    while ((c = fgetc(f)) != EOF) {
      printf("%d ", c);
      i = create_mpi_item(c, 0);
      qi = create_qitem(i);
      TAILQ_INSERT_TAIL(&in, qi, entries); i = NULL; qi = NULL;
    }
    putchar('\n');
    fflush(stdout);   /* write the input values before printing sorted values */

    if (fclose(f) == EOF) {
      perror("fclose()");
      return 0;
    }

    TAILQ_LENGTH(up_len, in, tmp, entries);
    if (up_len == 1) {
      /* if the file contains only one value then there is nothing to sort */
      printf("%d\n", TAILQ_FIRST(&in)->item->val);
    } else {
      struct timespec time1;

      if (measure) {
        clock_gettime(CLOCK_REALTIME, &time1);
      }

      TAILQ_FOREACH(tmp, &in, entries) {
        send = create_mpi_item(tmp->item->val, cur_seq);
        cur_seq++;
        MPI_Send(send, 1, mpi_qitem, 1, TAG, MPI_COMM_WORLD); 
        SEND_INFO(myid, send->val, send->seq);
      }

      if (measure) {
        /* wait until the last process end and ask for start time */
        MPI_Recv(&res, 1, MPI_INT, numprocs-1, TAG, MPI_COMM_WORLD, &status);
        MPI_Send(&(time1.tv_sec), 1, MPI_LONG_INT, numprocs-1, TAG, MPI_COMM_WORLD);
        MPI_Send(&(time1.tv_nsec), 1, MPI_LONG_INT, numprocs-1, TAG, MPI_COMM_WORLD);
      }
    }
  } 
  else {

    TAILQ_INIT(&down);
    TAILQ_INIT(&up);
    if (myid == numprocs-1) { 
      TAILQ_INIT(&out);
    }

    cur_up_down = UP;
    cur_seq = 0;
    last_seq = 0;
    new_seq = false;
    res = 1;
    n = pow(2, (numprocs-1));   /* count the number of input numbers */

    while (n != 0) {
      /* until all the input number pass throught the process */

      TAILQ_LENGTH(up_len, up, tmp, entries);
      TAILQ_LENGTH(down_len, down, tmp, entries)

      DPRINT("P%d: WHILE(%d) up_len=%d dwn_len=%d cur_seq=%d last_seq=%d up_do=%s\n", 
              myid, n, up_len, down_len, last_seq, cur_seq, (cur_up_down == UP) ? "UP" : "DOWN");

      if ((up_len + down_len) != n) {
        MPI_Recv(&recv, 1, mpi_qitem, myid-1, TAG, MPI_COMM_WORLD, &status);
        RECV_INFO(myid, recv.val, recv.seq);
        place_received_item(&recv, &cur_up_down, &cur_seq, &last_seq, &new_seq);
        NEW_SEQ_FLAG_INFO(new_seq, myid, recv.val);
      } else {
        DPRINT("P%d: Skip receiving another item ...\n", myid);
      }

      queues_print(myid);

      if ( compare_condition() ) {
        /* COMPARE CONDITION: The processor has enough item in its input queues
         * so it can compare them. */

        DPRINT("P%d: Comparing condition\n", myid);

        /* First two items in UP and DOWN queues are of the same sequence so we
         * can compare them. */
        send = get_greater_item();

        if (myid == numprocs-1) {
          /* put the greater value into the final queue if I'm the last
           * processor */
          QUEUE_UP_FINAL(send->val, send->seq, qi);
        } else {
          /* send the greater one to the right */
          MPI_Send(send, 1, mpi_qitem, myid+1, TAG, MPI_COMM_WORLD);
          SEND_INFO(myid, send->val, send->seq);
        }

        new_seq = false;
        n--;
        DPRINT("P%d: WHILE CONTINUE(compare)\n", myid);
        continue;
      } 
      else {

        TAILQ_LENGTH(up_len, up, tmp, entries);
        TAILQ_LENGTH(down_len, down, tmp, entries);

        if (new_seq || up_len==n || down_len==n || up_len+down_len==n) {
          /* no more items is comming so I have to send everything from UP and
           * DOWN queues from the smallest sequence numbers */
          
          DPRINT("P%d: No more incoming items or new seq condition\n", myid);

          /* the first item has lower sequnce number */
          set_queue_with_lower_seq(queue, myid);
          /* all items with this seq number will be send */
          tmp_seq = TAILQ_FIRST(queue)->item->seq;
          DPRINT("P%d: Send all items with seq=%d\n", myid, tmp_seq);

          
          for (tmp=TAILQ_FIRST(queue); tmp != NULL; tmp = TAILQ_FIRST(queue)) {

            if (tmp_seq == tmp->item->seq) {

              if (myid == numprocs-1) {
                QUEUE_UP_FINAL(tmp->item->val, tmp->item->seq, qi);
              } else {
                DPRINT("P%d-->P%d: Sending: val=%d, seq=%d\n", 
                   myid, myid+1, tmp->item->val, tmp->item->seq);
                MPI_Send(create_mpi_item(tmp->item->val, tmp->item->seq), 
                       1, mpi_qitem, myid+1, TAG, MPI_COMM_WORLD);
              }
 
              TAILQ_FREE_ENTIRE_ITEM(*queue, tmp);
              n--;

            } else {
              DPRINT("Next item in the queue has different seq num\n");
              break;
            }
          }

          new_seq = false;
        } /* end last condition queue_len == N */

        new_seq = false;
      }

      DPRINT("P%d: WHILE AGAIN\n", myid);

    } /* while end */

    if (measure && myid == numprocs-1) {
      struct timespec time1, time2, d;
      clock_gettime(CLOCK_REALTIME, &time2);
      MPI_Send(&res, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD);
      MPI_Recv(&(time1.tv_sec), 1, MPI_LONG_INT, 0, TAG, MPI_COMM_WORLD, &status);
      MPI_Recv(&(time1.tv_nsec), 1, MPI_LONG_INT, 0, TAG, MPI_COMM_WORLD, &status);

      diff(&time1, &time2, &d);
      printf("time: %ld.%ld\n", d.tv_sec, d.tv_nsec);
    }
 
    /* Free the entire UP queue  */
    while ((iterator = TAILQ_FIRST(&up))) {
      TAILQ_FREE_ENTIRE_ITEM(up, iterator);
    }

    /* Free the entire UP queue  */
    while ((iterator = TAILQ_FIRST(&down))) {
      TAILQ_FREE_ENTIRE_ITEM(down, iterator);
    }

    if (myid == numprocs-1) {
      TAILQ_FOREACH_REVERSE(iterator, &out, head, entries) {
        printf("%d\n", iterator->item->val);
      }

      /* Free the entire tail queue  */
      while ((iterator = TAILQ_FIRST(&out))) {
        TAILQ_FREE_ENTIRE_ITEM(out, iterator);
      }
    }
       
    DPRINT("(x) P%d END\n", myid);
  }

  MPI_Type_free(&mpi_qitem);
  MPI_Finalize(); 
  return 0;
}
