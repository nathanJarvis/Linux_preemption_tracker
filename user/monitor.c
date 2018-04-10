#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sched_monitor.h>

int 
main(int     argc,
     char ** argv)
{

    int fd, status, i;
	preemption_info_t buf;

    fd = open(DEV_NAME, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Could not open %s: %s\n", DEV_NAME, strerror(errno));
        return -1;
    }
    
    status = enable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not enable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }
    sleep(3);

    status = disable_preemption_tracking(fd);
    if (status < 0) {
        fprintf(stderr, "Could not disable preemption tracking on %s: %s\n", DEV_NAME, strerror(errno));
        close(fd);
        return -1;
    }
	i = 1;
	while(read(fd, &buf, sizeof(buf)) > 0) {
		printf("Event %d\n", i);
		printf("\tTime off: %llu ms. Time on: %llu ms.\n", (buf.time_off/1000), (buf.time_on/1000));
		printf("\tScheduled on core %d\n", buf.cpu);
	 	printf("\tPreempted by %s\n", &buf.preempted_by);
		++i;
	}
	
    close(fd);

    printf("Monitor ran to completion!\n");

    return 0;
}
