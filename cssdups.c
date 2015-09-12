
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
static int absent_excludes(char *buf, char *lobegin, char *loend);
static fdata findhtmlcss(char *begin, char *end);

static char *helpmsg =
"NAME\n\tcssdups - a program to report on duplicated "
"style names in a CSS or HTML\n\tfile, and optionally list empty lines"
" by line number."
"\nSYNOPSIS\n\tcssdups [options] cssfile\n"
"\nDESCRIPTION\n\tThe program examines the user named CSS or HTML"
"file and reports\n\tduplicated style names. Some such styles may "
"be replicated "
"harmlessly eg\n\t\'@font-family\' but other styles may harm the way "
"the html using the\n\tCSS renders in the browser, eg div.big is one "
"that may  be  problematic\n\tif duplicated. It is possible to block"
" reporting of harmless\n\tduplications by editing "
"$HOME/.config/cssdups/csdexcl.\n"

"\nOPTIONS\n\t-h\tprints helpfile.\n"
"\n\t-e\tLists empty lines by line number.\n"
"NOTES\n\tBe certain that anything you add to the csdexcl file is"
" really\n\tharmless when duplicated in the CSS, because once inserted"
" that object\n\twill never again be reported as a duplicate. If a"
" mistaken entry must\n\tbe removed you can do it in any text editor."
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
	char *begin, *end, *cp, *dp, *lobegin, *loend,
			*cssfn;
	int c1, c2, showempty, dupscount;

	showempty = 0;
	while((opt = getopt(argc, argv, ":hx:g:e")) != -1) {
		switch(opt){
		case 'h':
			dohelp(0);
		break;
		case 'e':
			showempty = 1;
		break;
		case ':':
			fprintf(stderr, "Option %c requires an argument\n",optopt);
			dohelp(1);
		break;
		case '?':
			fprintf(stderr, "Unknown option: %c\n",optopt);
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
	fdata sfd = readfile(cssfn, 1, 1);
	begin = sfd.from;
	end = sfd.to;
	// deal with shitty files that don't have a final '\n'.
	if (*(end-2) != '\n') *(end-1) = '\n';	// not part of original file

	char excludes[NAME_MAX];
	char *home = getenv("HOME");
	sprintf(excludes, "%s/.config/cssdups/csdexcl", home);

	sfd = readfile(excludes, 0, 0);	// not fatal if not existant
	if (sfd.from) {
		lobegin = sfd.from;
		loend = sfd.to;
	} else {
		firstrun("cssdups", "csdexcl");
		char *ehome = getenv("HOME");
		fprintf(stderr, "Wrote csdexcl to %s/.config/cssdups/\n"
		"Please edit this file to make it meet your needs.\n",
			ehome);
		exit (EXIT_SUCCESS);
	}

	// turn the CSS mess into C strings
	cp = begin;
	while(1) {
		cp = memchr(cp, '\n', end - cp);
		if (cp) {
			*cp = '\0';
		} else {
			break;
		}
		cp++;
	}

	// see if I am looking at a html file
	cp = begin;
	cp = memmem(cp, 1024, "<!DOCTYPE html", strlen("<!DOCTYPE html"));
	if(cp) {
		sfd = findhtmlcss(begin, end);
		if (!(sfd.from)) {
			char ebuf[NAME_MAX];
			sprintf(ebuf, "No internal css found in %s\n", cssfn);
			failure(ebuf);
		}
		begin = sfd.from;
		end = sfd.to;
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
			/* buf and buf2 are now malloced so as to deal with
			 * pathological css obfuscated by removing all
			 * \n. In such a case this program will be useless for
			 * finding duplicated style names. I must admit that I
			 * have no real desire to help the idiots who produce
			 * such css anyway.
			*/
			char *buf;
			buf = malloc(strlen(cp) + 1);
			if (!buf) {
				failure("Unable to malloc enough memory for buf\n");
			}

			strcpy(buf, cp);
			curly = strchr(buf, '{');
			*curly = '\0';
			result = absent_excludes(buf, lobegin, loend);
			if (result) {
				c2 = c1;
				dp = cp + strlen(cp) + 1;
				while (dp < end) {
					char *buf2;
					curly = strchr(dp, '{');
					c2++;
					if(curly) {
						buf2 = malloc(strlen(dp) + 1);
						strcpy(buf2, dp);
						curly = strchr(buf2, '{');
						*curly = '\0';
						if (strcmp(buf, buf2) == 0) {
							fprintf(stdout,
				"Duplicated style names: \'%s\' at lines %d and %d.\n",
							buf, c1, c2);
							dupscount++;
						}
						free(buf2);
					}
					dp += strlen(dp) + 1;
				}
			}
			free(buf);
		} // if(curly);
		cp += strlen(cp) + 1;
	}
	if (dupscount == 0) {
		fprintf(stdout, "%s has no duplicate style names.\n", cssfn);
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

void failure(const char *emsg) {
	/*
	 * The only exit point in the event of something invalid in command
	*/

	fprintf(stderr, "%s\n", emsg);

	exit(EXIT_FAILURE);
} // failure()

int absent_excludes(char *buf, char *lobegin, char *loend)
{
	/*
	 * searches for buf thru lobegin to loend,
	 * provided that lobegin is not NULL.
	 * The default return is 1, == not in excludes.
	 * If found in the list, returns 0, == is in excludes.
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
	if (lobegin) {
		cp = memmem(lobegin, loend-lobegin, buf1, strlen(buf1));
		if (cp) return 0;
	}
	return 1;	// it's not in the list.
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
	// NB I get compile time errors if I do 'retdat = { 0 };' ???
	retdat.from = NULL;
	retdat.to = NULL;
	from = memmem(from, to - from, "<style", strlen("<style"));
	if (!(from)) return retdat;
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
