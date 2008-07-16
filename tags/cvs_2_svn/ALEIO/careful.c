/* careful.c --- I/O functions that do error checking.
   Jim Blandy <jimb@cyclic.com> --- February 1995  */

#include <string.h>
#include <errno.h>
#include <stdio.h>

static const char *prog_name;

/* Use NAME as the name of the program in future error reports.
   If NAME is a filename containing slashes, only the last component
   of the path is used; this means that you can pass in argv[0], and
   still get clean error messages.
   Return the portion of NAME that will be used in error messages.  */
const char * careful_prog_name (const char *name)
{
    prog_name = strrchr (name, '/');
    if (prog_name) prog_name++;
    else prog_name = name;

    return prog_name;
}


void check_syscall (int call, const char *filename, const char *op)
{
    if (call != -1)
        return;

    fprintf (stderr, "%s: %s%s",
             prog_name,
             filename ? filename : "",
             filename ? ": " : "");
    perror (op);

    exit (2);
}


/* If an error has occured on F, print an error message mentioning
   FILENAME and OP, as with check_syscall, and exit.  If no error has
   occurred on F, simply return.  */
void check_file (FILE *f, const char *filename, const char *op)
{
    if (ferror (f))
        check_syscall (-1, filename, op);
}


/* If NAME is null, return DEFALT.
   If NAME is non-null, try to open it with MODE; print an error message
   and exit if this doesn't succeed.  */
FILE * careful_open (const char *name, const char *mode, FILE *defalt)
{
    if (name)
    {
        FILE *result = fopen (name, mode);

        if (! result)
        {
            fprintf (stderr, "%s: ", prog_name);
            perror (name);
            exit (2);
        }

        return result;
    }
    else
        return defalt;
}


/* If FILE is neither stdin nor stdout, close it.  */
void
careful_close (FILE *f, const char *filename)
{
    check_file (f, filename, "closing file");

    if (f != stdin && f != stdout)
        if (fclose (f) == EOF)
            check_syscall (-1, filename, "closing file");
}
