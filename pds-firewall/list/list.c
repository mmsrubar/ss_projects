/*
 * INIT_LIST_HEAD() - inicializace seznamu
 *
 * list_add         - Insert a new entry AFTER the specified head
 * list_add_tail    - Insert a new entry before the specified head
 * list_del         - deletes entry from list
 * list_empty       - tests whether a list is empty
 * list_splice      - join two lists
 *
 * list_for_each	    -	iterate over a list
 * list_for_each_prev	-	iterate over a list backwards
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 *    list_entry      - get the data struct for this entry
 *
 * list_for_each_entry	    -	iterate over list of given type
 * list_for_each_entry_safe - iterate over list of given type safe against
 *                            removal of list entry
 */
#include <stdio.h>
#include <stdlib.h>
#include "list.h"

struct list_item {
  int val;
	struct list_head list;
};

int main(int argc, char **argv)
{
	struct list_item *new, *item, *tmp;
	struct list_head *pos, *q;
	unsigned int i;

  /* LIST INITIALIZATION 
   * ===================
   * Initialize the HEAD item of the list. This will set next and prev pointers
   * to the HEAD item. This HEAD item serves as the HEAD of the list and also as
   * a  stop for loop operation. This HEAD structure is not meant to store any
   * usefull user data! The list is still empty after this initialization! */
	struct list_item mylist;
	INIT_LIST_HEAD(&mylist.list);
  printf("Is the list empty? %s\n\n", list_empty(&mylist.list) ? "yes" : "no");

  /* ADD A FEW ITEMS AT THE END OF THE LIST 
   * ======================================
   * mylist.list is address of the HEAD but you can also give it a different
   * item (somewhere in the middle for example) and it will put the new item
   * after the one you specify in second argument of list_for_each_entry */
  new = (struct list_item *)malloc(sizeof(struct list_item));
  /* check return value! */
  new->val = 10; 
  list_add_tail(&(new->list), &(mylist.list));

  new = (struct list_item *)malloc(sizeof(struct list_item));
  new->val = 20; 
  list_add_tail(&(new->list), &(mylist.list));

  new = (struct list_item *)malloc(sizeof(struct list_item));
  new->val = 30; 
  list_add_tail(&(new->list), &(mylist.list));

  new = (struct list_item *)malloc(sizeof(struct list_item));
  new->val = 40; 
  list_add_tail(&(new->list), &(mylist.list));

  new = (struct list_item *)malloc(sizeof(struct list_item));
  new->val = 50; 
  list_add_tail(&(new->list), &(mylist.list));

  /* PRINT CONTENT OF THE VARIABLE VAL OF ALL ITEMS IN THE LIST FROM THE
   * BEGINING TO THE END 
   * Head item is a sentinel and its data won't be readed. */
  printf("Iterate with list_for_each_entry()\n");
  list_for_each_entry(item, &(mylist.list), list) {
    printf("val: %d\n", item->val);
  }
  putchar('\n');

	
	/* list_for_each() is a macro for a for loop. 
	 * first parameter is used as the counter in for loop. in other words, inside the
	 * loop it points to the current item's list_head.
	 * second parameter is the pointer to the list. it is not manipulated by the macro.
	 */
  printf("Iterate with list_for_each()\n");
  list_for_each(pos, &(mylist.list)) {  /* pos as a position in the list */
    item = list_entry(pos, struct list_item, list);
    printf("val: %d\n", item->val);
  }
  putchar('\n');

  printf("Delete item with val=30 using list_for_each_entry_safe()\n");
  /* always you safe version if you deleting! */
  list_for_each_entry_safe(item, tmp, &mylist.list, list) {
    if (item->val == 30)
      list_del(&(new->list));
  }
  putchar('\n');

  printf("Iterate with list_for_each_entry()\n");
  list_for_each_entry(item, &(mylist.list), list) {
    printf("val: %d\n", item->val);
  }
  putchar('\n');

	/* now let's be good and free the list_item items. since we will be removing items
	 * off the list using list_del() we need to use a safer version of the list_for_each() 
	 * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
	 * involves deletions of items (or moving items from one list to another). */
	printf("deleting the list using list_for_each_safe()\n");
	list_for_each_safe(pos, q, &mylist.list){
    item = list_entry(pos, struct list_item, list);
    list_del(pos);  /* 1st remove item from the list */
    free(item);     /* 2nd free the entire item */
	}

  printf("Iterate with list_for_each_entry()\n");
  list_for_each_entry(item, &(mylist.list), list) {
    printf("val: %d\n", item->val);
  }

  printf("Is the list empty? %s\n", list_empty(&mylist.list) ? "yes" : "no");

	return 0;
}
