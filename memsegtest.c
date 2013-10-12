/*
Copyright (c) 2003 - 2012 Janne Blomqvist

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

/* Test how different ways of allocating memory affect VM and data
 * segment size. Use ulimit before invoking this test program to check
 * how limits on different memory resources are enforced.  */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int print_memstat()
{
    char line[80];
    FILE *f = fopen("/proc/self/status", "r");
    if (!f)
	return -1;
    while (1) {
	if (!fgets(line, 80, f))
	    break;
	if (strncmp(line, "VmSize", 6) == 0
	    || strncmp(line, "VmRSS", 5) == 0
	    || strncmp(line, "VmData", 6) == 0)
	    printf("%s", line);
    }
    fclose(f);
    return 0;
}
	
    

int main(int argc, char* argv[])
{
    int sleeptot = 0;

    if (argc < 3) {
	printf("Usage: %s MODE SIZE_IN_MiB\nwhere\n\
    MODE=0: Anonymous mmap\n\
    MODE=1: mmap of a temporary file\n\
    MODE=2: malloc\n\
    MODE=3: brk\n", argv[0]);
	return 0;
    }
    int mode = atoi(argv[1]);
    long n = atol(argv[2]); // Mem in MiB

    n *= 1024*1024;

    void *p;
    int fd;
    char template[] = "/tmp/mmaptestXXXXXX";
    
    switch (mode) {
    case 0:
	p = mmap(NULL, n, PROT_READ|PROT_WRITE, 
		       MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	if (p == MAP_FAILED) {
	    perror("anonymous mmap failed");
	    return 1;

	}
	break;
    case 1:
	fd = mkstemp(template);
	if (fd == -1) {
	    perror("mkstemp failed");
	    return 1;
	}
	unlink(template);
	if (ftruncate(fd, n) == -1) {
	    perror("ftruncate failed");
	    return 1;
	}
	p = mmap(NULL, n, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED) {
	    perror("mmap failed");
	    return 1;
	}
	break;
    case 2:
	p = malloc(n);
	if (!p) {
	    perror("malloc failed");
	    return 1;
	}
	break;
    case 3:
	p = sbrk(n);
	if (p == (void*) -1) {
	    perror("sbrk failed");
	    return 1;
	}
	break;
    default:
	printf("MODE must be 0, 1, 2 or 3\n");
	return 0;
    }
    sleep(2);
    memset(p, 42, n); // Bump up RSS to approx VSS
    print_memstat();
    while (1) {
	sleep(5);
	sleeptot += 5;
	printf("Slept for a total of %d s.\n", sleeptot);
	fflush(stdout);
    }
    return 0;
}
