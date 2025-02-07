```
 ____  _          _ _   ____
/ ___|| |__   ___| | | |___ \
\___ \| '_ \ / _ \ | |   __) |
 ___) | | | |  __/ | |  / __/
|____/|_| |_|\___|_|_| |_____|
```

Hi there welcome to my shell 2 description! This project almost broke me but thankfully it's over 
and I passed the tests! For your convenience, my dear grader, I have also included a description 
of my shell 1 here since it contains a lot of relevant descriptions: 

This is my Shell 1. It uses a while loop and reads in lines. It then parses using my modified lab2. 
Then it handles command execution. This function returns values based on if each command was 
executed. Everything returns 1 except for exit which returns 2 so exit can be handled properly. I 
return 1 to avoid using a continue keyword. I then fork and inside the child process I handle 
redirections which loops through the tokens and accordingly handles each redirection. We then excecv 
passing in the filepath and argv and wait at the end passing in NULL as the argument as suggested in 
the handout. 

But for shell 2 I built on this functionality. I added in handling for signals. It also now creates 
and maintains a job list to keep track of jobs being run. I also added functionality to the 
execute_builtins to handle fg and bg commands. I also implemented reaping to free any resources 
that persist on the system. I also added a lot of logic to handle setting terminal control and 
foreground vs background jobs. I made a few edits to parsing to handle running jobs in the 
background too. Please refer to function comments and in line comments too!

Compile using make clean all or make 33sh and make 33noprompt
