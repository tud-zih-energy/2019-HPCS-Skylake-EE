#define _GNU_SOURCE 1

#include <asm/unistd.h>
#include <fcntl.h>

#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>



#define L1_DATA_SIZE 16*1024
#define L3_DATA_SIZE 2*1024*1024
#define CACHE_LINE 64
#define RAND_NUM 1234567
#define MAX_CYCLES 20000

/**
 * Returns time stamp counter
 */
static __inline__ uint64_t rdtsc(void)
{
    uint32_t a, d;
    asm volatile("rdtsc" : "=a" (a), "=d" (d));
    return (uint64_t) a | (uint64_t) d << 32;

}

typedef void** test_buffer_t;

/**
 * Create a buffer for pointer chasing
 */

test_buffer_t create_buffer(size_t size)
{
    void** buffer = malloc(size);
    size_t nr_elements = (size / sizeof(void*));
    // only use every nth element (cache line)
    size_t sep_elements = CACHE_LINE / sizeof(void*);
    size_t nr_used_elements = nr_elements / sep_elements;
    void** list = malloc(nr_used_elements * sizeof(void*));
    for (size_t i = 0; i < nr_used_elements - 1; i++)
        list[i] = (void*) &buffer[(i + 1) * sep_elements];
    srand(RAND_NUM);
    void** current = buffer;
    size_t nr = 0;
    size_t max = nr_used_elements - 1;
    for (size_t i = 0; i < nr_used_elements - 1; i++)
    {
        int r = rand() % max;
        // set target for jump
        *current = list[r];
        // set new current
        current = list[r];
        // remove list[r] from list
        // end of list: nr_used_elements-1-(i-1)
        for (size_t j = r; j < max - 1; j++)
            list[j] = list[j + 1];
        max--;
        nr++;
    }
    *current = buffer;
    free(list);
    return buffer;
}

/**
 * Jump around nr_accesses times in pointer chasing buffer
 */

static __inline__ test_buffer_t run_buffer_plain(test_buffer_t buffer,
        size_t nr_accesses)
{
    size_t nr = 0;
    void * current = (void*) buffer;
    while (nr < nr_accesses)
    {
        current = (*(void**) current);
        nr++;
    }
    return current;
}

/**
 * Jump around nr_accesses times in pointer chasing buffer
 * Will take rdtsc for each access, will change UFS target register (given as fd) to target
 * Will check for first latency that takes longer then max_cycles cycles
 * Will report the time between write to UFS register until this gap as change
 * Will report the gap as duration
 * Will report the average latecies before/after the gap as report/after
 */

static __inline__ test_buffer_t run_buffer(test_buffer_t buffer,
        size_t nr_accesses, uint64_t max_cycles, uint64_t* before,
        uint64_t* change, uint64_t* duration, uint64_t* after, int fd,
        uint64_t target)
{
    size_t nr = 0;
    void * current = (void*) *buffer;

    if (target != 0)
        pwrite(fd, &target, 8, 0x620);

    uint64_t start = rdtsc();
    uint64_t last_reading = start;
    uint64_t current_reading, mid = 0, mid2, nr_before;
    while (nr < nr_accesses)
    {
        nr++;
        current = (*(void**) current);
        current_reading = rdtsc();
        if ((mid == 0) && ((current_reading - last_reading) > max_cycles))
        {
            mid = current_reading;
            mid2 = last_reading;
            nr_before = nr;
            *before = (mid2 - start) / nr_before;
            *duration = mid - mid2;
        }
        last_reading = current_reading;
    }
    if (mid == 0)
    {
        *before = 0;
        *after = 0;
        *change = 0, *duration = 0;
    } else
    {
        *after = (current_reading - mid) / (nr - nr_before);
//	printf("%lu %lu %lu %lu\n",(current_reading-mid),nr,nr_before,(mid2-start)/nr_before);
        *change = mid2 - start;
    }
    return current;
}

void main()
{
    // tested UFS frequencies
    uint64_t settings[] = { 0xc0c, 0xd0d, 0xe0e, 0xf0f, 0x1010, 0x1111, 0x1212,
            0x1313, 0x1414, 0x1515, 0x1616, 0x1717, 0x1818 };

    uint64_t default_setting= 0xc18;
    // nr of tested UFS frequencies
    int nr_settings = 13;

    // open fd for switching msr
    int msr_fd = open("/dev/cpu/0/msr", O_RDWR);
    // report it, should be > 0 ;)
    printf("fd=%d\n", msr_fd);

    // create pointer chasing buffers
    test_buffer_t l1_buffer = create_buffer( L1_DATA_SIZE );

    test_buffer_t l3_buffer = create_buffer( L3_DATA_SIZE );

    // nr of accesses for pointer chasing
    size_t nr = 100000;

    // gathered results
    uint64_t performance_before, performance_after, cycles_switch,
            cycles_duration;

    // for each source target combination:
    for (uint64_t source = 0; source < nr_settings; source++)
        for (uint64_t target = 0; target < nr_settings; target++)
        {
            if (source == target)
                continue;
            // repeat measurement 1000 times
            for (int i = 0; i < 1000; i++)
            {
                // set default
                pwrite(msr_fd, &settings[source], 8, 0x620);
                // run in default
                l3_buffer = run_buffer(l3_buffer, nr, MAX_CYCLES,
                        &performance_before, &cycles_switch, &cycles_duration,
                        &performance_after, msr_fd, 0);
                // switch to target and measure
                l3_buffer = run_buffer(l3_buffer, nr, MAX_CYCLES,
                        &performance_before, &cycles_switch, &cycles_duration,
                        &performance_after, msr_fd, settings[target]);
                printf(
                        "%d00 MHz->%d00Mhz Cycles per access before:%lu after:%lu, switch after %lu cycles, took %lu cycles\n",
                        0xFF & settings[source], 0xFF & settings[target],
                        performance_before, performance_after, cycles_switch,
                        cycles_duration);
            }
        }
    // set default again
    pwrite(msr_fd, &default_setting, 8, 0x620);
    for (int i = 0; i < 1000; i++)
    {
        // first train for local access (L1)
        l1_buffer = run_buffer(l1_buffer, nr*10000, MAX_CYCLES,
                &performance_before, &cycles_switch, &cycles_duration,
                &performance_after, msr_fd, 0);
        // then: go to offcore (L3)
        l3_buffer = run_buffer(l3_buffer, nr*1000, MAX_CYCLES,
                &performance_before, &cycles_switch, &cycles_duration,
                &performance_after, msr_fd, 0);
        // then: report performance stuff :)
        printf(
                "L1->L3 Cycles per access before:%lu after:%lu, switch after %lu cycles, took %lu cycles\n",
                performance_before, performance_after, cycles_switch,
                cycles_duration);
    }

}
