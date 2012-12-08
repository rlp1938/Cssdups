
/*      cssdups.c
 *
 *	Copyright 2011 Bob Parker <rlp1938@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *	MA 02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h> 
#include <string.h>

void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen);
struct filedata {
    char *from; // start of file content
    char *to;   // last byte of data + 1
};


struct filedata *readfile(char *path, int fatal);
void failure(const char *emsg);
struct filedata *mkstructdata(char *from, char *to);


char *helpmsg = "\n\tUsage: cssdups [option] cssfile\n"
  "\tProgram to find duplicated class names in a css file\n"
  "\n\tOptions:\n"
  "\t-h outputs this help message.\n"
  ;

void dohelp(int forced);

char ebuf[FILENAME_MAX];
char *memabslimit;	// set by readfile, it's 1 page more than the data
int replaying;

int main(int argc, char **argv)
{
	int opt;
	struct filedata *sfd;
	char *begin, *end, *cp, *dp;
	int c1, c2;

	while((opt = getopt(argc, argv, ":h")) != -1) {
		switch(opt){
		case 'h':
			dohelp(0);
		break;
		/*
		case 'x': // fill in actual options
		break;
		* */
		case ':':
			fprintf(stderr, "Option %c requires an argument\n",optopt);
			dohelp(1);
		break;
		case '?':
			fprintf(stderr, "Illegal option: %c\n",optopt);
			dohelp(1);
		break;
		} //switch()
	}//while()
	// now process the non-option arguments

	// 1.Check that argv[???] exists.
	if (!(argv[optind])) {
		fprintf(stderr, "No css file provided\n");
		dohelp(1);
	}

	sfd = readfile(argv[optind], 1);
	begin = sfd->from;
	end = sfd->to; 
	free (sfd);
	
	// turn the mess into C strings
	cp = begin;
	while(cp < end) {
		cp = memchr(cp, '\n', end - cp);
		if(cp) *cp = '\0';
		cp++;
	}
	// go looking for class names
	c1 = 0;
	cp = begin;
	while(cp < end) {
		char *curly;
		curly = strchr(cp, '{');
		c1++;
		if(curly) {
			char buf[128];
			strcpy(buf, cp);
			curly = strchr(buf, '{');
			*curly = '\0';
			//fprintf(stdout, "Class \'%s\' at line %d\n", buf, c1);
			c2 = c1;
			dp = cp + strlen(cp) + 1;
			while (dp < end) {
				char buf2[128];
				curly = strchr(dp, '{');
				c2++;
				if(curly) {
					strcpy(buf2, dp);
					curly = strchr(buf2, '{');
					*curly = '\0';
					if (strcmp(buf, buf2) == 0) {
						fprintf(stdout, 
			"There may be duplicates of \'%s\' at lines %d and %d.\n",
						buf, c1, c2);
					}
				}
				dp += strlen(dp) + 1;
			}
		}
		cp += strlen(cp) + 1;
	}
	return 0;
}//main()

void dohelp(int forced)
{
  fputs(helpmsg, stderr);
  exit(forced);
}

struct filedata *readfile(char *path, int fatal)
{
    /*
     * open a file and read contents into memory.
     * if fatal is 0 and the file does not exist return NULL
     * but if it's non-zero abort and do that for all other
     * errors.
    */
    struct stat sb;
    FILE *fpi;
    off_t bytes;
    char *from, *to;

    if ((stat(path, &sb)== -1)){
        if (!(fatal)) return NULL;
			failure(strerror(errno));
    }
    if (!(S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode))) {
        sprintf(ebuf, "%s: is not a file or symlink. Aborting!\n",
                    path);
        failure(ebuf);
    }
    fpi = fopen(path, "r");
    if (!(fpi)) {
		failure(strerror(errno));
    }
    from = malloc(sb.st_size + FILENAME_MAX);
    if(!(from)) {
        perror("No memory to read file");
        exit(EXIT_FAILURE);
    }
    bytes = fread(from, 1, sb.st_size, fpi);
    if (bytes != sb.st_size){
        char *fmt =
            "Tried to read %ld bytes, actually read %ld bytes.\n";
        sprintf(ebuf, fmt, (long)sb.st_size, (long)bytes);
        failure(ebuf);
    }
    to = from + bytes;
    memabslimit = to + FILENAME_MAX;
    return mkstructdata(from, to);
} // readfile()

void failure(const char *emsg) {
	/*
	 * The only exit point in the event of something invalid in command
	*/

	fprintf(stderr, "%s\n", emsg);

	exit(EXIT_FAILURE);
} // failure()

struct filedata *mkstructdata(char *from, char *to)
{
	// create and initialise this struct in 1 place only
	struct filedata *dp = malloc(sizeof(struct filedata));
	if (!(dp)){
		perror("malloc failure making 'struct filedata'");
		exit(EXIT_FAILURE);
	}
	dp->from = from;
	dp->to = to;
	return dp;
} // mkstructdata()

