#!/bin/bash
# nooptim.sh - build target with no compiler optimisations.

gcc -Wall -Wextra -g -O0 -D_GNU_SOURCE=1 -c cssdups.c
gcc -Wall -Wextra -g -O0 -D_GNU_SOURCE=1 -c firstrun.c
gcc -Wall -Wextra -g -O0 -D_GNU_SOURCE=1 -c fileops.c
gcc -g cssdups.o firstrun.o fileops.o -o cssdups
