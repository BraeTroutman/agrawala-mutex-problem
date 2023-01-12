CSC-641 - Lab 4 - Agrawala's Mutual Exclusion Problem
===========================

In the paper [An Optimal Algorithm for Mutual Exclusion in Computer Networks](https://dl.acm.org/doi/pdf/10.1145/358527.358537), Ricart and Agrawala define an algorithm for ensuring mutual exlusion of a shared medium by any number of nodes on a distributed networked computer system, without requiring any shared memory between the nodes. This project is an implementation thereof.

This lab simulates the behavior of 5 hosts communicating on a network, and coordinating the mutually-exclusive use
of a shared resource: the print server.

To compile the source for this lab, enter the top-level directory of the project (the one containing this README)
and run make in the command line.

Then, to run the simulation, execute the file 'run'-- you may need to update the file permissions. To halt the
simulation's execution, enter Ctrl-C, and cleanup the background processes that are still running using the cleanup script

Example execution:

```
	$ make
	$ ./run
	...program output...
	$ ./cleanup
```

