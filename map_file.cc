#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>

#include "util.h"

using namespace std;

/**
 * Open & read in a file (compile w/ -D_FILE_OFFSET_BITS=64)
 */
processed_file_t *process_file(const char *fname) {
    processed_file_t *mfile = (processed_file_t *) alloc_check( ALLOC_ZERO, sizeof( processed_file_t), "map_file", "mfile", ERROR_EXIT);
    struct stat file_stat;
    ifstream *is= new ifstream();
        
    if( stat( fname, &file_stat)) {
        fprintf( stderr, "Warning: Could not stat file '%s'. Skipping.\n", fname);
        return NULL;
    }
    is->open(fname,ios::binary);

    mfile->buffer = (uint8_t*)alloc_check(ALLOC_ZERO,sizeof(uint8_t)*file_stat.st_size, "read_file", "mfile", ERROR_EXIT);
    is->read((char*)mfile->buffer,file_stat.st_size);
    int res=is->gcount();
    if( res != file_stat.st_size) {
        fprintf( stderr, "read failed: %s.\n", strerror( errno));
        free( mfile);
        is->close();
        return NULL;
    }
    mfile->name = (char*)fname;
    mfile->size = file_stat.st_size;
    is->close();
    return mfile;
}

