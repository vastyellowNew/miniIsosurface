/*
 * RunTime_errors.h
 *
 *  Created on: Jul 27, 2015
 *      Author: sjmunn
 */

#ifndef REPORTING_RUNTIME_ERRORS_H_
#define REPORTING_RUNTIME_ERRORS_H_

#include"../includes.h"

#include"Log.h"

class wrong_arguments: public std::exception {
public:
	wrong_arguments(const int inArgc) :
		argc(inArgc) {

		CLOG(logERROR) << "Wrong number of arguments";
		CLOG(logERROR) << "Usage:";
		CLOG(logERROR) << "MarchingCubes in_image.vtk out_poly.vtk isoval";
		CLOG(logERROR) << "OR";
		CLOG(logERROR) << "MarchingCubes in_image_header in_image_data out_poly.vtk isoval";
	}

private:
	const int argc;
};

class implementation_not_supported: public std::exception {
public:
	implementation_not_supported(const char* const inImplementationID) :
		implementationID(inImplementationID) {

		CLOG(logERROR) << "This implementation is not supported";
		CLOG(logERROR) << "Given implementation ID: " << implementationID;
		CLOG(logERROR) << "Try a different make rule when building MarchingCubes";
	}

private:
	const char* const implementationID;
};

class too_many_edges: public std::exception {
public:
	too_many_edges(const unsigned inNedges, const unsigned inNedgesAllowed) :
		nEdges(inNedges), nEdgesAllowed(inNedgesAllowed) {

		CLOG(logERROR) << "There are too many edges in this mesh for display";
		CLOG(logERROR) << "Number of edges in mesh: " << nEdges;
		CLOG(logERROR) << "Allowed number of edges: " << nEdges;
	}

private:
	unsigned nEdges, nEdgesAllowed;
};

#endif /* REPORTING_RUNTIME_ERRORS_H_ */
