This program uses two child processes to perform the summation of 1...100.
The parent waits for the first child process to finish before creating the next, this is done using wait().
Interprocess communication is required to communicate between the processes, to accomplish this program uses pipes.

To compile the program:
    make

To clean the folder:
    make clean

To run the program:
    ./hw1