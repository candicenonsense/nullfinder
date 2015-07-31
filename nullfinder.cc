
/** File: nullfinder.cc
    Author: Candice Quates 
    Program: gpunullfinder

    Takes argument of a file of dumped gpu memory, attempts to divine structure 
    by searching for blocks of nulls which have been added for padding. 
    Default block of nulls to search for is 8.  extracting is only working 
    for 8-null blocks.  Not reporting really really short (1-2 byte blocks)

    File parsing code lifted from sdhash, apache license (Roussev/Quates 2013)
    General structure and command line arguments lifted from zsniff. (apache license)
*/

/* compile with:  
g++ -std=c++0x -o nullf nullfinder.cc map_file.cc error.cc
*/


#include <vector>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>



#define VERSION "nullfind 0.2 by Candice Quates July 2015"


#include "util.h"


int
writeExtracted(std::vector<unsigned char> &image, std::string filename)  {
    std::string outfilename = filename+".extract";
    std::filebuf fb;
    fb.open (outfilename.c_str(),std::ios::out|std::ios::binary);
    if (fb.is_open()) {
        std::ostream os(&fb);
        os.write((const char *)image.data(), image.size());
        fb.close();
        return 0;
    } else {
        std::cerr << "nullfind: cannot write to " << outfilename << std::endl;
        return 1;
    }
}

// using null size default to 8 nulls 
// option for extract/noextract
// null size changing does not work yet
// other desired feature: use stringbuilder to make pretty hex address filenames
int 
main(int argc, char *argv[])
{
    bool extract = false;
    if (argc < 2 ) {
        std::cout << VERSION << std::endl << std::endl;
        std::cout << "Usage: "<< argv[0] << " [--extract] [--nulls 8] filename(s) " <<  std::endl;
        return 1;
    }   
    bool nullsm=false;
    uint32_t nullssize=8;
    // look for extract and nulls size
    for (int i=0; i<argc; i++) {
        if (std::string("--extract").compare(std::string(argv[i]))==0) {
            extract=true;
        }
        if (nullsm==true) {
            if (isdigit(argv[i][0])) {
                nullssize=atoi(argv[i]); 
            } else {
                std::cerr << "usage: "<< argv[0] << " [--extract] [--nulls 8] filename(s) " <<  std::endl;
                return 1;
            }
            nullsm=false;
        }
        if (std::string("--nulls").compare(std::string(argv[i]))==0) {
            nullsm=true;
        }
    }
    nullsm=false;
    //loop for multiple files here -- from prior code but does work.
    for (int i=1; i<argc; i++) {
        if (std::string("--extract").compare(std::string(argv[i]))==0) {
            continue;
        } 
        // nulls marker to ignore argument after --block
        if (nullsm==true) {
            nullsm=false;
            continue;
        }
        if (std::string("--nulls").compare(std::string(argv[i]))==0) {
            nullsm=true;
            continue;
        }
        const char* filename = argv[i] ;

        //load files -- the sdhash methods reliably parse files with nulls 
	processed_file_t *mfile=process_file(filename);
        if (mfile->size == 0)  {
            std::cerr << "nullf: file " << filename << " empty or not found" << std::endl;
            return 1;
        }
	std::cout << "nullf processing: "<< filename << std::endl;
        uint8_t current=0;
	uint64_t range=0;
	uint64_t nullcount=0;
	uint64_t startaddr=0;
        // I am well aware that there are prettier and more
        // elegant ways to do this, but this is much faster 
        // than I expected it to be (ie, a few minutes on a VM for 2gb file)
        for (uint64_t n=0; n < mfile->size ; n++) {
	    current=(uint8_t)*(mfile->buffer+n);
	    if (n==0 && current != 0) 
               std::cout<< std::hex <<n <<" data begins ";
	    if (current == 0) {
		nullcount++;
		if (range != 1 && nullcount == nullssize) {
                    std::cout<< std::hex <<n <<" ends, size ";
		    std::cout << std::dec << n-startaddr << std::endl;
                    std::cout<< std::hex <<n <<" nulls begin " ;
		    // extract data 
		    if (extract) {
			std::vector<unsigned char> image;
			image.resize(n-(nullssize-1)-startaddr);
			for (uint64_t x=0,y=startaddr; y<n-(nullssize-1); x++,y++) {
			    image[x]=(unsigned char)*(mfile->buffer+y);
			}
			std::stringstream builder;
			builder << filename<< "-"<< std::hex << startaddr;
			writeExtracted(image,builder.str());
		    }
		    range=1;
		    startaddr=0;
		}
            } else {
		// if we've changed state, note that.
		if (range == 1) {
                    std::cout<< std::hex <<n <<" end "<< std::endl;
                    std::cout<< std::hex <<n <<" data begins ";
		    startaddr=n;
		}
		range=0;
		nullcount = 0;
	    }
	} 
        // If we've reached EOF and are in a block of data, 
	// extract if asked.	
	if (nullcount==0) {
            std::cout<< std::hex <<mfile->size<<" ends, " ;
	    std::cout << std::dec << (mfile->size)-startaddr << std::endl;
	    if (extract) {
		std::vector<unsigned char> image;
		image.resize((mfile->size)-(nullssize-1)-startaddr);
		for (uint64_t x=0,y=startaddr; y<(mfile->size)-(nullssize-1); x++,y++) {
		    image[x]=(unsigned char)*(mfile->buffer+y);
		}
	        std::stringstream builder;
	        builder << filename<< "-"<< std::hex << startaddr;
		writeExtracted(image,builder.str());
	    }
	}
    } // loop for parsing multiple files
    return 0;
}



