#define LWT_NULL NULL

typedef struct __lwt_t * lwt_t;
typedef void *(*lwt_fn_t)(void *);

typedef enum {
	/* Thread is currently running */
	RUNNING,
	/* Thread is blocked and not running */
	BLOCKED,
	/* Thread is dead and needs to be joined */
	DEAD,
	/* Thread is blocked and waiting to join */
	JOINING,
	/* Thread is joining but grandchildren are alive */
	DEAD_RUNNING,
	/* Thread has been joined */
	FREE
} states;

typedef enum {
	LWT_INFO_NTHD_RUNNABLE,
	LWT_INFO_NTHD_ZOMBIES,
	LWT_INFO_NTHD_BLOCKED
} info_flags;

struct __lwt_t {
	void *eip;
	void **esp;
	void **ebp;
	/* Order of struct organization no longer matters
	   for lwt_asm.S */
	states state;
	int id;
	void *return_val;
	/* Pointer of originally malloced memory */
	void *memory;
	struct __lwt_t *parent;
	struct __lwt_t *previous;
	struct __lwt_t *next;
	/* Stack located below */
};

/* Will eventually overflow -- warning */
static unsigned int id_counter = 0;
/* Padding */
static struct __lwt_t *head = NULL;
/* Not static so the linker can link lwt_asm.S with this variable */
struct __lwt_t *current = NULL;

extern void __lwt_trampoline(void);
extern void __lwt_dispatch(struct __lwt_t *next, struct __lwt_t *current);

/*
 * Caller of __lwt_schedule is forced to handle the state of
 * the current thread.  This removes some unnecessary branches. */
void __lwt_schedule(void)
{
	struct __lwt_t *next;
	/* Linked list handling */
	if (current->next == NULL)
		next = head;
	else
		next = current->next;

	/* Skip everything except for BLOCKED threads;
	   No threads should be RUNNING other than current.*/
	while (next->state != BLOCKED) {
		/* There has to be a (grand)child that is never
		   in the JOINING state. */
		if (next->next == NULL)
			next = head;
		else
			next = next->next;
	}
	next->state = RUNNING;
	__lwt_dispatch(next, current);
}

void * lwt_join(struct __lwt_t *tcb)
{
	/* Check if we're its parent! */
	if (tcb->parent != current) {
		return NULL;
	}
	if (tcb->state == DEAD) {
		void *return_val = tcb->return_val;
		if (tcb == head)
			head = tcb->next;
		if (tcb->previous)
			tcb->previous->next = tcb->next;
		if (tcb->next)
			tcb->next->previous = tcb->previous;
		
		/* tcb was a manufactured address */
		free(tcb->memory);

		return return_val;
	}

	current->state = BLOCKED;
	__lwt_schedule();
	/* If we get put back on the run queue, then our child
	   has joined.  */
	return lwt_join(tcb);
}

void lwt_die(void *data)
{
	current->return_val = data;
	current->state = DEAD;
	/* Parent may be blocked indefinitely waiting to join, put
	   the parent back on the run queue. */
	current->parent->state = BLOCKED;
	__lwt_schedule();
}

lwt_t lwt_create(lwt_fn_t fn, void *data)
{
	struct __lwt_t *tcb;
	void *stack, *memory;
	int i;
	
	memory = malloc(4096);
	/* This address is the start of the thread control block.
	   The stack starts at one word less.  Stack grows down,
	   while thread control block grows up. */
	tcb = memory + 4095 - sizeof(struct __lwt_t);
	stack = tcb - sizeof(void *);

	/* If this is the first thread we're creating, we need
	   to create a thread control block, tcb, for the parent. */
	if (!head) {
		/* Parent already has a stack. */
		struct __lwt_t *tcb_parent = malloc(sizeof(struct __lwt_t));
		tcb_parent->eip = NULL;
		tcb_parent->esp = NULL;
		tcb_parent->ebp = NULL;
		tcb_parent->state = RUNNING;
		/* Integer overflow -- warning */
		tcb_parent->id = id_counter++;
		tcb_parent->return_val = NULL;
		tcb_parent->memory = NULL;
		tcb_parent->parent = NULL;
		tcb_parent->previous = tcb;
		tcb_parent->next = NULL;
		head = tcb_parent;
		current = tcb_parent;
	}

	/* Offset to write the parameters */
	tcb->esp = stack - 2 * sizeof(void *);
	/* Write the two parameters in */
	tcb->esp[0] = fn;
	tcb->esp[1] = data;

	tcb->ebp = stack;
	tcb->eip = __lwt_trampoline;
	tcb->state = BLOCKED;
	tcb->id = id_counter++;
	tcb->return_val = NULL;
	tcb->memory = memory;
	tcb->parent = current;

	tcb->previous = NULL;
	tcb->next = head;
	head->previous = tcb;
	head = tcb;

	return (lwt_t) tcb;
}

void lwt_yield(lwt_t tcb)
{
	if (tcb == LWT_NULL) {
		current->state = BLOCKED;
		__lwt_schedule();
	} else {
		/* tcb's state needs to be modified here as dispatch
		   does not deal with that. */
		current->state = BLOCKED;
		tcb->state = RUNNING;
		__lwt_dispatch(tcb, current);
	}
}

lwt_t lwt_current(void)
{
	return (lwt_t) current;
}

int lwt_id(lwt_t tcb)
{
	return ((struct __lwt_t *)tcb)->id;
}

void __lwt_start(lwt_fn_t fn, void *data)
{
	void * return_val;
	return_val = fn(data);
	lwt_die(return_val);
}

int lwt_info(info_flags flag)
{
	struct __lwt_t *cursor = head;
	int return_val = 0;

	switch (flag) {
	case LWT_INFO_NTHD_RUNNABLE:
		return_val = 0;
		while (cursor) {
			if (cursor->state == RUNNING)
				return_val++;
			cursor = cursor->next;
		}
		break;
	case LWT_INFO_NTHD_ZOMBIES:
		return_val = 0;
		while (cursor) {
			if (cursor->state == DEAD)
				return_val++;
			cursor = cursor->next;
		}
		break;
	case LWT_INFO_NTHD_BLOCKED:
		return_val = 0;
		while (cursor) {
			if (cursor->state == BLOCKED)
				return_val++;
			cursor = cursor->next;
		}
		break;
	}

	printf("Flag %d: %d.\n", flag, return_val);
	return return_val;
}
