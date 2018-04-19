#include "diningPhils.h"

const char* names[16] = {"Hannah Arendt ", "Mary Astell ", "Laura Bassi ", "Helena Blavatsky ", "Antoinette Brown Blackwell ", "Mary Whiton Calkins ", "Margaret Cavendish ", "Émilie du Châtelet ", "Catharine Trotter Cockburn ", "Anne Conway ", "Helene von Druskowitz ", "Mary Ann Evans ", "Elisabeth of Bohemia ", "Sor Juana ", "Edith Stein ", "Felicia Nimue Ackerman "};

void sleeps() {
  yield();
}

void think(int x) {
  writes("Thinking\n");
  sleeps();
}

void eat(int x, int n) {
  void* left = shmget(x);
  void* right = shmget((x+1)%16);

  char str[12];
  itoa(str, n);

  writes((char*) names[x]);
  writes("is EATING for the ");
  writes(str);
  writes(" time\n");

  sleeps();

  shmdt(x);
  shmdt((x+1)%16);

}

void philosopher(int x) {
  int n = 0;
  while(1) {
    think(x);
    eat(x, ++n);
  }
}

void main_phil() {
  writes( "phil");

  for (int i = 0; i < 16; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      philosopher(i);
    }
  }

  exit( EXIT_SUCCESS );
}
