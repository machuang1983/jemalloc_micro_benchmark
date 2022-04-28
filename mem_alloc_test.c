/*
 * jemalloc micro-benchmark 
 * test memory usage in different page size/huge page size system 
 * Martin Ma 2022
 */

#define N_TOTAL		500
#define N_THREADS	2
#define MEMORY		(1ULL << 26)
#define MSIZE		10000
#define PATH_MAX 2048

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <malloc.h>
#include <string.h>

struct bin
{
	unsigned char *ptr;
	size_t size;
};

int getPidStatus()
{
	char *line;
	char *vmsize;
	char *vmpeak;
	char *vmrss;
	char *vmhwm;
	
	size_t len;
	
	FILE *f;

	vmsize = NULL;
	vmpeak = NULL;
	vmrss = NULL;
	vmhwm = NULL;
	line = malloc(128);
	len = 128;
	pid_t pid;
	pid = getpid();

	char pidstatus[PATH_MAX];

	snprintf(pidstatus,PATH_MAX, "/proc/%d/status",(int)pid);
	//printf("pidstatus %s\n",pidstatus);

	f = fopen(pidstatus, "r");
	if (!f) return 1;
	
	/* Read memory size data from /proc/pid/status */
	while (!vmsize || !vmpeak || !vmrss || !vmhwm)
	{
		if (getline(&line, &len, f) == -1)
		{
			/* Some of the information isn't there, die */
			return 1;
		}
		//printf("line=%s",line);	
		/* Find VmPeak */
		if (!strncmp(line, "VmPeak:", 7))
		{
			vmpeak = strdup(&line[7]);
		}
		
		/* Find VmSize */
		else if (!strncmp(line, "VmSize:", 7))
		{
			vmsize = strdup(&line[7]);
		}
		
		/* Find VmRSS */
		else if (!strncmp(line, "VmRSS:", 6))
		{
			vmrss = strdup(&line[7]);
		}
		
		/* Find VmHWM */
		else if (!strncmp(line, "VmHWM:", 6))
		{
			vmhwm = strdup(&line[7]);
		}
	}
	free(line);
	
	fclose(f);

	/* Get rid of " kB\n"*/
	len = strlen(vmsize);
	vmsize[len - 4] = 0;
	len = strlen(vmpeak);
	vmpeak[len - 4] = 0;
	len = strlen(vmrss);
	vmrss[len - 4] = 0;
	len = strlen(vmhwm);
	vmhwm[len - 4] = 0;
	
	/* Output results to stderr */
	fprintf(stderr, "vmsize=%s KB vmpeak=%s KB vmrss=%s KB vmhwm=%s KB\n\n", vmsize, vmpeak, vmrss, vmhwm);
	
	free(vmpeak);
	free(vmsize);
	free(vmrss);
	free(vmhwm);
	
	/* Success */
	return 0;
}

/*
 * Allocate a bin with malloc().
 */
static void bin_alloc(struct bin *m, size_t size)
{
        if (m->size > 0) free(m->ptr);
        m->ptr = malloc(size);
        //printf("bin_alloc: malloc memory (size=%ld)!\n", (unsigned long)size);
	//getPidStatus();
	return;
}

/* Free a bin. */

static void bin_free(struct bin *m)
{
	if (!m->size) return;

	free(m->ptr);
	m->size = 0;
}

struct bin_info
{
	struct bin *m;
	size_t size, bins;
};

struct thread_st
{
	int bins, flags,idx;
	size_t size;
	pthread_t id;
};

static int n_thread_sleep = 10;
static void *malloc_test(void *ptr)
{
	struct thread_st *st = ptr;
	int i, pid = 1;
	unsigned b, j, actions;
	struct bin_info p;

	p.m = malloc(st->bins * sizeof(*p.m));
	p.bins = st->bins;
	p.size = st->size;
	for (b = 0; b < p.bins; b++)
	{
		p.m[b].size = 0;
		p.m[b].ptr = NULL;
		bin_alloc(&p.m[b], p.size);
	}
	printf("malloc_test:malloc mem in thread %d tid %lx.\n", st->idx, (long)st->id);
	getPidStatus();
	
	for (b = 0; b < p.bins; b++) bin_free(&p.m[b]);
	
	free(p.m);
	//keep the thread for a while
        sleep(n_thread_sleep);

	if (pid > 0)
	{
		st->flags = 1;
	}
	return NULL;
}

static int my_start_thread(struct thread_st *st)
{
	pthread_create(&st->id, NULL, malloc_test, st);
	printf("my_start_thread: Created thread %d tid %lx.\n",st->idx, (long)st->id);
	getPidStatus();
	return 0;
}

static int n_total = 0;
static int n_total_max = N_TOTAL;
static int n_running;

static int my_end_thread(struct thread_st *st)
{
	/* Thread st has finished.  Start a new one. */
	if (n_total >= n_total_max)
	{
		n_running--;
	}
	else
	{
		n_total++;
	}
	//printf("my_end_thread: thread %d tid %lx n_total %d n_total_max %d n_running %d\n",st->idx, (long)st->id,n_total,n_total_max, n_running);
	//getPidStatus();
	return 0;
}

int main(int argc, char *argv[])
{
	int i, bins;
	int n_thr = N_THREADS;
	size_t size = MSIZE;
	struct thread_st *st;

	if (argc!=4) {
		printf("please input thread num, malloc size, bin number\n");
		printf("example: ./mem_alloc_test 10 100 10\n");
		return 0;
	}
	printf("main: original process status\n");
	getPidStatus();
	sleep(2);

        if (argc > 1) n_thr = atol(argv[1]);
	if (n_thr < 2) n_thr = 2;
        
	n_total_max = n_thr;
	n_thread_sleep = n_thr+1;

	if (argc > 2) size = atol(argv[2]);
	if (size < 2) size = 2;


	bins = MEMORY  /(size * n_thr);
	if (argc > 3) bins = atoi(argv[3]);
	if (bins < 4) bins = 4;


	printf("main: Using posix threads.\n");

	printf("main: threads=%d size=%ld bins=%d\n",
		   n_thr, size, bins);

	st = malloc(n_thr * sizeof(*st));
	if (!st) exit(-1);

	/* Start all n_thr threads. */
	for (i = 0; i < n_thr; i++)
	{
		st[i].bins = bins;
		st[i].size = size;
		st[i].flags = 0;
		st[i].idx = i;
		if (my_start_thread(&st[i]))
		{
			printf("main: Creating thread #%d failed.\n", i);
			n_thr = i;
			break;
		}
		//printf("main: Succes Created thread %d tid %lx.\n",i, (long)st[i].id);
	        //getPidStatus();
		sleep(1);
	}

	for (n_running = n_total = n_thr; n_running > 0;)
	{
		/* Wait for subthreads to finish. */
		for (i = 0; i < n_thr; i++)
		{
			if (st[i].flags)
			{
				pthread_join(st[i].id, NULL);
				st[i].flags = 0;
				my_end_thread(&st[i]);
			}
		}
	}

	free(st);
	
	printf("main: Done.\n");
	return 0;
}

