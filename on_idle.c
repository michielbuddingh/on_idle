#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/wait.h>

#include <assert.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)


void exerror(const char * errmsg) {
	fprintf(stderr, errmsg);
	exit(EXIT_FAILURE);
}


int input_line ( unsigned int * len, char ** line ) {
	unsigned int bufsize;
	int c;

	*len = 0;
	*line = malloc(512);
	assert(*line);
	bufsize = 512;

	while ((c = getchar())) {
		if (c == EOF) {
			if (ferror(stdin)) {
				exerror("Error reading from standard input");
			} else {
				*(*line + (*len)) = 0;
				return 0;
			}
		} else if (c == '\n') {
			break; /* while (c = getchar()) */
		} else {
			if (*len == (bufsize - 1)) {
				char * temp;
				bufsize += 512;
				if (!(temp = realloc(*line, bufsize))) {
					free(*line);
					exerror(strerror(errno));
				}
				*line = temp;
			}
			*(*line + (*len)++) = (char) c;
		}
	}
	
	*(*line + (*len)) = 0;

	return 1;
}

struct _task {
	unsigned int delay;
	pid_t pid;
	const char * command[4];
};

int search_tasks (const unsigned int ntasks, const struct _task tasks[], unsigned int delay) {
	int l = -1, h = ntasks;
	
	while ((h - l) > 1) {
		int half = (h + l) / 2;
		if (tasks[half].delay <= delay) {
			l = half;
		} else if (tasks[half].delay > delay) {
			h = half;
		}
	}
	return h;
}


int compare_tasks(const void * a, const void * b) {
	const struct _task * aa = (const struct _task *) a;
	const struct _task * bb = (const struct _task *) b;
	
	if (aa->delay > bb->delay) {
		return 1;
	} else if (aa->delay < bb->delay) {
		return -1;
	} else {
		return 0;
	}
}

void get_input (unsigned int * tasknr, struct _task ** tasks) {
	
	unsigned int taskbuf = 12;
	
	*tasknr = 0;
	* tasks = malloc(10 * sizeof(** tasks));
	assert(*tasks);

	while (1) {
		char * line = NULL;
		unsigned int length;
		int delay = 0;
		int offset;
		int d, c = input_line(&length, &line);
		if ((d = sscanf(line, "%d %n", &delay, &offset)) > 0) {
			(*tasks)[*tasknr].delay = abs(delay);
			(*tasks)[*tasknr].command[0] = "sh";
		        (*tasks)[*tasknr].command[1] = "-c";
			(*tasks)[*tasknr].
			      command[2] = 
				strndup(line + offset, (unsigned int) length - offset);
			(*tasks)[*tasknr].command[3] = NULL;
			(*tasks)[*tasknr].pid = -2;
			(*tasknr)++;
		} else if (d == 0) {
			fprintf(stderr, "Lines should be of the format <delay> <command>\n");
		}
		if (!c) {
			break;
		}
		if (*tasknr == taskbuf) {
			taskbuf += 12;
			if (!(*tasks = realloc(*tasks, taskbuf * sizeof(** tasks)))) {
				exerror(strerror(errno));
			}
			
		}
	}

	qsort(*tasks, *tasknr, sizeof(** tasks), &(compare_tasks));
}


int main(int argc, char ** argv) {
	int sleeptime = 5;
	XScreenSaverInfo * xsi;
	Display *d;

	(void) argc;
	(void) argv;

	
	d = XOpenDisplay(NULL);

	if (!d) {
		exerror("Could not open display\n");
	}

	{
		int event_no, voidno;
		if (!(XScreenSaverQueryExtension(d, &event_no, &voidno))) {
			XCloseDisplay(d);
			exerror("XScreenSaver Extension not available\n");
		}
	}

	{
		struct _task * tasks;
		unsigned int tasknr;
		
		get_input(&tasknr, &tasks);

		if (tasknr == 0) {
			XCloseDisplay(d);
			exit(EXIT_SUCCESS);
		}
	
		assert(xsi = XScreenSaverAllocInfo());
	
		{
			unsigned int last_wakeup = 0;
			int sleepleft = tasks[0].delay;
			do {
				unsigned int idle, next_task;
				sleep((unsigned int) sleepleft);
				XScreenSaverQueryInfo(d, DefaultRootWindow(d), xsi);

				idle = xsi->idle / 1000;
				next_task = search_tasks(tasknr, tasks, idle);

				if (last_wakeup > idle) {
					last_wakeup = 0;
				}
				{
					int c = next_task - 1;
					while ((c >= 0) && (tasks[c].delay > last_wakeup)) {
						if (!(wait4(tasks[c].pid,
							    NULL,
							    WNOHANG,
							    NULL))) {
							c--;
							continue;
						} else {
							tasks[c].pid = -2;
						}
						
						tasks[c].pid = fork();
						if (!(tasks[c].pid)) {
							execvp("sh",
							       tasks[c].command);
						}
						c--;
					}
					last_wakeup = idle;
				}

				
				if (next_task < tasknr) {
					sleepleft = MAX(MIN(tasks[next_task].delay - idle,
							    tasks[0].delay), 1);
				} else {
					sleepleft = tasks[0].delay;
				}
			} while (sleepleft > 0);
		}
	}
		

	XFree(xsi);

	XCloseDisplay(d);
	return EXIT_SUCCESS;
}
