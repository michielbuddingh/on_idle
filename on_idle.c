// Copyright 2008-2017, Michiel Buddingh, All rights reserved.
// Use of this code is governed by version 2.0 or later of the Apache
// License, available at http://www.apache.org/licenses/LICENSE-2.0
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/wait.h>

#include <assert.h>

#include <xcb/xcb.h>
#include <xcb/xprint.h>
#include <xcb/screensaver.h>

#define MIN(a, b) (a > b ? b : a)
#define MAX(a, b) (a > b ? a : b)

static void exerror(const char * errmsg) {
	fprintf(stderr, "%s\n", errmsg);
	exit(EXIT_FAILURE);
}


static int input_line ( unsigned int * len, char ** line ) {
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
	char * command[4];
};

static int search_tasks (const int ntasks, const struct _task tasks[], unsigned long int delay) {
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


static int compare_tasks(const void * a, const void * b) {
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

static void get_input (unsigned int * tasknr, struct _task ** tasks) {

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
			(*tasks)[*tasknr].delay = (unsigned int) abs(delay);
			(*tasks)[*tasknr].command[0] = strdup("sh");
		        (*tasks)[*tasknr].command[1] = strdup("-c");
			(*tasks)[*tasknr].
			      command[2] =
				strndup(line + offset, (unsigned int) length - offset);
			(*tasks)[*tasknr].command[3] = NULL;
			(*tasks)[*tasknr].pid = -2;
			(*tasknr)++;
		} else if (d == 0) {
			fprintf(stderr, "Lines should be of the format <delay> <command>\n");
		}
		free(line);
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
	(void) argc;
	(void) argv;

	xcb_connection_t * xc = xcb_connect (NULL, NULL);

	if (xcb_connection_has_error(xc) > 0) {
		exerror("Could not open display\n");
	}

	{
		xcb_query_extension_reply_t const * extension_reply =
			xcb_get_extension_data (xc, &xcb_screensaver_id);

		if (!extension_reply->present) {
			xcb_disconnect (xc);
			exerror("XScreenSaver Extension not available\n");
		}
	}

	xcb_drawable_t root = xcb_setup_roots_iterator (xcb_get_setup (xc)).data->root;

	{
		struct _task * tasks;
		unsigned int tasknr;

		get_input(&tasknr, &tasks);

		if (tasknr == 0) {
			xcb_disconnect(xc);
			exit(EXIT_SUCCESS);
		}

		{
			unsigned long int last_wakeup = 0;
			long int sleepleft = tasks[0].delay;
			do {
				unsigned long int idle;
				unsigned int next_task;
				sleep((unsigned int) sleepleft);
				xcb_screensaver_query_info_reply_t * info =
					xcb_screensaver_query_info_reply (
						xc,
						xcb_screensaver_query_info_unchecked(
							xc,
							root),
						NULL);

				idle = info->ms_since_user_input / 1000;
				free(info);

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

	xcb_disconnect (xc);
	return EXIT_SUCCESS;
}
