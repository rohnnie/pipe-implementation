# Introduction to Operating Systems - Pipe Flow

## Overview

We create a more user-friendly way to chain processes together, similar to how the Linux shell handles piping and redirection. Instead of using bash commands, we will develop a custom language and an interpreter that reads graph descriptions from files and executes them as data-flow graphs.

### Examples

Below are some examples to understand common input and output redirections in Unix:

```bash
# Example 1: Standard output to standard input
$ ls | wc
      25      37     418
```

```bash
# Example 2: Redirecting output to a file
$ ls > result.txt
$ ls -la result.txt
-rw-r--r--@ 1 user  staff  429 Sep 28 20:03 result.txt
```

```bash
# Example 3: Redirecting input from a file
$ wc < result.txt
      26      38     429
```
### More complex examples demonstrate the use of combining standard error with standard output:
```bash
# Example 4: Redirect standard error to standard output
$ mkdir a | wc
mkdir: a: File exists
       0       0       0
```
```bash
$ mkdir a |& wc
       1       4      22
```
```bash
# Example 5: Using file descriptor redirection
$ mkdir a 2>&1 | wc
       1       4      22
```

## Task Description

We will implement a program named flow that interprets and executes a description file containing commands and data-flow graphs. This program should support:

node: Runs a simple command passed to the command attribute.

pipe: Chains two nodes together using the from and to attributes.

concatenate: Runs a sequence of nodes and combines their outputs sequentially.

Examples in Flow Language

### Example 1: ls | wc

File: filecount.flow
```css
node=list_files
command=ls

node=word_count
command=wc

pipe=doit
from=list_files
to=word_count
```

Run Command:
```bash
./flow filecount.flow doit
```

### Example 2: Complex Data Flow

File: complicated.flow
```css
node=cat_foo
command=cat foo.txt

node=sed_o_u
command=sed 's/o/u/g'

pipe=foo_to_fuu
from=cat_foo
to=sed_o_u

concatenate=foo_then_fuu
parts=2
part_0=cat_foo
part_1=foo_to_fuu

node=word_count
command=wc

pipe=shenanigan
from=foo_then_fuu
to=word_count
```

Run Command:
```bash
./flow complicated.flow shenanigan
```

### Implementation Details

node: Defines a command to be executed.

pipe: Connects the output of one node to the input of another.

concatenate: Combines the output of multiple nodes into a sequence.

Extra Tasks:

1. stderr: Extracts the standard error output of a node, specified using the from attribute.
2. file: Handles input or output from a file, with the filename passed using the name attribute.
3. Advanced Redirection: Implementing more complex redirection scenarios, like using 2>&1.

#### Extra Credit Examples

Error Handling

Flow File: error_handling.flow
```css
node=mkdir_attempt
command=mkdir a

node=word_count
command=wc

stderr=stdout_to_stderr_for_mkdir
from=mkdir_attempt

pipe=catch_errors
from=stdout_to_stderr_for_mkdir
to=word_count
```
Explanation: mkdir_attempt tries to create a directory a. If it fails, the error message is redirected and passed to word_count.

### Setup and Compilation

#### Environment Setup
You can use Linux, WSL (Windows Subsystem for Linux), or Mac for this assignment. Make sure you have the necessary permissions and libraries installed.

#### Compilation
Compile your program using:
```bash
gcc -o flow flow.c
```

#### Running Your Program
```bash
./flow <description_file> <action>
```

1. <description_file>: The .flow file describing your data-flow graph.
2. <action>: The specific action or pipe to execute.

### Notes and Tips

Redirection: All redirections can be handled using pipes. For example:

ls > foo.txt can be achieved with ls | tee foo.txt.

wc < foo.txt can be achieved with cat foo.txt | wc.

Standard Error: Handling 2>&1 redirection is part of the extra credit.

Debugging: Use simple examples to test your program and gradually increase complexity.

For more details on bash redirection, refer to:

[GNU Bash Manual - Redirection](https://www.gnu.org/software/bash/manual/html_node/Redirections.html)

[Process Substitution](https://tldp.org/LDP/abs/html/process-sub.html)
