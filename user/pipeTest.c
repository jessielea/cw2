#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc.h"

int main_pipe() {

  int pipeID = pipe();
  //
  // open(pipeID);
  //
  // write(pipeID, 0, 6);

  pid_t pid = fork();

  if(pid != 0) { //CHILD
    open(pipeID);

    int n = read(pipeID, 0, 0); //write this out
    char *y = "5";
    itoa(y, n);
    write( STDOUT_FILENO, y, 1 );

  }
  else { //PARENT
    write( STDOUT_FILENO, "HI", 2);
    open(pipeID);
    write(pipeID, 0, 7);

}


  exit( EXIT_SUCCESS );

}
