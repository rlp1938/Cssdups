
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
#include <libgen.h>
#include <ctype.h>

#include "fileops.h"
#include "firstrun.h"


static void failure(const char *emsg);
static void change_excludes(char *fn, char *excl, char *cmnt, int fatal);
static int absent_excludes(char *buf, char *glbegin, char *glend,
					char *lobegin, char *loend);
static fdata findhtmlcss(char *begin, char *end);

char *nocomment="No user comment entered";
char *localexcl = "~/.config/cssdups/csdexcl";
char *globalexcl = "/usr/local/share/cssdups/csdexcl";

static char *helpmsg =
"NAME\n\tcssdups - a program to report on duplicated "
"style names in a CSS or HTML\n\tfile, and optionally list empty lines"
" by line number."
"\nSYNOPSIS\n\tcssdups [options] cssfile\n"
"\nDESCRIPTION\n\tThe program examines the user named CSS or HTML"
"file and reports\n\tduplicated style names. Some such styles may "
"be replicated "
"harmlessly eg\n\t\'@font-family\' but other styles may harm the way "
"the html using the\n\tCSS renders in the browser, eg div.big is one that"
" may  be  problematic\n\tif duplicated. It is possible to block reporting"
" of harmless\n\tduplications. This may be done globally in\n\t"
"/usr/local/share/cssdups/csdexcl or per user in\n\t"
"~/.config/cssdups/csdexcl. The first file is created when the program"
"\n\tis installed and the latter only when the -x option is used.\n\t"
"See OPTIONS below.\n"
"\nOPTIONS\n\t-h\tprints helpfile.\n"
"\n\t-x\ttext 'comment text'\n"
"\tText is the string to ignore and comment text is a suitable comment"
" to\n\tinsert into the exclusions file following the text to ignore.  It"
" must\n\tbe present but may be empty ie ''. If empty a comment will be"
" generated\n\t´No user comment entered´. The file altered is\n\t"
"~/.config/cssdups/csdexcl. It is created on first use of this option"
"\n\tand appended subsequently.\n"
"\tFor this and the following option no CSS file need be provided,"
" nor will\n\tit be processed if present.\n"
"\n\t-g\ttext 'comment text'\n"
"\tProcessing this option is the same as for option -x except that the"
"\n\tfile affected is /usr/local/share/cssdups/csdexcl and that file is"
"\n\tinstalled at program installation time. You need root capability to"
"\n\tuse this option.\n"
"\n\t-e\tLists empty lines by line number.\n"
"NOTES\n\tBe certain that anything you add to either csdexcl file is"
" really\n\tharmless when duplicated in the CSS, because once inserted"
" that object\n\twill never again be reported as a duplicate. If a"
" mistaken entry must\n\tbe removed you can do it in any text editor."
" You will need to be root to\n\talter the global file,"
" /usr/local/share/cssdups/csdexcl.\n"
"\n\tNote  that for HTML files the duplicate search is confined to\n"
"\tthe style tags contained within the head tags. Individual styles\n"
"\tattached to html elements are not examined.\n"
;
void dohelp(int forced);

char ebuf[FILENAME_MAX];
char *memabslimit;	// set by readfile, it's 1 page more than the data


