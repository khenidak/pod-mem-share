## pod mem share

Linux allows sharing memory between processes via IPC. The same can be applied to Kubernetes pods, this works for:

1. Minimizing the resource used by pods by having a single copy of whatever they all need. A classic usecase is set of static assets used by multiple workers deployed on the same note.  
2. Creating a producer/consumer style where a pods operate on a ring buffer a single pod creating work items and one or more pods can act as consumers for the work.

In this example the controller pod (deployed as a daemon-set) creates and mounts the memory as a shared memory segment (static asset use case) which is then mounted by consumers (deployed as a Kubernetes deployment). Signals to notify consumers not part of this example and is left as an exercise for reader.

> The example expects a file named `data.file` which will become the memory segment content 

