// newlib stubs. printf() output goes to the UART log.
// Adapted from amir1387aht/phy6222_smartwatch (MIT); _write fixed to use
// dbg_printf, since the LOG_INFO it called does not exist in the SDK.
#include <sys/stat.h>
#include <errno.h>
#include "log.h"

extern int __heap_start__;
extern int __heap_end__;
static char *heap_end = (char *)&__heap_start__;

int _write(int file, char *ptr, int len) {
    (void)file;
    dbg_printf("%.*s", len, ptr);
    return len;
}

int _read(int file, char *ptr, int len) {
    (void)file; (void)ptr; (void)len;
    errno = ENOSYS;
    return -1;
}

int _sbrk(int incr) {
    char *prev = heap_end;
    if (heap_end + incr > (char *)&__heap_end__) {
        errno = ENOMEM;
        return -1;
    }
    heap_end += incr;
    return (int)prev;
}

int _close(int file) { (void)file; return -1; }
int _lseek(int file, int ptr, int dir) { (void)file; (void)ptr; (void)dir; return 0; }
int _fstat(int file, struct stat *st) { (void)file; st->st_mode = S_IFCHR; return 0; }
int _isatty(int file) { (void)file; return 1; }
int _kill(int pid, int sig) { (void)pid; (void)sig; errno = EINVAL; return -1; }
int _getpid(void) { return 1; }
void _exit(int status) { (void)status; while (1); }
