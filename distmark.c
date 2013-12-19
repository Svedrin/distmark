#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#if RAND_MAX < 32
#error "Your OS's randomizer sucks."
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int numprocs;
int lastops;
int ops;
int *child_pid;
int *child_pipe;
int pipefd[2];

void printstats(int sig){
	int currops = ops - lastops;
	write(pipefd[1], &currops, sizeof(int));
	lastops = ops;
	signal(SIGALRM, printstats);
	alarm(1);
}

void kill_children(int sig) {
	int i = 0, status;
	signal(SIGTERM, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	while(i < numprocs) {
		if(child_pid[i] != 0){
			kill(child_pid[i], SIGINT);
			waitpid(child_pid[i], &status, 0);
		}
		i++;
	}
	free(child_pid);
	kill(getpid(), SIGINT);
}

int main(int argc, char **argv){
	if( argc < 2 || argc > 3 ){
		printf("Usage: %s <directory> [<numprocs = 4>]\n", argv[0]);
		return 1;
	}
	numprocs = ( argc >= 3 ? atoi(argv[2]) : 4 );
	if(numprocs <= 0) {
		printf("Number of processes must be at least 1!\n");
		return 1;
	}
	child_pid  = malloc( sizeof(int) * numprocs );
	child_pipe = malloc( sizeof(int) * numprocs );
	int i=0;
	while(i < numprocs) {
		pipe2(pipefd, O_NONBLOCK);
		child_pid[i] = fork();
		if(child_pid[i] == 0) {
			int fd, dataidx;
			unsigned int maxblock;
			off_t pos;
			char srsdata[32 * 4096];
			
			lastops = 0;
			ops = 0;
			
			close(pipefd[0]);
			
			fd = open("/dev/urandom", O_RDONLY);
			read(fd, srsdata, 32 * 4096);
			close(fd);
			
			srand(time(NULL));
			char testfile[256];
			snprintf(testfile, 256, "%s/test%d.img", argv[1], i);
			printf("Using file '%s'.\n", testfile);
			fd = open(testfile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			if( fd == -1 ){
				perror("open() failed");
				return 1;
			}
			
			maxblock = 10737418240 / 4096; /* 10GB */
			signal(SIGALRM, printstats);
			alarm(1);
			while(1){
				pos = (rand() % maxblock) * 4096;
				if( lseek(fd, pos, SEEK_SET) == -1 ){
					fprintf(stderr, "lseek(%d, %ld) failed: ", fd, pos);
					perror("");
					close(fd);
					close(pipefd[1]);
					return 1;
				}
				dataidx = rand() & 31; /* select record to write */
				if( write(fd, srsdata + (dataidx * 4096), 4096) == -1 ){
					perror("write() failed");
					close(fd);
					close(pipefd[1]);
					return 1;
				}
				++ops;
			}
			close(fd);
			close(pipefd[1]);
			return 0;
		}
		else if(child_pid[i] < 0) {
			perror("fork() failed");
			return 1;
		}
		child_pipe[i] = pipefd[0];
		close(pipefd[1]);
		i++;
	}
	
	signal(SIGTERM, kill_children);
	signal(SIGINT,  kill_children);
	
	struct timeval tv;
	int *child_iops = malloc( sizeof(int) * numprocs );
	int maxfd;
	int selectret;
	fd_set selectfds;
	
	for( i = 0; i < numprocs; i++ ){
		child_iops[i] = -1;
	}
	
	while(1){
		maxfd = 0;
		FD_ZERO(&selectfds);
		
		for( i = 0; i < numprocs; i++ ){
			if( child_iops[i] == -1 ){
				FD_SET(child_pipe[i], &selectfds);
				maxfd = MAX(child_pipe[i], maxfd);
			}
		}
		
		tv.tv_sec  = 1;
		tv.tv_usec = 1000;
		
		selectret = select(maxfd + 1, &selectfds, NULL, NULL, &tv);
		if( selectret == -1 ){
			perror("select()");
		}
		else if(selectret > 0){
			for( i = 0; i < numprocs; i++ ){
				if( FD_ISSET(child_pipe[i], &selectfds) ){
					read(child_pipe[i], &child_iops[i], sizeof(int));
				}
			}
			
			int haveall = 1;
			for( i = 0; i < numprocs; i++ ){
				if( child_iops[i] == -1 ){
					haveall = 0;
					break;
				}
			}
			
			int iopssum = 0;
			if( haveall ){
				for( i = 0; i < numprocs; i++ ){
					iopssum += child_iops[i];
					printf("%6d\t", child_iops[i]);
				}
				printf("sum=%-6d avg=%-6d\n", iopssum, iopssum / numprocs);
				for( i = 0; i < numprocs; i++ ){
					child_iops[i] = -1;
				}
			}
		}
		else{
			// dafuq no dataz!?
			printf("timeout...\n");
		}
		
	}
	
	return 0;
}
