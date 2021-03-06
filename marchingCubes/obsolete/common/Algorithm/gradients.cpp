/*
 * gradients.cpp
 *
 *  Created on: Aug 6, 2015
 *      Author: sjmunn
 */

#include"gradients.h"

template<typename T>
void computeGradient(unsigned xidx, unsigned yidx, unsigned zidx, const Image3DReader<T>* imageVolume, T grad[3]) {
	T x[3][2];
	T run[3];
	imageVolume->getValsForGradient(x,run,xidx,yidx,zidx);

	for (int iAxis=0;iAxis<3;++iAxis) {
		grad[iAxis]=(x[iAxis][1]-x[iAxis][0]) / run[iAxis];
	}
}

template<typename T>
void computeAllGradients (unsigned &xidx, unsigned &yidx, unsigned &zidx, const Image3DReader<T>* imgVol, T (& grad)[8][3]) {
	computeGradient(xidx, yidx, zidx, imgVol, grad[0]);
	computeGradient(xidx + 1, yidx, zidx, imgVol, grad[1]);
	computeGradient(xidx + 1, yidx + 1, zidx, imgVol, grad[2]);
	computeGradient(xidx, yidx + 1, zidx, imgVol, grad[3]);
	computeGradient(xidx, yidx, zidx + 1, imgVol, grad[4]);
	computeGradient(xidx + 1, yidx, zidx + 1, imgVol, grad[5]);
	computeGradient(xidx + 1, yidx + 1, zidx + 1, imgVol, grad[6]);
	computeGradient(xidx, yidx + 1, zidx + 1, imgVol, grad[7]);
}


// Must explicitly instantiate template functions for separate compiling to work
template void computeGradient<float_t> (unsigned xidx, unsigned yidx, unsigned zidx, const Image3DReader<float_t>*,	float_t grad[3]);

template void computeAllGradients<float_t> (unsigned &xidx, unsigned &yidx, unsigned &zidx, const Image3DReader<float_t>*, float_t (& grad)[8][3]);
