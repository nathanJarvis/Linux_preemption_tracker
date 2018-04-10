#ifndef __SCHED_MONITOR_H
#define __SCHED_MONITOR_H

/* Seen by kernel and user */
#define SCHED_MONITOR_MODULE_NAME "sched_monitor"

/* ioctl commands to enable/disable tracking */
#define ENABLE_TRACKING     0xdeadbeef
#define DISABLE_TRACKING    0xdeaddead

/* Data structure for user<->kernel transfer */
typedef struct preemption_info {
    /* populate with info to transfer from kernel to user */
    int cpu;
    unsigned long long time_on;
    unsigned long long time_off;
    char preempted_by[16];
} __attribute__((packed)) preemption_info_t;


#define DEV_NAME "/dev/" SCHED_MONITOR_MODULE_NAME

#ifndef __KERNEL__

/* Only seen by user-space */

#include <sys/ioctl.h>

/* Functions to enable/disable tracking */
static inline int
enable_preemption_tracking(int fd)
{
    return ioctl(fd, ENABLE_TRACKING, 0);
}

static inline int
disable_preemption_tracking(int fd)
{
    return ioctl(fd, DISABLE_TRACKING, 0);
}
#endif

#endif
