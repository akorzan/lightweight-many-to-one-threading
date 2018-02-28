For prospective employers.
# Not actually a threading library, this was an earlier implementation of trampolines in assembly.
I believe the other threading library on my site is more fully implemented.

## The start to Many-to-One LightWeight Round Robin Threading
Designed for class.  If I am not mistaken, it was fully implemented as long as the included main function is used.

## The following is the README for the old assignment submission.

I have my own main function.  Please see that it works with that.

Currently what is broken are the states.  They are not being set correctly and this leads to a loop with a dead thread, running thread, and two joining threads... and according to my scheduler, there is no blocked thread to switch so on your test file it will spin.

The trashing of the registers was not the problem, but there's a problem with the scheduler.  I guarentee that the assembly code and everything is fine.  I read over and fine combed it.  The segfault you saw earlier was from the assert function.

For anthony's test function
make
./main

For gabe's test function
make test
./test
