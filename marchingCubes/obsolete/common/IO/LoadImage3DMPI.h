/*
 * LoadImage3DMPI.h
 *
 *  Created on: Aug 20, 2015
 *      Author: sjmunn
 */

#ifndef IO_LOADIMAGE3DMPI_H_
#define IO_LOADIMAGE3DMPI_H_

#include"../includes.h"

// I/O headers
#include"../Data_Obj/Image3D.h"
#include"LineStream.h"
#include"ConvertBuffer.h"

// Reporting Headers
#include"../Reporting/IO_errors.h"
#include"../Reporting/MPI_errors.h"
#include"../Reporting/Log.h"

static const int N_HEADER_LINES=10;

template<typename T>
class LoadImage3DMPI {
public:
	LoadImage3DMPI();
	LoadImage3DMPI(LoadImage3DMPI&);
	virtual ~LoadImage3DMPI();

	// File loading members
	void setSperateData(const char *);
	// General file
	void loadHeader(const char *);
	void report(YAML_Doc &) const;
	// MPI process specific
	void setBlockExtent(const unsigned *);
	void readEntireVolumeData(Image3D<T>&);
	void readBlockData(Image3D<T>&);
	// Object member read
	unsigned const* getVolumeDimensions(void);
	unsigned getMaxVoumeDimension(void) const;
	unsigned getnVolumePoints(void) const;
	const T* getSpacing(void) const;
	const T* getOrigin(void) const;
 	const char* whichFile(void) const { return vtkFile; }
private:
	// Private member functions
	void streamIgnore(unsigned);
	// File reader objects
	std::ifstream stream;
	LineStream *reader;
	const char *vtkFile;
	const char *vtkDataFile;
	bool seperateData;

	// general parameters for the image file
	unsigned xdimFile,ydimFile,zdimFile;
	unsigned dimsArray[3];
	unsigned fileNpoints;
	T spacing[3];
	T origin[3];
	TypeInfo* typeInfo;

	// MPI process specific
	bool blockExtentSet;
	unsigned blockExtent[6];
	unsigned nPointsInBlock;
	unsigned imageDataIdx;
};

#endif /* IO_LOADIMAGE3DMPI_H_ */
