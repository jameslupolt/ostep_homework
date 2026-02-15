\
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11

all: timer_precision syscall_cost ctxswitch_cost

timer_precision: timer_precision.c common.h
	$(CC) $(CFLAGS) -o $@ timer_precision.c

syscall_cost: syscall_cost.c common.h
	$(CC) $(CFLAGS) -o $@ syscall_cost.c

ctxswitch_cost: ctxswitch_cost.c common.h
	$(CC) $(CFLAGS) -o $@ ctxswitch_cost.c

clean:
	rm -f timer_precision syscall_cost ctxswitch_cost
