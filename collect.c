/**********************************************************************
 * File Exchange collect client
 *
 * $Id: collect.c,v 1.1 1999-09-28 22:10:56 danw Exp $
 *
 * Copyright 1989, 1990 by the Massachusetts Institute of Technology.
 *
 * For copying and distribution information, please see the file
 * <mit-copyright.h>.
 **********************************************************************/

#include <mit-copyright.h>

#ifndef lint
static char rcsid_collect_c[] = "$Id: collect.c,v 1.1 1999-09-28 22:10:56 danw Exp $";
#endif /* lint */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fxmain.h"

/*** Global variables ***/
char *full_name();
long do_dump();
int kbytes = 0;			/* disk space used */
extern int verbose, errno;

/*
 * collect_arg checks to see if the current argument indicates
 * a type for which PRESERVE should be set.
 */

/*ARGSUSED*/
int
collect_arg(argc, argv, ip, p, flagp)
     int argc;
     char *argv[];
     int *ip;
     Paper *p;
     int *flagp;
{
  if (argv[*ip][0] == '-' && strchr("*tgPheAH", argv[*ip][1]))
    *flagp |= PRESERVE;
  return(0);
}

/*
 * adjust_criterion -- put filename in criterion
 */

void
adjust_criterion(p, s)
     Paper *p;
     char *s;
{
  p->author = s;
  return;
}

/*
 * compar -- compares two papers, used by qsort
 */

compar(p1, p2)
     Paper **p1, **p2;
{
  register int ret;

  ret = strcmp((*p1)->author, (*p2)->author);
  if (ret) return(ret);
  ret = strcmp((*p1)->filename, (*p2)->filename);
  if (ret) return(ret);
  ret = (int) (*p1)->modified.tv_sec - (*p2)->modified.tv_sec;
  if (ret) return(ret);
  return((int) ((*p1)->modified.tv_usec - (*p2)->modified.tv_usec));
}

/*
 * prep_paper -- gets filename, disk usage for paper
 */

long
prep_paper(p, f, flags)
     Paper *p;			/* paper to retrieve */
     char *f;			/* filename (modified) */
     int flags;
{
  static char *old_author = NULL;
  struct stat buf;

  (void) sprintf(f, "%s/%s", p->author, p->filename);
  kbytes += ((p->size + 1023) >> 10);

  if (!old_author || strcmp(p->author, old_author)) {
    /******** deal with new student ********/
    if (verbose) {
      printf("%s:\n", full_name(p->author));
    }
    kbytes++;
    if (!(flags & LISTONLY))
      if (mkdir(p->author, 0777))
	if (errno != EEXIST) {
	  sprintf(fxmain_error_context, "(%s not collected)", f);
	  return ((long) errno);
	} else {
	  stat(p->author, &buf);
	  if (buf.st_mode & S_IFDIR) {
	    kbytes--;
	    if (verbose)
	      printf("Using existing directory \"%s\".\n",
		     p->author);
	  } else {
	    sprintf(fxmain_error_context, "(%s not collected)", f);
	    return((long) errno);
	  }
	}
    old_author = p->author;
  }

  return(0L);
}

void
empty_list(criterion)
     Paper *criterion;
{
  if (criterion->author)
    printf("%s:\n", full_name(criterion->author));
  printf("No papers turned in\n");
}

void
mark_retrieved(fxp, p)
     FX *fxp;
     Paper *p;
{
  Paper taken;
  static int warned = 0;
  long code;

  if (!warned) {
    /******** mark file on server as TAKEN ********/
    paper_copy(p, &taken);
    taken.type = TAKEN;
    if (!warned && (code = fx_move(fxp, p, &taken))) {
      com_err("Warning", code, "-- files not marked TAKEN on server.", "");
      warned = 1;
    }
  }
  return;
}

/*
 * main collect procedure
 */

main(argc, argv)
  int argc;
  char *argv[];
{
  Paper p;

  paper_clear(&p);
  p.type = TURNEDIN;

  if (fxmain(argc, argv,
	     "Usage: %s [options] [assignment] [username ...]\n",
	     &p, collect_arg, do_dump)) exit(1);
  if (verbose && kbytes) printf("%d kbytes total\n", kbytes);
  exit(0);
}
