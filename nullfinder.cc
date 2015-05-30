
/** File: nullfinder.cc
    Author: Candice Quates 
    Program: gpunullfinder

    Takes argument of a file of dumped gpu memory, attempts to divine structure 
    by searching for blocks of nulls which have been added for padding. 
    Default block of nulls to search for is 8.  extracting is only for 8-null blocks.

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



#define VERSION "nullfind 0.1 by Candice Quates May 2015"

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
    int nullssize=8;
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
std::cout <<"null size "<< nullssize << std::endl;
    nullsm=false;
    //loop for multiple files here 
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
        int error=0;
	std::cout << "nullf processing: "<< filename << std::endl;
        uint8_t current=0;
	// There is some integer size mismatching to be fixed here.
	int range=0;
	int nullcount=0;
	int startaddr=0;
        // I am well aware that there are prettier and more
        // elegant ways to do this, but this is much faster 
        // than I expected it to be (ie, a few minutes on a VM for 2gb file)
        for (int n=0; n < mfile->size ; n++) {
	    current=(uint8_t)*(mfile->buffer+n);
	    if (n==0 && current != 0) 
               std::cout<< std::hex <<n <<" data begin ";
	    if (current == 0) {
		nullcount++;
		if (range != 1 && nullcount == 8) {
                    std::cout<< std::hex <<n <<" ends" << std::endl;
                    std::cout<< std::hex <<n-8 <<" nulls begin " ;
		    // extract data 
		    if (extract) {
			std::vector<unsigned char> image;
			image.resize(n-7-startaddr);
			for (int x=0,y=startaddr; y<n-7; x++,y++) {
			    image[x]=(unsigned char)*(mfile->buffer+y);
			}
			writeExtracted(image,std::string(filename)+std::to_string((int)startaddr));
		    }
		    range=1;
		    startaddr=0;
		}
            } else {
		// if we've changed state, note that.
		if (range == 1) {
                    std::cout<< std::hex <<n <<" end "<< std::endl;
                    std::cout<< std::hex <<n <<" data begin ";
		    startaddr=n;
		}
		range=0;
		nullcount = 0;
	    }
	} 
        // If we've reached EOF and are in a block of data, 
	// extract if asked.	
	if (nullcount==0) {
            std::cout<< std::hex <<n<<" ends" << std::endl;
	    if (extract) {
		std::vector<unsigned char> image;
		image.resize(n-7-startaddr);
		for (int x=0,y=startaddr; y<n-7; x++,y++) {
		    image[x]=(unsigned char)*(mfile->buffer+y);
		}
		writeExtracted(image,std::string(filename)+std::to_string((int)startaddr));
	    }
	}
    } // loop for parsing multiple files
    return 0;
}


