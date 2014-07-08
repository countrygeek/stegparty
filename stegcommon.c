/*
  stegcommon.c - common code for building StegParty parsers
  Copyright (C) 1999  Steven E. Hugg (hugg@pobox.com)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define STEG_VERSION "0.2"

char verbose = 0;
char decode = 0;
char earlyout = 1;

char eof = 0;
char done = 0;
FILE *textout = stdout;
FILE *cipherin = NULL;

int exit_status = 0;
int num_subs = 0;
int chars_lexed = 0;
int bytes_read = 0;
int pos_stopped = 0;

typedef unsigned int cipblock_t;

cipblock_t cipher, curfactor, ciplimit;

extern FILE *yyin;
extern void yylex();

#define STAT_BAD_ARGS	1
#define STAT_MORE_DATA	2
#define STAT_BAD_MATCH  4

#define IFDEBUG(level) if (verbose >= (level))

/*
static void computelimit()
{
	ciplimit = 1;
	while (ciplimit*factor > ciplimit)
		ciplimit *= factor;
	ciplimit--;
	IFDEBUG(2)
		fprintf(stderr, "Cipher limit = %x\n", ciplimit);
}
*/

static void readchunk()
{
	cipher = 0;
	ciplimit = 0;
	if (!eof)
	{
		int ch,n = 0;
		while (n < sizeof(cipher) && ((ch = fgetc(cipherin)) != EOF))
		{
			cipher += ((cipblock_t)(ch & 0xff))<<(n*8);
			n++;
			bytes_read++;
		}
		ciplimit = (0x1<<(n*8))-1;
		if (ch == EOF)
			eof = 1;
		IFDEBUG(2)
			fprintf(stderr, "read %d bytes from new chunk = %x, limit = %x\n", 
				n, cipher, ciplimit);
	} else {
		IFDEBUG(2)
			fprintf(stderr, "hit eof, no more bytes read\n");
	}
	curfactor = 1;
}

static void writechunk()
{
	int n=0;
	curfactor--;
	while (curfactor)
	{
		int ch = (cipher>>(n*8)) & 0xff;
		fputc(ch, textout);
		n++;
		bytes_read++;
		curfactor >>= 8;
	}
	IFDEBUG(2)
		fprintf(stderr, "wrote %d bytes to chunk = %x\n", n, cipher);
	cipher = 0;
	curfactor = 1;
}

inline static void mulfactor(int mod)
{
	curfactor *= mod;
	IFDEBUG(3)
		fprintf(stderr, "curfactor = %d, bytes_read = %d\n", 
			curfactor, bytes_read);
}

inline static int getbits(int mod)
{
	int val;
	if (!curfactor)
		readchunk();
	val = cipher % mod;
	cipher /= mod;
	IFDEBUG(2)
		fprintf(stderr, "mod %d = %d, factor = %x\n", mod, val, curfactor);
	mulfactor(mod);
	return val;
}

inline static void putbits(int mod, int val)
{
	if (!curfactor)
		writechunk();
	cipher += (val*curfactor);
	IFDEBUG(2)
		fprintf(stderr, "mod %d = %d, factor = %x\n", mod, val, curfactor);
	mulfactor(mod);
}

int chooseTerm(char *text, int flags, int nargs, char *args, ...)
{
	va_list ap;
	chars_lexed += strlen(text) - (flags&1);
	num_subs++;
	
	if (done && !decode)
	{
		fprintf(textout, "%s", text);
		return 0;
	}
	
	va_start(ap, args);
	if (decode)
	{
		int val;
		int l1 = strlen(text) - (flags&1) - ((flags&2)>>1);
		char *arg = args;
		for (val=0; val<nargs; val++)
		{
			int c;
			if (strlen(arg) == l1)
			{
				c = strncmp(text + (flags&1), arg, l1);
				if (!c)
					break;
			}
			arg = va_arg(ap, char*);
		}
		if (val<nargs)
		{
			putbits(nargs, val);
		} else {
			fprintf(stderr, "***ERROR: '%s'(%d) did not match\n", text, flags);
			exit_status |= STAT_BAD_MATCH;
		}
	} else {
		int x = getbits(nargs);
		char *arg = args;
		while (x-- > 0)
			arg = va_arg(ap, char*);
		if (flags & 1)
			fputc(text[0], textout);
		fprintf(textout, "%s", arg);
		if (flags & 2)
			fputc(text[strlen(text)-1], textout);
	}
	va_end(ap);
	if (!decode && eof && !curfactor && !pos_stopped)
	{
		fprintf(stderr, "All data converted, position = %d\n", chars_lexed);
		pos_stopped = chars_lexed;
		done = earlyout;
	}
	return 0;
}

int noMatch(char *text)
{
	chars_lexed += strlen(text);
	if (!decode)
	{
		fprintf(textout, "%s", text);
	}
	return 0;
}

static void usage()
{
	fprintf(stderr,
	"\nStegParty " STEG_VERSION " - Usage:\n\n"
	"stegparty [-v[v]] -c secretfile -i carrierfile > codedfile\n"
	"stegparty -d [-v[v]] -i codedfile > secretfile\n"
	"\n");
}

int main(int argc, char **argv)
{
	int             i;
	extern int      optind;
	extern char     *optarg;

	while ((i = getopt(argc, argv, "Evdc:i:o:")) != EOF)
	{
		switch (i)
		{
			case 'v':
         	verbose++;
            break; 
			case 'd':
				decode = 1;
				break;
			case 'c':
				cipherin = fopen(optarg, "r");
				break;
			case 'i':
				yyin = fopen(optarg, "r");
				break;
			case 'o':
				textout = fopen(optarg, "w");
				break;
			case 'E':
				earlyout = 0;
				break;
			default:
				exit_status |= STAT_BAD_ARGS;
				break;
		}
	}
	
	if (exit_status)
	{
		usage();
		return exit_status;
	}
	
	argc -= optind;
	argv = &argv[optind];
	if (argc)
		cipherin = fopen(argv[0], "r");

	curfactor = decode;
	cipher = 0;
	yylex();
	if (decode)
		writechunk();
	if (!decode && !eof)
	{
		fprintf(stderr, "***ERROR: not all data converted\n");
		exit_status |= STAT_MORE_DATA;
	}
	fflush(stdout);
	IFDEBUG(1)
	{
		fprintf(stderr, "\n");
		if (exit_status)
			fprintf(stderr, "Done, exit status = %d\n", exit_status);
		if (chars_lexed)
		{
			fprintf(stderr, "%d substitutions made considered, %d bytes encoded", 
				num_subs, bytes_read);
			if (!decode)
				fprintf(stderr, ", efficiency = %3.2f%%",
				(100.0f*bytes_read)/chars_lexed);
			fprintf(stderr, "\n");
			if (pos_stopped)
				fprintf(stderr, "stopped at pos %d out of %d, %3.2f%% waste\n",
					pos_stopped, chars_lexed,
					100.0f-(100.0f*pos_stopped)/chars_lexed);
		}
	}
	return exit_status;
}
