CSC-641 - Lab 4 - Ash, Enze, Jiachen, Brae
===========================

This lab simulates the behavior of 4 hosts communicating on a network, and coordinating the mutually-exclusive use
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

