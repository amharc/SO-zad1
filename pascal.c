// Krzysztof Pszeniczny (347208)
// Systemy operacyjne 2014/2015
// Zadanie 1
#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "err.h"

static int n;

// Passess values from descriptor from to descriptor to, until eof
void pass_values(int id, int from, int to) {
	uint64_t recv;

	while(true) {
		switch(read(from, &recv, sizeof(recv))) {
			case -1:
				syserr("Worker %d: Unable to read", id);
			case 0:
				return;
			default:
				if(-1 == write(to, &recv, sizeof(recv)))
					syserr("Worker %d: Unable to write", id);
		}
	}
}

// Common interface of pascal and worker
// -----------------------------------------------------
// Arguments:
// id -- identifier of the process
//       (0 for pascal, 1..n for worker)
// left[0] -- read end of the pipe leading from the previous process
// left[1] -- write end of the pipe leading to the previous process
// right[0], right[1] -- as above, but the next process
// if there is no previous/next process, left/right is NULL
//
// Postcondition:
// function closes left[0..1], right[0..1]
void worker(int id, int left[2], int right[2]) {
	uint64_t value = 1, recv;

	for(int iter = 1; true; ++iter) {
		switch(read(left[0], &recv, sizeof(recv))) {
			case -1:
				syserr("Worker %d: Unable to read", id);
			case 0: // EOF: pass values, close pipes and return
				if(right != NULL && -1 == close(right[1]))
					syserr("Worker %d: Unable to close", id);

				if(-1 == write(left[1], &value, sizeof(value)))
					syserr("Worker %d: Unable to write", id);

				if(right != NULL) {
					pass_values(id, right[0], left[1]);

					if(-1 == close(right[0]))
						syserr("Worker %d: Unable to close", id);
				}

				if(-1 == close(left[0]) || -1 == close(left[1]))
					syserr("Worker %d: Unable to close", id);
				return;
			default:
				value += recv;
				if(id + iter != n && right != NULL &&
				   -1 == write(right[1], &value, sizeof(value)))
					syserr("Worker %d: Unable to write", id);
		}
	}
}

void pascal(int id, int left[2], int right[2]) {
	uint64_t value = 0;

	assert(id == 0);
	assert(left == NULL);

	for(int i = 1; i < n; ++i)
		if(-1 == write(right[1], &value, sizeof(value)))
			syserr("Pascal: Unable to write");

	if(-1 == close(right[1]))
		syserr("Pascal: Unable to close");

	for(int i = 0; i < n; ++i)
		switch(read(right[0], &value, sizeof(value))) {
			case -1:
				syserr("Pascal: Unable to read");
			case 0:
				fatal("Pascal: Unexpected end of pipe");
			default:
				printf("%" PRIu64 " ", value);
		}

	putchar('\n');

	if(-1 == close(right[0]))
		syserr("Pascal: Unable to close");
}

// Function to create processes. It does not return.
_Noreturn void create_procs(void) {
	// left[0], left[1] -- read and write end of the pipe leading to the
	// previous process
	int left[2];

	for(int i = 0; i < n; ++i) {
		int pipes_to[2], pipes_from[2], right[2];

		if(-1 == pipe(pipes_to) || -1 == pipe(pipes_from))
			syserr("Creating processes failed: Unable to create pipe");

		pid_t pid;
		switch(pid = fork()) {
			case -1:
				syserr("Creating processes failed: Unable to fork");
			case 0:
				if(-1 == close(pipes_to[1]) || -1 == close(pipes_from[0]))
					syserr("Creating processes failed: Unable to close");

				if(i > 0 && (-1 == close(left[0]) || -1 == close(left[1])))
					syserr("Creating processes failed: Unable to close");

				left[0] = pipes_to[0];
				left[1] = pipes_from[1];

				break; // Loop to create the next process
			default: // Run the i-th process
				right[0] = pipes_from[0];
				right[1] = pipes_to[1];

				if(-1 == close(pipes_to[0]) || -1 == close(pipes_from[1]))
					syserr("Creating processes failed: Unable to close");

				if(i == 0)
					pascal(i, NULL, right);
				else
					worker(i, left, right);

				if(pid != waitpid(pid, NULL, 0))
					syserr("Ending processes failed: Unable to wait");

				exit(0);
		}
	}

	// Run the last process
	worker(n, left, NULL);
	exit(0);
}

_Noreturn void usage(const char *name) {
	fprintf(stderr, "USAGE: %s ROW_NUMBER\n", name);
	exit(1);
}

int main(int argc, char **argv) {
	if(argc != 2)
		usage(argv[0]);

	n = atoi(argv[1]);

	if(n <= 0)
		usage(argv[0]);

	create_procs();
}
