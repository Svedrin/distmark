#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#if RAND_MAX < 32
#error "Your OS's randomizer sucks."
#endif

int lastops;
int ops;

void printstats(int sig){
	printf("%d\n", ops - lastops);
	lastops = ops;
	signal(SIGALRM, printstats);
	alarm(1);
}

int main(int argc, char **argv){
	if( argc < 2 ){
		printf("Usage: %s <testfile>\n", argv[0]);
		return 1;
	}
	
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
	
	printf("Using file '%s'.\n", argv[1]);
	fd = open(argv[1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
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
