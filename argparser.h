#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <string>
#include <cassert>
#include "MersenneTwister.h"


// ====================================================================================

class ArgParser {

public:

	ArgParser() { DefaultValues(); }
	
	ArgParser(int argc, char *argv[]) {
		DefaultValues();
		for (int i = 1; i < argc; i++) {

			if (argv[i] == std::string("-input")) {
				i++; assert (i < argc); 	
				// we need to separate the filename from the path
				// (we assume the vertex & fragment shaders are in the same directory)
				std::string filename = argv[i];
				// first, locate the last '/' in the filename
				size_t last = std::string::npos;	
				while (1) {
					int next = filename.find('/',last+1);
					if (next == std::string::npos) { break;
					}
					last = next;
				}
				if (last == std::string::npos) {
					// if there is no directory in the filename
					input_file = filename;
					path = ".";
				} 
				else {
					// separate filename & path
					input_file = filename.substr(last+1,filename.size()-last-1);
					path = filename.substr(0,last);
				}
			} 
			else if (argv[i] == std::string("-size")) {
				i++; assert (i < argc); 
				width = height = atoi(argv[i]);
			} 
			else if (argv[i] == std::string("-wireframe")) {
				wireframe = 1;
			} 
			else if (argv[i] == std::string("-gouraud")) {
				gouraud = true;
			} 
			else {
				printf ("whoops error with command line argument %d: '%s'\n",i,argv[i]);
				assert(0);
			}
		}
	}

	void DefaultValues() {
		width = 500;
		height = 500;
		wireframe = 0;
		gouraud = false;
	}

	// ==============
	// REPRESENTATION
	// all public! (no accessors)
	std::string input_file;
	std::string path;
	int width;
	int height;
	GLint wireframe;
	bool gouraud;
	MTRand mtrand;

};

// ====================================================================================

#endif
