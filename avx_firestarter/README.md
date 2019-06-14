### AVX frequency application and latencies

## Compilation

Requires:
* Score-P https://www.vi-hps.org/projects/score-p/ with PAPI support
* GCC with support for pthreads
* A kernel that allows measuring specific events

Build command
```make```

You can switch to non instrumented, which will result in a modified FIRESTARTER version, but no trace.


## Run

```./FIRESTARTER (options)```

* You can change the period and the percentage for low high via the usual knobs
* This will write a trace to the given SCOREP_EXPERIMENT_DIRECTORY, default sth like scorep-(date/time)/traces.otf2
