/*
 * TriMeshWithEdges.cpp
 *
 *  Created on: Jul 28, 2015
 *      Author: sjmunn
 */

#include "../Algorithm/EdgeIndexer.h"

template<typename T>
EdgeIndexer<T>::EdgeIndexer(unsigned * extentOfMarch) {
	/*
	 * The extent of the march coordinates is one less than the
	 * volume coordinates because we stop at the last cell of
	 * the image volume
	 */
	rangeX = extentOfMarch[1] - extentOfMarch[0] + 1;
	rangeY = extentOfMarch[3] - extentOfMarch[2] + 1;
	rangeZ = extentOfMarch[5] - extentOfMarch[4] + 1;

	nXedges = rangeX * (rangeY + 1) * (rangeZ + 1);
	nXYedges = nXedges + (rangeX + 1) * rangeY * (rangeZ + 1);
	nAllEdges = nXYedges + (rangeX + 1) * (rangeY + 1) * (rangeZ) - 1;
}

template<typename T>
unsigned EdgeIndexer<T>::getEdgeIndex(unsigned x, unsigned y,
		unsigned z, int iEdge) const {
	unsigned edgeIndex=0;
	switch (iEdge) {
	// Edges parallel to the x-axis
	case 0:
		edgeIndex = edgeIndexXaxis(x, y, z, iEdge);
		break;
	case 2:
		edgeIndex = edgeIndexXaxis(x, y + 1, z, iEdge);
		break;
	case 4:
		edgeIndex = edgeIndexXaxis(x, y, z + 1, iEdge);
		break;
	case 6:
		edgeIndex = edgeIndexXaxis(x, y + 1, z + 1, iEdge);
		break;
		// Edges parallel to the y-axis
	case 3:
		edgeIndex = edgeIndexYaxis(x, y, z, iEdge);
		break;
	case 1:
		edgeIndex = edgeIndexYaxis(x + 1, y, z, iEdge);
		break;
	case 7:
		edgeIndex = edgeIndexYaxis(x, y, z + 1, iEdge);
		break;
	case 5:
		edgeIndex = edgeIndexYaxis(x + 1, y, z + 1, iEdge);
		break;
		// Edges parallel to the z-axis
	case 8:
		edgeIndex = edgeIndexZaxis(x, y, z, iEdge);
		break;
	case 9:
		edgeIndex = edgeIndexZaxis(x + 1, y, z, iEdge);
		break;
	case 10:
		edgeIndex = edgeIndexZaxis(x, y + 1, z, iEdge);
		break;
	case 11:
		edgeIndex = edgeIndexZaxis(x + 1, y + 1, z, iEdge);
		break;
	}
	return edgeIndex;
}

template<typename T>
unsigned EdgeIndexer<T>::edgeIndexXaxis(unsigned x, unsigned y,
		unsigned z, int iEdge) const {
	unsigned index=x + rangeX * y + (rangeY + 1) * rangeX * z;
	return index;
}

template<typename T>
unsigned EdgeIndexer<T>::edgeIndexYaxis(unsigned x, unsigned y,
		unsigned z, int iEdge) const {
	unsigned index= nXedges + x + (rangeX + 1) * y + (rangeX + 1) * rangeY * z;
	return index;
}

template<typename T>
unsigned EdgeIndexer<T>::edgeIndexZaxis(unsigned x, unsigned y,
		unsigned z, int iEdge) const {
	unsigned index= nXYedges + x + (rangeX + 1) * y + (rangeX + 1) * (rangeY + 1) * z;
	return index;
}

template<typename T>
void EdgeIndexer<T>::getPointCoordinates(unsigned edgeIndex,Point3dIdx& r1,Point3dIdx& r2) const {
	unsigned x,y,z;
	unsigned x1,y1,z1;
	if (edgeIndex < nXedges) {

		x=edgeIndex % rangeX;
		z=edgeIndex/(rangeX*(rangeY+1));
		y=(edgeIndex-z*rangeX*(rangeY+1))/rangeX;

		x1=x+1;
		y1=y;
		z1=z;

	}
	// Edges parallel to the y-axis
	else if (edgeIndex >= nXedges && edgeIndex < nXYedges) {

		x=(edgeIndex-nXedges) % (rangeX+1);
		z=(edgeIndex-nXedges)/(rangeY*(rangeX+1));
		y=(edgeIndex-nXedges-z*rangeY*(rangeX+1))/(rangeX+1);

		x1=x;
		y1=y+1;
		z1=z;

	}
	// Edges parallel to the z-axis
	else {

		x=(edgeIndex-nXYedges) % (rangeX+1);
		z=(edgeIndex-nXYedges)/((rangeY+1)*(rangeX+1));
		y=(edgeIndex-nXYedges-z*(rangeY+1)*(rangeX+1))/(rangeX+1);

		x1=x;
		y1=y;
		z1=z+1;
	}
	r1.setCoordinates(x,y,z);
	r2.setCoordinates(x1,y1,z1);
}



// Must instantiate class for separate compilation
template class EdgeIndexer<float_t> ;
