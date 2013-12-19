#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#if RAND_MAX < 32
#error "Your OS's randomizer sucks."
#endif

int numprocs;
int lastops;
int ops;
int *child_pid;

void printstats(int sig){
	printf("%d\n", ops - lastops);
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
	child_pid = malloc( sizeof(int) * numprocs );
	int i=0;
	while(i < numprocs) {
		child_pid[i] = fork();
		if(child_pid[i] == 0) {
			int fd, dataidx;
			unsigned int maxblock;
			off_t pos;
			char srsdata[32 * 4096];
			
			lastops = 0;
			ops = 0;
			
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
					return 1;
				}
				dataidx = rand() & 31; /* select record to write */
				if( write(fd, srsdata + (dataidx * 4096), 4096) == -1 ){
					perror("write() failed");
					close(fd);
					return 1;
				}
				++ops;
			}
			close(fd);
			return 0;
		}
		else if(child_pid[i] < 0) {
			perror("fork() failed");
			return 1;
		}
		i++;
	}
	
	printf("signal() in %d\n", getpid());
	signal(SIGTERM, kill_children);
	signal(SIGINT,  kill_children);
	
	while(1){
		sleep(99999999);
	}
	
	return 0;
}
