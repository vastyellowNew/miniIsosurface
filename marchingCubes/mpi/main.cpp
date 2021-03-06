/*
 * mpi/main.cpp
 *
 *  Created on: Jan 25, 2017
 *      Author: dbourge
 *
 * miniIsosurface is distributed under the OSI-approved BSD 3-clause License.
 * See LICENSE.txt for details.
 *
 * Copyright (c) 2017
 * National Technology & Engineering Solutions of Sandia, LLC (NTESS). Under
 * the terms of Contract DE-NA0003525 with NTESS, the U.S. Government retains
 * certain rights in this software.
 */

#include <array>
#include <vector>
#include <unordered_map>

#include <string>
#include <string.h>
#include <cstdlib>

#include <ctime>
#include <chrono>
#include <iomanip>

#include "../util/Image3D.h"
#include "../util/TriangleMesh.h"

#include "../util/LoadImage.h"
#include "../util/SaveTriangleMesh.h"

#include "../util/util.h"     // for findCaseId and interpolate
#include "../util/TypeInfo.h"
#include "../util/MarchingCubesTables.h"

#include "../util/Timer.h"
#include "../mantevoCommon/YAML_Doc.hpp"

#include "mpi_size_type.h"
#include "mpiutil.h"
#include <mpi.h>

using std::size_t;

template <typename T>
void
sectionOfMarchingCubes(
    T const&                                isoval,
    util::Image3D<T> const&                 image,
    std::vector<std::array<T, 3> >&         points,         // reference
    std::vector<std::array<T, 3> >&         normals,        // reference
    std::vector<std::array<size_t, 3> >&  indexTriangles, // reference
    std::unordered_map<size_t, size_t>& pointMap)       // reference
{
    // For each cube, determine whether or not the isosurface intersects
    // the given cube. If so, first find the cube configuration from a lookup
    // table. Then add the triangles of that cube configuration to points,
    // normals and triangles. Use pointMap to not add any duplicate to
    // points or normals.

    size_t xbeg = image.xBeginIdx();
    size_t ybeg = image.yBeginIdx();
    size_t zbeg = image.zBeginIdx();

    size_t xend = image.xEndIdx();
    size_t yend = image.yEndIdx();
    size_t zend = image.zEndIdx();

    size_t ptIdx = points.size();
    for(size_t zidx = zbeg; zidx != zend; ++zidx)
    {
        for(size_t yidx = ybeg; yidx != yend; ++yidx)
        {
            // A buffer is used to improve cache efficency when retrieving
            // vertex values of each cube.
            auto buffer = image.createBuffer(xbeg, yidx, zidx);

            for(size_t xidx = xbeg; xidx != xend; ++xidx)
            {
                // For each x, y, z index, get the corresponding 8 scalar values
                // of a cube with one corner being at the x, y, z index.
                std::array<T, 8> cubeVertexVals = buffer.getCubeVertexValues(xidx);

                int cellCaseId = util::findCaseId(cubeVertexVals, isoval);

                // The 0th and 255th cellCaseId are the cases where the isosurface
                // does not intersect the cube.
                if (cellCaseId == 0 || cellCaseId == 255)
                {
                    continue;
                }

                // From a pre-generated list of possible cube configurations,
                // get the corresponding "triangles" to this caseId. Each
                // triangle contains 3 cube edge endices, where each vertex
                // of the triangle lies uniquely on one of the edges.
                const int *triEdges = util::caseTrianglesEdges[cellCaseId];
                // triEdges has the form [a1, b1, c1, ..., an, bn, cn, -1]
                // where ax, bx, cx are the edge indices of a triangle for this
                // cube configuration. n can be from 1 to 5.

                // No buffer is used for retrieving scalar values and
                // calculating gradients.
                std::array<std::array<T, 3>, 8> posCube =
                    image.getPosCube(xidx, yidx, zidx);
                std::array<std::array<T, 3>, 8> gradCube =
                    image.getGradCube(xidx, yidx, zidx);

                // For each edge in each triangle, find it's global edge index.
                // If the point and normal for that  global edge index has not
                // been calculated, calculate it and add it to the points and
                // normals vector. Plus, keep track of the indices in points
                // and normals that will make up the next index triangle to
                // add to indexTriangles.
                for(; *triEdges != -1; triEdges += 3)
                {
                    // tri contains indices to points and normals
                    std::array<size_t, 3> tri;
                    for(int i = 0; i != 3; ++i)
                    {
                        size_t globalEdgeIndex =
                            image.getGlobalEdgeIndex(xidx, yidx, zidx, triEdges[i]);

                        if(pointMap.find(globalEdgeIndex) != pointMap.end())
                        {
                            // This global edge index has an associated index
                            // corresponding to its point and normal in points
                            // and normals.

                            tri[i] = pointMap[globalEdgeIndex];
                        }
                        else
                        {
                            // This point and normal value on this global edge
                            // index has not been calculated.

                            pointMap[globalEdgeIndex] = ptIdx;
                            tri[i] = ptIdx++;

                            const int *vs = util::edgeVertices[triEdges[i]];
                            int v1 = vs[0];
                            int v2 = vs[1];
                            T w = (isoval - cubeVertexVals[v1]) /
                                (cubeVertexVals[v2] - cubeVertexVals[v1]);

                            std::array<T, 3> newPt =
                                util::interpolate(posCube[v1], posCube[v2], w);
                            std::array<T, 3> newNorm =
                                util::interpolate(gradCube[v1], gradCube[v2], w);

                            points.push_back(newPt);
                            normals.push_back(newNorm);
                        }
                    }

                    // TODO Is this check needed?
                    if(tri[0] != tri[1] && tri[1] != tri[2] && tri[2] != tri[0])
                    {
                        indexTriangles.push_back(tri);
                    }
                }
            }
        }
    }
}

