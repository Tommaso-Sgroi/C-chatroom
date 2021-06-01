/* Program to demonstrate the use of the signal handler to send
 * and catch signals.
 *
 * Just compile and run, nothing to input.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/wait.h>


int x = 0;

main()
{
   int status;
   int pid;                // pid depending on parent or child
   int o;                  // dummy variable return from kill
   void handler(int);         // signal handler prototype
   void handler1(int);        // signal handler prototype
   struct sigaction mySigActions;

   /*
    * Get a child process
    */
   if((pid = fork()) < 0)
   {
      printf("%s\n","fork error");
      exit(1);
   }
   printf("parent/child process id = %d\n",pid);
   /*
    * The child executes the code inside the if
    */
   if (pid == 0)
   {
//      sigset(SIGUSR1, handler);  // set handler to use the function
//      signal(SIGUSR1, handler);  // sigset could also be used
                                   // works similar to signal
        signal(SIGUSR2, handler1); // set handler to use the function
  mySigActions.sa_handler = handler;
  sigemptyset(&mySigActions.sa_mask);
  mySigActions.sa_flags = 0;
  //if (sigaction(SIGUSR1, &sa, NULL) < 0) {
  if (sigaction(SIGUSR1, NULL, NULL) < 0) {
    printf("Sigaction failed for SIGHUP");
    exit(1);
  }

     printf("executing in the child process\n");
     printf("in the child process waiting for a signal\n");
      sleep(20);
      while (1){};// printf("waiting   "); sleep(0);}
   }



   /*
    * The parent executes the following.
    */
   else {
      printf("executing in the parent process\n");
      printf("pid = %d\n",pid);   // the child pid
      sleep(1);                   // wait to be sure the child has
                                  // a chance to execute
      o = kill(pid, SIGUSR1);     // send the signal to child
      printf("parent just sent sig1 o = %d\n",o);
      sleep(3);                   // wait to be sure the child has

      o = kill(pid, SIGUSR2);     // send the signal to child
      printf("parent just sent SIGUSR2 o = %d\n",o);
      sleep(1);

      o = kill(pid, SIGKILL);     // send the signal to child
      printf("SIGKILL o = %d\n",o);
      while(wait(&status) != pid) // wait for the child to finish
         printf("parent waiting\n ");
   }
}

void handler(int signo)
{
   printf("Signal received In signal handler signo = %d\n",signo);
   printf("In signal handler NONE \n\n\n");
}


void handler1(int signo)
{
   printf("Signal received In signal handler1 signo = %d\n",signo);
   printf("In signal handler ONE  \n\n\n");
}
