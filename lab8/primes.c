#include "kernel/types.h"
#include "user/user.h"

void sieve(int fd) __attribute__((noreturn));

// One stage of the chain: the first number read from fd is a prime p.
// The stage prints it and passes on only the numbers not divisible by p.
void
sieve(int fd)
{
  int p, n;

  if(read(fd, &p, sizeof(p)) != sizeof(p)){   // no numbers left: end of the chain
    close(fd);
    exit(0);
  }
  printf("prime %d\n", p);

  int next[2];
  pipe(next);
  if(fork() == 0){                      // the next stage
    close(next[1]);
    close(fd);
    sieve(next[0]);
  }
  close(next[0]);                       // this stage only writes to the next one
  while(read(fd, &n, sizeof(n)) == sizeof(n)){
    if(n % p != 0)
      write(next[1], &n, sizeof(n));
  }
  close(fd);
  close(next[1]);                       // the next stage sees the end of its input
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[])
{
  int p[2];

  pipe(p);
  if(fork() == 0){                      // the first stage
    close(p[1]);
    sieve(p[0]);
  }
  close(p[0]);
  for(int i = 2; i <= 35; i++)
    write(p[1], &i, sizeof(i));
  close(p[1]);
  wait(0);
  exit(0);
}
