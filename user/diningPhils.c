#include "diningPhils.h"

const char* names[16] = {"Hannah Arendt ", "Mary Astell ", "Laura Bassi ", "Helena Blavatsky ", "Antoinette Brown Blackwell ", "Mary Whiton Calkins ", "Margaret Cavendish ", "Émilie du Châtelet ", "Catharine Trotter Cockburn ", "Anne Conway ", "Helene von Druskowitz ", "Mary Ann Evans ", "Elisabeth of Bohemia ", "Sor Juana ", "Edith Stein ", "Felicia Nimue Ackerman "};

//sleep, think, eat, beaPhil
void sleeps() {
  yield();
}

void think() {
  writes("think\n");
  sleeps();
}

void eat(int x) {
  void* left = shmget(x);
  void* right = shmget((x+1)%16);

  writes((char*) names[x]); //print these names
  writes("is eating\n");

  sleeps();

  shmdt(x);
  shmdt((x+1)%16);

}

void philosopher(int x) {
  while(1) {
    think();
    eat(x);
  }
}

int main_phil() {
  writes( "phil");

  for (int i = 0; i < 16; i++) {
    int pid = fork();
    if (pid == 0) {
      writes("child");
      philosopher(i);
    }
  }

  exit( EXIT_SUCCESS );
}