int main(int argc, char **argv)
{
	int opt;
	struct filedata *sfd;
	char *begin, *end, *cp, *dp, *glbegin, *glend, *lobegin, *loend,
			*cssfn;
	int c1, c2, showempty, dupscount;

	showempty = 0;
	while((opt = getopt(argc, argv, ":hx:g:e")) != -1) {
		char *fn, *excl, *cmnt;
		switch(opt){
		case 'h':
			dohelp(0);
		break;
		case 'e':
			showempty = 1;
		break;
		case 'x': // local config file
			fn = localexcl;
			excl = optarg;
			if ((argv[3]) && (strlen(argv[3]) != 0)){
				cmnt = strdup(argv[3]);
			} else {
				cmnt = nocomment;
			}
			change_excludes(fn, excl, cmnt, 0);
		break;
		case 'g': // global config file
			if (geteuid() != 0) {
				fputs("You must be root to use this option.\n", stderr);
				dohelp(1);
			}
			fn = globalexcl;
			excl = optarg;
			if ((argv[3]) && (strlen(argv[3]) != 0)){
				cmnt = strdup(argv[3]);
			} else {
				cmnt = nocomment;
			}
			change_excludes(fn, excl, cmnt, 1);
		break;
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
	cssfn = argv[optind];
	sfd = readfile(cssfn, 1);
	begin = sfd->from;
	end = sfd->to;
	free (sfd);
	sfd = readfile(globalexcl, 1);	// fatal if not existant
	glbegin = sfd->from;
	glend = sfd->to;
	free(sfd);
	sfd = readfile(localexcl, 0);	// not fatal if not existant
	if (sfd) {
		lobegin = sfd->from;
		loend = sfd->to;
		free(sfd);
	} else {
		lobegin = (char *)NULL;
		loend = (char *)NULL;
	}

	// turn the CSS mess into C strings
	cp = begin;
	while(cp < end) {
		cp = memchr(cp, '\n', end - cp);
		if(cp) *cp = '\0';
		cp++;
	}

	// see if I am looking at a html file
	cp = begin;
	cp = memmem(cp, 1024, "<!DOCTYPE html", strlen("<!DOCTYPE html"));
	if(cp) {
		sfd = findhtmlcss(begin, end);
		if (!(sfd)) {
			fprintf(stderr, "No internal css found in %s\n", cssfn);
			exit(EXIT_FAILURE);
		}
		begin = sfd->from;
		end = sfd->to;
		free(sfd);
	}
	// go looking for class names
	dupscount = 0;
	c1 = 0;
	cp = begin;
	while(cp < end) {
		char *curly;
		int result;
		curly = strchr(cp, '{');
		c1++;
		if(curly) {
			char buf[128];
			strcpy(buf, cp);
			curly = strchr(buf, '{');
			*curly = '\0';
			result = absent_excludes(buf, glbegin, glend, lobegin,
										loend);
			if (result) {
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
				"Duplicated style names: \'%s\' at lines %d and %d.\n",
							buf, c1, c2);
							dupscount++;
						}
					}
					dp += strlen(dp) + 1;
				}
			}
		}
		cp += strlen(cp) + 1;
	}
	if (dupscount == 0) {
		fprintf(stdout, "%s has no duplicate syle names.\n", cssfn);
	}
	if (showempty) {
		c1 = 0;
		cp = begin;
		while(cp < end) {
			c1++;
			if (strlen(cp) == 0) {
				fprintf(stdout, "Empty line at: %d\n", c1);
			}
			cp += strlen(cp) + 1;
		}
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

void change_excludes(char *fn, char *excl, char *cmnt, int fatal)
{
	/* appends fn with the excl and comment text on one line
	 * If the file fn does not exist it creates it unless fatal
	 * is non-zero. In the latter case we abort with error message.*/
	 FILE *fp;
	 struct stat sb;
	 char buf[255];
	 if (stat(fn, &sb) < 0) {
		 if(fatal) {
			 perror(fn);
			 exit(EXIT_FAILURE);
		 } else { // must create this file
			initfile(fn);
		 }
	 }
	 // now add the text
	 fp = fopen(fn, "a");
	 if(!(fp)) {
		 perror(fn);
		 exit(EXIT_FAILURE);
	 }
	 // add our text
	 strcpy(buf, excl);
	 strcat(buf, " #");
	 strcat(buf, cmnt);
	 strcat(buf, "\n");
	 fputs(buf, fp);
	 if (fclose(fp) < 0) {
		 perror(fn);
		 exit(EXIT_FAILURE);
	 }
	 // I don't want to return and have the user required to provide
	 // a CSS file for processing.
	 exit(EXIT_SUCCESS);

} // change_excludes()

int absent_excludes(char *buf, char *glbegin, char *glend,
					char *lobegin, char *loend)
{
	/* searches for buf firstly thru glbegin to glend
	 * if found returns 0. If not it searches thru lobegin to loend,
	 * provided that lobegin is not NULL.
	 * The default return is 1,  == not in excludes.
	 * If found in either list, returns 0, not absent from excludes/
	*/
	char *cp;
	char buf1[255];
	char buf2[255];

	strcpy(buf2, buf);	// trim trailing white space
	cp = buf2 + strlen(buf2) - 1;
	while(isspace(*cp)) {
		*cp = '\0';
		cp--;
	}
	strcpy(buf1, "\n");	// search target MUST be at the beginning of the
						// line.
	strcat(buf1, buf2);
	cp = memmem(glbegin, glend-glbegin, buf1, strlen(buf1));
	if (cp) return 0;
	if (lobegin) {
		cp = memmem(lobegin, loend-lobegin, buf1, strlen(buf1));
		if (cp) return 0;
	}
	return 1;	// it's absent from both lists
} // absent_excludes()

fdata findhtmlcss(char *begin, char *end)
{
	/* searches a html file for the area beginning with '<style and
	 * ending with </style>. Returns NULL if not found.
	 * */

	char *from, *to;
	fdata retdat = { 0 };	// init NULL

	// limit my search to what is between <head> ... </head>
	from = memmem(begin, end - begin, "<head", strlen("<head"));
	if (!(from)) return retdat;
	// might not have <head> ... </head>
	to = memmem(from, end - from, "</head>", strlen("</head>"));
	if (!(to)) {
		fputs("Opening '<head>' tag found without closing '</head>'.\n"
				, stderr);
		exit(EXIT_FAILURE);
	}
	// now find the styles if any
	from = memmem(from, to - from, "<style", strlen("<style"));
	if (!(from)) return (struct filedata *)from;
	// might not have <style> ... </style>
	to = memmem(from, to - from, "</style>", strlen("</style>"));
	if (!(to)) {
		fputs("Opening '<style>' tag found without closing"
				" '</style>'.\n", stderr);
		exit(EXIT_FAILURE);
	}
	retdat.from = from;
	retdat.to = to;
	return retdat;
} // findhtmlcss()
