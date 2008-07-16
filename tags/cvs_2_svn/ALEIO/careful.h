/* careful.c --- I/O functions that do error checking.
   Jim Blandy <jimb@cyclic.com> --- February 1995  */

#ifndef CAREFUL_H
#define CAREFUL_H


/* Use NAME as the name of the program in future error reports.
   If NAME is a filename containing slashes, only the last component
   of the path is used; this means that you can pass in argv[0], and
   still get clean error messages.
   Return the portion of NAME that will be used in error messages.  */
extern char *careful_prog_name (char *name);


/* Check the value of a system call.
   If CALL is -1, print an error message like:
       PROGNAME: FILENAME: OP: ERRNO-MESSAGE
   and exit with a non-zero status.
   If FILENAME is zero, omit it and the ": " after it.  */
extern void check_syscall (int CALL, char *FILENAME, char *OP);


/* If an error has occured on F, print an error message mentioning
   FILENAME and OP, as with check_syscall, and exit.  If no error has
   occurred on F, simply return.  */
extern void check_file (FILE *f, char *filename, char *op);


/* If NAME is null, return DEFALT.
   If NAME is non-null, try to open it with MODE; print an error message
   and exit if this doesn't succeed.  */
extern FILE *careful_open (const char *name, const char *mode, FILE *defalt);


/* If F is neither stdin nor stdout, close it.
   If an error has occurred on it, print an error message including
   FILENAME (if it is non-null).  */
extern void careful_close (FILE *F, char *FILENAME);


#endif /* CAREFUL_H */
