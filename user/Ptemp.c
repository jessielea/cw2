/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of
 * which can be found via http://creativecommons.org (and should be included as
 * LICENSE.txt within the associated archive or repository).
 */

#include "Ptemp.h"

void main_Ptemp() {
  int x = 0;
  while( 1 ) {
    if(x%6000 == 0) {  //used to be %20
      write( STDOUT_FILENO, "Ptemp", 5 );
    }
  x++;
  }

  exit( EXIT_SUCCESS );
}
