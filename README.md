# Operating Systems Laboratory Exercises

## Exercise 1: System Calls, Processes, and Interprocess Communication

### File Reading and Writing with System Calls
- Implement a C program to count occurrences of a character in a file and write the result to another file.
- Use system calls (`open`, `read`, `write`, `close`, `exit`) instead of standard C library functions.
- Extend the program to handle incorrect user input and provide usage instructions.

### Process Creation
- Extend the program to create a child process that:
  - Greets the world.
  - Prints its PID and its parent's PID.
  - Experiments with variable sharing between parent and child.
- Assign the character search task to the child process.
- Use `execv` to create a child process that executes the counting code.

### Interprocess Communication
- Extend the program to create **P** child processes that search for the character in parallel.
- The parent process collects and prints the total result.
- Handle `SIGINT` (Ctrl+C) to print the number of active processes instead of terminating.

### Parallel Character Counting Application
- Implement a modular application for parallel character counting.
- The application should allow:
  - Dynamic addition/removal of worker processes.
  - Displaying worker information.
  - Showing search progress (percentage completed and characters found so far).
- The application consists of three components:
  - **Front-end**: Handles user interaction.
  - **Dispatcher**: Distributes work to workers and collects results.
  - **Workers**: Perform the character search on assigned file segments.

---

## Exercise 2: Synchronization

### Synchronization in Existing Code
- Modify the provided `simplesync.c` program to synchronize two threads using:
  - POSIX **mutexes** (`simplesync-mutex`).
  - **GCC atomic operations** (`simplesync-atomic`).
- Measure and compare execution times of synchronized and unsynchronized versions.
- Investigate how atomic operations and mutexes are translated into assembly instructions.

### Parallel Mandelbrot Set Computation
- Modify the provided Mandelbrot set computation program to use **POSIX threads**.
- Implement two versions of synchronization:
  - **POSIX semaphores**.
  - **Condition variables**.
- Measure execution time and compare serial vs. parallel performance.
- Handle `Ctrl+C` to reset terminal color state.

---

## Exercise 3: Virtual Memory Mechanisms

### System Calls and Virtual Memory Management
- Experiment with system calls and virtual memory using the provided `mmap.c` skeleton.
- Tasks include:
  - Printing the virtual memory map of a process.
  - Allocating and mapping memory buffers using `mmap`.
  - Finding physical addresses of virtual memory mappings.
  - Sharing memory between parent and child processes using `fork`.
  - Restricting write access to shared memory and unmapping buffers.

### Parallel Mandelbrot Computation with Processes
- Modify the Mandelbrot set computation program to use **processes** instead of threads.
- Implement synchronization using:
  - **Semaphores** over shared memory.
  - A **shared buffer** without semaphores.
- Compare performance between thread-based and process-based implementations.
- Investigate how `mmap` can be used for interprocess memory sharing.

---

## Optional Extension of Exercise 1
- Modify the character counting program to use **shared memory** and **semaphores** for interprocess communication.
- Allow the program to print the current count of character occurrences when receiving `SIGINT` (Ctrl+C).

---

## How to Run the Programs
1. **Compile the programs** using `gcc -o program program.c -pthread` (where applicable).
2. **Run the programs** using `./program` with appropriate arguments.
3. For programs requiring multiple processes, ensure to handle process cleanup.
4. For Mandelbrot computations, compare execution times between different synchronization techniques.

---

## Dependencies
- GCC compiler
- POSIX threads (pthread)
- Semaphores and shared memory support

---

