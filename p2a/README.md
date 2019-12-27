The following is the description of the code and some mayjor implement techniques used in this project.

input: parse user input by token
exit: call exit(0) if input is the only command provided
history: use a linked list to record user inputs
cd: use chidir to go to the destinated directory
path: set path array as a global array of char arrays, update it every time user puts > path (...)
redirection: use pipe to redirect stdout to a file
pipe: use fork to create 2 child processes.
In the first child process, redirect stdout and stderr of exeve the arguments before '|' to pipe, 
In the second child process, redirect stdin to pipe as the input of the arguments after '|'

