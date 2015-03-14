#ifndef PMS_H
#define PMS_H

#define TAG       0
#define FILE_NAME "numbers"

#define UP        1
#define DOWN      2

#ifdef DEBUG
  #define DPRINT(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )
#else
  #define DPRINT(...) do{ } while ( false )
#endif

#define	TAILQ_LENGTH(count, head, tmp, entries)	  \
  count = 0;                                  \
  TAILQ_FOREACH(tmp, &head, entries) {        \
      count++;                                \
  }                                           \
  tmp = NULL;

#define TAILQ_FREE_ENTIRE_ITEM(queue, member) \
  TAILQ_REMOVE((&queue), member, entries);    \
    free(member->item);                       \
    free(member);                             \

#define QUEUE_UP_FINAL(val, seq, qitem)             \
  qitem = create_qitem(create_mpi_item(val, seq));  \
  TAILQ_INSERT_TAIL(&final, qitem, entries);
 

#define set_queue_with_lower_seq(queue, id) \
  if (TAILQ_EMPTY(&up)) {\
    queue = &down;\
    DPRINT("P%d: DOWN queue has item with lower seq=%d\n", id, (TAILQ_FIRST(queue))->item->seq);  \
  } else if (TAILQ_EMPTY(&down)) {\
    queue = (struct head *) &up;  \
    DPRINT("P%d: UP queue has item with lower seq=%d\n", id, (TAILQ_FIRST(queue))->item->seq);  \
  } else if (TAILQ_FIRST(&up)->item->seq < TAILQ_FIRST(&down)->item->seq) {\
    queue = (struct head *) &up; \
    DPRINT("P%d: UP queue has item with lower seq=%d\n", id, (TAILQ_FIRST(queue))->item->seq);  \
  } else {\
    queue = &down;\
    DPRINT("P%d: DOWN queue has item with lower seq=%d\n", id, (TAILQ_FIRST(queue))->item->seq);  \
  }

#define RECV_INFO(id, val, seq) {\
  DPRINT("P%d: Receiving: val=%d, seq=%d\n", id, val, seq); \
}

#define SEND_INFO(myid, val, seq) { \
  DPRINT("P%d-->P%d: Sending: val=%d, seq=%d \n", myid, 1, val, seq); \
}

#define NEW_SEQ_FLAG_INFO(flag, id, val) {\
  if (flag) DPRINT("P%d: item val=%d -> NEW SEQ\n", id, val); \
}


#endif	// PMS_H
