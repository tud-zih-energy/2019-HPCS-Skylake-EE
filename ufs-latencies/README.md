### Uncore Frequency Scaling Test
Test for getting the uncore frequency switching latencies.

## Compilation
Requirements:
- GCC compiler

You should look into the code and change the tested uncore frequencies (we used the ones available at our system)

```gcc latency_test.c -o latency_test```

## Run

Requirements
* The program should run at CPU 0 (you can change the source code if you want to change the used MSRs)
* The kernel module msr should be loaded (`sudo modprobe msr`)
* You need root access

```sudo taskset -c 0 ./latency_test > result.log```

The resulting log should include the time that a switch needs and the average performance before and after the switch
