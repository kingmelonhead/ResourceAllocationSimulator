John Hackstadt

Assignment 5

start date 4/14/21

github: https://github.com/kingmelonhead/hackstad.5

university: UMSL

class: operating systems (cs 4760)


---------------- Basic description -------------------

In this project we are making a different version of the OS simulator that we made in our previous project.
Here instead of simulating round robin scheduling, we will be simulating resource allocation between multiple
processes who are requesting the various resources

the user proccesses will now all do their own work concurrently as opposed to being scheduled and only working during a specific time.

---------------- How program works -------------------------

gets shared memory

creates semaphores for resource and clock access

forks children after a random amount of time

main body of the oss follows this general outline:
1. check for early terminations
2. take care of release requests
3. using the updated resources, go through the resource allocation requests
4. check if the system is safe, if not then deal with the deadlock by killing processes

the forked processes can either request resources, release resources, or terminate early
what these processes do is based on random number generation 

the program has an alarm set to end the OSS after 5 real clock seconds

the program also keeps track of some stats and logs them at the end of run time

-------------- how to invoke ----------------------------------

make
./oss

--------------- Changes ------------------------------------

"Execute oss with no parameters.You may add some parameters to modify simulation, such as number and instances of resources; if you do so, 
please document it in your README."

this said arguments were optional 

but then 

"When verbose is off, it should only indicate what resources are requested and granted, and available resources. When verbose
is off, it should only log when a deadlock is detected, and how it was resolved."

said to do something would need arguments and on top of that, the wording doesnt make sense. it says 
"when verbose is off" both times. Confusing. Based on this instruction I cant determine what the difference between 
verbose on and off would be so I just left the program to log everything I felt might be important. Anything unwanted
can just be ignored I suppsoe. I tried to keep the log as clean and easy on the eyes as possible

I also wasnt 100% sore where all you wanted the resource allocation chart to be printed. It made the most sense to place it at the
end to function as a final resource allocation snapshot. 

however, printing the resource allocation graph is simple, it can be wedged anywhere in the code with the print_allocation() function

-------------- Issues -------------------------------------

I didnt see any issues on my final build and I tested it many times