template <typename T>
util::TriangleMesh<T>
MarchingCubes(std::vector<util::Image3D<T> > const& images, T const& isoval)
{
    std::vector<std::array<T, 3> > processPoints;
    std::vector<std::array<T, 3> > processNormals;
    std::vector<std::array<size_t, 3> > processIndexTriangles;

    std::unordered_map<size_t, size_t> processPointMap;

    for(util::Image3D<T> const& image: images)
    {
        sectionOfMarchingCubes(
            isoval, image,                  // constant inputs
            processPoints,                  // for modification, taken by reference
            processNormals,                 // for modification, taken by reference
            processIndexTriangles,          // for modification, taken by reference
            processPointMap);               // for modification, taken by reference
    }

    return util::TriangleMesh<T>(processPoints, processNormals, processIndexTriangles);
}

int main(int argc, char* argv[])
{
    // Initialize MPI
    MPI::Init(argc, argv);

    int pid = MPI::COMM_WORLD.Get_rank();
    int nProcesses = MPI::COMM_WORLD.Get_size();

    float isoval;
    bool isovalSet = false;
    char* vtkFile = NULL;
    char* outFile = NULL;
    bool oneOutputMesh = false;
    std::string yamlDirectory = "";
    std::string yamlFileName  = "";

    std::size_t nSectionsX = 1;
    std::size_t nSectionsY = 1;
    std::size_t nSectionsZ = nProcesses;

    // Read command line arguments
    for(int i=0; i<argc; i++)
    {
        if( (strcmp(argv[i], "-i") == 0) || (strcmp(argv[i], "-input_file") == 0))
        {
            vtkFile = argv[++i];
        }
        else if( (strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-output_file") == 0))
        {
            outFile = argv[++i];
        }
        else if( (strcmp(argv[i], "-v") == 0) || (strcmp(argv[i], "-isoval") == 0))
        {
            isovalSet = true;
            isoval = atof(argv[++i]);
        }
        else if( (strcmp(argv[i], "-sx") == 0) || (strcmp(argv[i], "-sections_x") == 0))
        {
            nSectionsX = std::stoul(argv[++i]);
        }
        else if( (strcmp(argv[i], "-sy") == 0) || (strcmp(argv[i], "-sections_y") == 0))
        {
            nSectionsY = std::stoul(argv[++i]);
        }
        else if( (strcmp(argv[i], "-sz") == 0) || (strcmp(argv[i], "-sections_z") == 0))
        {
            nSectionsZ = std::stoul(argv[++i]);
        }
        else if( (strcmp(argv[i], "-m") == 0) || (strcmp(argv[i], "-one_mesh") == 0))
        {
            oneOutputMesh = atoi(argv[++i]);
        }
        else if( (strcmp(argv[i], "-y") == 0) || (strcmp(argv[i], "-yaml_output_file") == 0))
        {
            std::string wholeFile(argv[++i]);

            size_t pos = wholeFile.rfind("/");
            if(pos == std::string::npos)
            {
                yamlDirectory = "./";
                yamlFileName = wholeFile;
            }
            else
            {
                yamlDirectory = wholeFile.substr(0, pos + 1);
                yamlFileName = wholeFile.substr(pos + 1);
            }
        }
        else if( (strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "-help") == 0))
        {
            std::cout <<
                "Serial Marching Cubes Options:"  << std::endl <<
                "  -input_file (-i)"              << std::endl <<
                "  -output_file (-o)"             << std::endl <<
                "  -isoval (-v)"                  << std::endl <<
                "  -sections_x (-sx)"             << std::endl <<
                "  -sections_y (-sy)"             << std::endl <<
                "  -sections_z (-sz)"             << std::endl <<
                "  -one_mesh (-m), default 0"     << std::endl <<
                "  -yaml_output_file (-y)"        << std::endl <<
                "  -help (-h)"                    << std::endl;
            return 0;
        }
    }

    if(isovalSet == false || vtkFile == NULL || outFile == NULL)
    {
        std::cout << "Error: isoval, input_file and output_file must be set." << std::endl <<
                     "Try -help" << std::endl;
        return 0;
    }

    // Create a yamlDoc. If yamlDirectory and yamlFileName weren't assigned,
    // YAML_Doc will create a file at in the current directory with a
    // timestamp on it.
    //
    // This file won't be created unless pid is 0.
    YAML_Doc doc("Marching Cubes", "0.2", yamlDirectory, yamlFileName);

    // Each image in images will be used to run the algorithm for a section.
    // Each processor is in charge of running the algorithm on an evenly
    // distributed number of sections.
    std::vector<util::Image3D<float> > images =
        mpiutil::loadImageSections<float>(vtkFile, nSectionsX, nSectionsY, nSectionsZ);

    // Readjust nSections if they are set too large.
    if(nSectionsX > images[0].xdimension() - 1)
        nSectionsX = images[0].xdimension() - 1;
    if(nSectionsY > images[0].ydimension() - 1)
        nSectionsY = images[0].ydimension() - 1;
    if(nSectionsZ > images[0].zdimension() - 1)
        nSectionsZ = images[0].zdimension() - 1;

    if (pid == 0)
    {
        // images should never be empty on the 0th processer.
        std::size_t xdim = images[0].xdimension();
        std::size_t ydim = images[0].ydimension();
        std::size_t zdim = images[0].zdimension();
        // Add information related to this run to doc.
        doc.add("Marching Cubes Algorithm", "mpi");
        doc.add("Volume image data file path", vtkFile);
        doc.add("Polygonal mesh output file", outFile);
        doc.add("Isoval", isoval);
        doc.add("Number of X sections", nSectionsX);
        doc.add("Number of Y sections", nSectionsY);
        doc.add("Number of Z sections", nSectionsZ);
        doc.add("Number of sections", nSectionsX * nSectionsY * nSectionsZ);
        doc.add("File x-dimension", xdim);
        doc.add("File y-dimension", ydim);
        doc.add("File z-dimension", zdim);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Time the output. Timer's constructor starts timing.
    util::Timer runTime;

    // MarchingCubes runs the algorithm. As inputs it takes the image
    // loaded at vtkFile and the isoval of the surface to approximate. It's
    // output is a TriangleMesh which stores the mesh as a vector
    // of triangles.
    util::TriangleMesh<float> polygonalMesh = MarchingCubes(images, isoval);

    // End timing
    runTime.stop();

    // Gather the following information onto process zero for output
    size_t numSectionsHere = images.size();
    size_t numVertsHere = polygonalMesh.numberOfVertices();
    size_t numTrisHere = polygonalMesh.numberOfTriangles();
    double CPUticksHere = runTime.getTotalTicks();
    double CPUtimeHere = runTime.getCPUtime();
    double wallTimeHere = runTime.getWallTime();

    // The size of the vectors should be 0 for all processes except the 0 process
    int gatherSize = 0;
    if(pid == 0)
    {
        gatherSize = nProcesses;
    }

    std::vector<size_t> numSections(gatherSize);
    std::vector<size_t> numVerts(gatherSize);
    std::vector<size_t> numTris(gatherSize);
    std::vector<double> CPUticks(gatherSize);
    std::vector<double> CPUtimes(gatherSize);
    std::vector<double> wallTimes(gatherSize);

    MPI_Gather(&numSectionsHere, 1, my_MPI_SIZE_T,
               numSections.data(), 1, my_MPI_SIZE_T,
               0, MPI_COMM_WORLD);
    MPI_Gather(&numVertsHere, 1, my_MPI_SIZE_T,
               numVerts.data(), 1, my_MPI_SIZE_T,
               0, MPI_COMM_WORLD);
    MPI_Gather(&numTrisHere, 1, my_MPI_SIZE_T,
               numTris.data(), 1, my_MPI_SIZE_T,
               0, MPI_COMM_WORLD);
    MPI_Gather(&CPUticksHere, 1, MPI_DOUBLE,
               CPUticks.data(), 1, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    MPI_Gather(&CPUtimeHere, 1, MPI_DOUBLE,
               CPUtimes.data(), 1, MPI_DOUBLE,
               0, MPI_COMM_WORLD);
    MPI_Gather(&wallTimeHere, 1, MPI_DOUBLE,
               wallTimes.data(), 1, MPI_DOUBLE,
               0, MPI_COMM_WORLD);

    // Only create the YAML file on process zero
    if(pid == 0)
    {
        size_t totalSections = 0;
        size_t totalTriangles = 0;
        double totalTicks = 0.0;
        double totalCPU = 0.0;
        double maxWallTime = 0.0;

        for(int i = 0; i != nProcesses; ++i)
        {
            std::string process = "Process " + std::to_string(i);

            doc.add(process, "");

            doc.get(process)->add("Number of sections", numSections[i]);
            doc.get(process)->add("Number of vertices in mesh", numVerts[i]);
            doc.get(process)->add("Number of triangles in mesh", numTris[i]);
            doc.get(process)->add("CPU Time (clicks)", CPUticks[i]);
            doc.get(process)->add("CPU Time (seconds)", CPUtimes[i]);
            doc.get(process)->add("Wall Time (seconds)", wallTimes[i]);

            totalSections += numSections[i];
            totalTriangles += numTris[i];
            totalTicks += CPUticks[i];
            totalCPU += CPUtimes[i];
            maxWallTime = std::max(maxWallTime, wallTimes[i]);
        }
        doc.add("Total number of sections", totalSections);
        doc.add("Total number of triangles in mesh", totalTriangles);
        doc.add("Total CPU time (clicks)", totalTicks);
        doc.add("Total CPU time (seconds)", totalCPU);
        doc.add("Max wall time (seconds)", maxWallTime);

        std::cout << doc.generateYAML();
    }

    if(oneOutputMesh)
    {
        // It is an option to output only one output mesh instead of one
        // output mesh for each processer. gatherMeshes and mergeMeshes
        // are not designed to be performant.

        std::vector<util::TriangleMesh<float> > meshes =
            mpiutil::gatherMeshes(polygonalMesh, pid, numVerts, numTris);

        if(pid == 0)
        {
            util::TriangleMesh<float> globalPolygonalMesh =
                mpiutil::mergeMeshes(meshes);

            util::saveTriangleMesh(globalPolygonalMesh, outFile);
        }
    }
    else
    {
        // Write the output file, appending the process id number
        std::string outFilePid = std::string(outFile) + "." + std::to_string(pid);
        util::saveTriangleMesh(polygonalMesh, outFilePid.c_str());
    }

    // And don't forget to tell MPI that everything is done!
    MPI::Finalize();
}
