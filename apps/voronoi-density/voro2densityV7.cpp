#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sys/time.h>
#include <cassert>
#include <cstring>
#include <filesystem>

#include <omp.h>

/* 
   This version is a copy of V6, but adapted to read input from CODA's data.
*/

#include "hdf5.h"

#include "kdtree/nanoflann.hpp"

#define GROW_THRESHOLD_1 3 
#define GROW_THRESHOLD_2 6
#define GROW_THRESHOLD_3 9

int GROW_MULTIPLIER = 1;
//double DOMAIN_MULTIPLIER = 90;
double DOMAIN_MULTIPLIER = 80.5;
//double DOMAIN_MULTIPLIER = 60;

//#define 512 (512)
//#define 512 (512)
//#define 512 (512)
//#define 512 (16)
//#define 512 (16)
//#define 512 (512)

using UINT = unsigned int;
const UINT totalGridPts = (512) * (512) * (512);

double GetElapsedSeconds( const struct timeval* begin, 
                          const struct timeval* end )
{
    return (end->tv_sec - begin->tv_sec) + ((end->tv_usec - begin->tv_usec)/1000000.0);
}

float CalcDist2( const float* p, const UINT* q )  // two points
{
    return ( (p[0]-(float)q[0]) * (p[0]-(float)q[0]) + 
             (p[1]-(float)q[1]) * (p[1]-(float)q[1]) + 
             (p[2]-(float)q[2]) * (p[2]-(float)q[2]) );
}

// Specialized method to 1) read in, and 2) process Bipin's data.
void ReadCODA( const char* name,       // input:  filename
                UINT&       len,          // output: number of particles
                float**     buf,          // output: (x, y, z) of each particle
                int         binStart,
                int         binEnd )
{
    herr_t   status;
    double maxX = 0;
    double maxY = 0;
    double maxZ = 0;

    //double binStart = 1.;
    //double binEnd   = 5.;
    //double binStart = 15.;
    //double binEnd   = 400.;
       
    // Get number of particles within our current bin 
    len = 0;
    std::vector<int> timesteps;
    for (const auto & entry : std::filesystem::directory_iterator( name ) ) {
        std::string filename = std::filesystem::path( entry.path().c_str() ).filename();
        
        hid_t file       = H5Fopen( entry.path().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT );
        hid_t merSizeSet = H5Dopen( file, "Mer Size", H5P_DEFAULT);
        hid_t dataspace  = H5Dget_space( merSizeSet );    /* dataspace handle */
        int pCount       = H5Sget_simple_extent_npoints( dataspace );
        
        int merSize[ pCount ];
        status = H5Dread (merSizeSet, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, merSize );

        for ( int i=0; i<pCount; i++) {
            int size = merSize[i];
            if (  size >= binStart && size < binEnd ) {
                len++;
                /*if (size > GROW_THRESHOLD_3) {
                    len += GROW_MULTIPLIER*8;
                }
                else if (size > GROW_THRESHOLD_2) {
                    len += GROW_MULTIPLIER*4;
                }
                else if (size > GROW_THRESHOLD_1) {
                    len += GROW_MULTIPLIER*2;
                }
                else { 
                    len++;
                }*/
            }
        }
        
        status = H5Fclose( file );
        status = H5Sclose( dataspace );
        status = H5Dclose( merSizeSet );
    }

    if( len == 0 ) {
        std::cout << "No particles.  Exiting." << std::endl;
        exit(0);
    }
    std::cout << "len " << len << std::endl;
    //int minimum = 512;
    //int minimum = totalGridPts/512;
    int minimum = 100000;
    if ( len < minimum) {
        GROW_MULTIPLIER = (int)(minimum/len);
        len = len*GROW_MULTIPLIER;
    }
    std::cout << "grow " << GROW_MULTIPLIER << std::endl;

    sort( timesteps.begin(), timesteps.end() );
   
    // Initialize buffer to hold XYZ coordinates
    std::cout << "nParticles " << len << std::endl;
    *buf = new float[ len*3 ];
    int bufferIndex = 0;

    // Populate buffer with XYZ coordinates
    for (const auto & entry : std::filesystem::directory_iterator( name ) ) {
        hid_t file       = H5Fopen( entry.path().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT );
        hid_t merSizeSet = H5Dopen( file, "Mer Size", H5P_DEFAULT);
        hid_t dataspace  = H5Dget_space( merSizeSet );    /* dataspace handle */

        int   rank       = H5Sget_simple_extent_ndims(dataspace);
        int   pCount     = H5Sget_simple_extent_npoints( dataspace );

        int merSize[ pCount ];
        status = H5Dread (merSizeSet, H5T_STD_I32LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, merSize );

        hid_t merPosSet  = H5Dopen( file, "Position", H5P_DEFAULT);
        dataspace = H5Dget_space( merPosSet );    /* dataspace handle */
        rank      = H5Sget_simple_extent_ndims(dataspace);

        double position[ pCount * 3 ];
        status = H5Dread ( merPosSet, H5T_IEEE_F64LE, H5S_ALL, H5S_ALL, H5P_DEFAULT, position );
        
        for (int i=0; i<pCount; i++) {
            if ( position[i*3+2] > maxX )
                maxX = position[i*3+2];
            if ( position[i*3+1] > maxY )
                maxY = position[i*3+1];
            if ( position[i*3] > maxZ )
                maxZ = position[i*3];

            if (position[i*3+2] > 2*M_PI) position[i*3+2] = 2*M_PI-.01;
            if (position[i*3+1] > 2*M_PI) position[i*3+1] = 2*M_PI-.01;
            if (position[i*3  ] > 2*M_PI) position[i*3  ] = 2*M_PI-.01;
            if (position[i*3+2] < 0)      position[i*3+2] = .01;
            if (position[i*3+1] < 0)      position[i*3+1] = .01;
            if (position[i*3  ] < 0)      position[i*3  ] = .01;

            int size = merSize[i];
            if (  size >= binStart && size < binEnd ) {
                float* inputBuf = new float[3];
                if ( size > GROW_THRESHOLD_3 ) {
                    int multiplier = GROW_MULTIPLIER;//*8; 
                    for (int j=0; j<multiplier; j++ ) { 
                        //float* inputBuf = new float[3];
                        //float fuzz = (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/256.f );
                        float fuzz = (((rand()%100)-50)/1000.f);
                        inputBuf[0] = (float)position[ i*3+2 ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/256.f );
                        inputBuf[1] = (float)position[ i*3+1 ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/256.f );
                        inputBuf[2] = (float)position[ i*3   ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/256.f );
                        if (inputBuf[0] > 2*M_PI) inputBuf[0] = 2*M_PI-.01;
                        if (inputBuf[1] > 2*M_PI) inputBuf[1] = 2*M_PI-.01;
                        if (inputBuf[2] > 2*M_PI) inputBuf[2] = 2*M_PI-.01;
                        if (inputBuf[0] < 0) inputBuf[0] = .01;
                        if (inputBuf[1] < 0) inputBuf[1] = .01;
                        if (inputBuf[2] < 0) inputBuf[2] = .01;
                        inputBuf[0] *= DOMAIN_MULTIPLIER;
                        inputBuf[1] *= DOMAIN_MULTIPLIER;
                        inputBuf[2] *= DOMAIN_MULTIPLIER;
                        memcpy( (*buf) + bufferIndex*3, inputBuf, sizeof(float)*3 );
                        bufferIndex++;
                    }
                }
                else if ( size > GROW_THRESHOLD_2 ) {
                    int multiplier = GROW_MULTIPLIER;//*4; 
                    for (int j=0; j<multiplier; j++ ) { 
                        //float* inputBuf = new float[3];
                        float fuzz = (((rand()%100)-50)/2000.f);
                        inputBuf[0] = (float)position[ i*3+2 ] + fuzz; //+ (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/512.f );
                        inputBuf[1] = (float)position[ i*3+1 ] + fuzz; //+ (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/512.f );
                        inputBuf[2] = (float)position[ i*3   ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/512.f );
                        if (inputBuf[0] > 2*M_PI) inputBuf[0] = 2*M_PI-.01;
                        if (inputBuf[1] > 2*M_PI) inputBuf[1] = 2*M_PI-.01;
                        if (inputBuf[2] > 2*M_PI) inputBuf[2] = 2*M_PI-.01;
                        if (inputBuf[0] < 0) inputBuf[0] = .01;
                        if (inputBuf[1] < 0) inputBuf[1] = .01;
                        if (inputBuf[2] < 0) inputBuf[2] = .01;
                        //inputBuf[0] /= (2*M_PI);
                        //inputBuf[1] /= (2*M_PI);
                        //inputBuf[2] /= (2*M_PI);
                        inputBuf[0] *= DOMAIN_MULTIPLIER;
                        inputBuf[1] *= DOMAIN_MULTIPLIER;
                        inputBuf[2] *= DOMAIN_MULTIPLIER;
                        memcpy( (*buf) + bufferIndex*3, inputBuf, sizeof(float)*3 );
                        bufferIndex++;
                    }
                }
                else if ( size > GROW_THRESHOLD_1 ) {
                    int multiplier = GROW_MULTIPLIER;//*2; 
                    for (int j=0; j<multiplier; j++ ) { 
                        //float* inputBuf = new float[3];
                        //float fuzz = (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/2160.f );
                        float fuzz = (((rand()%100)-50)/3000.f);
                        inputBuf[0] = (float)position[ i*3+2 ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/2160.f );
                        inputBuf[1] = (float)position[ i*3+1 ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/2160.f );
                        inputBuf[2] = (float)position[ i*3   ] + fuzz; // + (26*M_PI*(rand()%100-50)/100.f) * ( 2*M_PI/2160.f );
                        if (inputBuf[0] > 2*M_PI) inputBuf[0] = 2*M_PI-.01;
                        if (inputBuf[1] > 2*M_PI) inputBuf[1] = 2*M_PI-.01;
                        if (inputBuf[2] > 2*M_PI) inputBuf[2] = 2*M_PI-.01;
                        if (inputBuf[0] < 0) inputBuf[0] = .01;
                        if (inputBuf[1] < 0) inputBuf[1] = .01;
                        if (inputBuf[2] < 0) inputBuf[2] = .01;
                        inputBuf[0] *= DOMAIN_MULTIPLIER;
                        inputBuf[1] *= DOMAIN_MULTIPLIER;
                        inputBuf[2] *= DOMAIN_MULTIPLIER;
                        memcpy( (*buf) + bufferIndex*3, inputBuf, sizeof(float)*3 );
                        bufferIndex++;
                    }
                }
                else {
                    //float* inputBuf = new float[3];
                    inputBuf[0] = (float)position[ i*3+2 ];
                    inputBuf[1] = (float)position[ i*3+1 ];
                    inputBuf[2] = (float)position[ i*3   ];
                    if (inputBuf[0] > 2*M_PI) inputBuf[0] = 2*M_PI-.01;
                    if (inputBuf[1] > 2*M_PI) inputBuf[1] = 2*M_PI-.01;
                    if (inputBuf[2] > 2*M_PI) inputBuf[2] = 2*M_PI-.01;
                    if (inputBuf[0] < 0) inputBuf[0] = .01;
                    if (inputBuf[1] < 0) inputBuf[1] = .01;
                    if (inputBuf[2] < 0) inputBuf[2] = .01;
                    inputBuf[0] *= DOMAIN_MULTIPLIER;
                    inputBuf[1] *= DOMAIN_MULTIPLIER;
                    inputBuf[2] *= DOMAIN_MULTIPLIER;
                    memcpy( (*buf) + bufferIndex*3, inputBuf, sizeof(float)*3 );
                    //std::cout << len << " " << bufferIndex << " " << inputBuf[0] << " " << inputBuf[1] << " " << inputBuf[2] << std::endl;
                    bufferIndex++;
                }
                /*if (inputBuf[0] > 2*M_PI) inputBuf[0] = 2*M_PI-.01;
                if (inputBuf[1] > 2*M_PI) inputBuf[1] = 2*M_PI-.01;
                if (inputBuf[2] > 2*M_PI) inputBuf[2] = 2*M_PI-.01;
                if (inputBuf[0] < 0) inputBuf[0] = .01;
                if (inputBuf[1] < 0) inputBuf[1] = .01;
                if (inputBuf[2] < 0) inputBuf[2] = .01;
                memcpy( (*buf) + bufferIndex*3, inputBuf, sizeof(float)*3 );
                bufferIndex++;*/
            }
        }
        status = H5Fclose( file );
        status = H5Sclose( dataspace );
        status = H5Dclose( merSizeSet );
        status = H5Dclose( merPosSet );
        /*
         * Output the data to the screen.
         */
    }
    std::cout << "BufferIndex " << bufferIndex << std::endl;
}

// Specialized method to 1) read in, and 2) process Bipin's data.
void ReadBipin( const char* name,       // input:  filename
                UINT&       len,        // output: number of particles
                float**     
buf )       // output: (x, y, z) of each particle
{
    FILE*  f  = fopen( name, "r" );
    fseek( f, 0, SEEK_END );
    long int totalByte = ftell( f );
    assert( totalByte % 20 == 0 );      // Every 20 byte represents one particle
    len =   totalByte / 20;
    rewind( f );

    // First, read in all the values, each particle is represented by (id, x, y, z, r)
    float* inputBuf = new float[ totalByte / 4 ];
    size_t rt = fread( inputBuf, 4, totalByte / 4, f );
    fclose( f );
    assert( rt == totalByte / 4 );

    // Second, extract all the (x, y, z) tuples
    // !! For a grid size of 512, we also multiply the coordinate (x, y, z) by 4 !!
    *buf = new float[ len * 3 ];
    for( UINT i = 0; i < len; i++ )
    {
        memcpy( (*buf) + i * 3, inputBuf + i * 5 + 1, sizeof(float) * 3 );
        for( int j = 0; j < 3; j++ )
            (*buf)[ i * 3 + j ] *= 5.0f;
    }
    
    delete[] inputBuf;
}

template <typename T>
class  PointCloud
{
public:
	struct Point
	{
		T  x,y,z;
	};

	std::vector<Point>  pts;

	// Must return the number of data points
	inline size_t kdtree_get_point_count() const { return pts.size(); }

	// Returns the dim'th component of the idx'th point in the class:
	// Since this is inlined and the "dim" argument is typically an immediate value, the
	//  "if/else's" are actually solved at compile time.
	inline T kdtree_get_pt(const size_t idx, int dim) const
	{
		if (dim == 0) return pts[idx].x;
		else if (dim == 1) return pts[idx].y;
		else return pts[idx].z;
	}

	// Optional bounding-box computation: return false to default to a standard bbox computation loop.
	//   Return true if the BBOX was already computed by the class and returned in "bb" so it can be avoided to redo it again.
	//   Look at bb.size() to find out the expected dimensionality (e.g. 2 or 3 for point clouds)
	template <class BBOX>
	bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }

    void FillByArray( const UINT N, const T* buf )
    {
        pts.resize(N);
        UINT idx = 0;
        for (UINT i = 0; i < N; i++)
        {
            pts[i].x = buf[idx++];
            pts[i].y = buf[idx++];
            pts[i].z = buf[idx++];
        }
    }
};


int main(int argc, char** argv )
{
    if( argc < 3 || argc > 5 )  // has to be  5
    {
        printf("Usage: ./a.out inputDir output.rawFile binStart binEnd\n");
        exit(1);
    }
    bool use_helicity = false;
    //if( argc == 4 )
    //    use_helicity  = true;
    struct timeval start, end;

    // Read in particles
    UINT      nParticles;
    float*    ptcBuf = nullptr;
    //ReadBipin( argv[1], nParticles, &ptcBuf );
    int binStart = atoi( argv[3] );
    int binEnd   = atoi( argv[4] );
    std::cout << binStart << " " << binEnd << std::endl;
    ReadCODA( argv[1], nParticles, &ptcBuf, binStart, binEnd );
	//exit(0);
    UINT      nPtcToUse = nParticles;           // use a subset of particles for experiments

    // Put particles in a "PointCloud"
	PointCloud<float> cloud;
    cloud.FillByArray( nPtcToUse, ptcBuf );

	// construct a kd-tree index:
	typedef nanoflann::KDTreeSingleIndexAdaptor< nanoflann::L2_Simple_Adaptor<float, PointCloud<float> > ,
                                      PointCloud<float>, 3 >     my_kd_tree_t;
	my_kd_tree_t   kd_index(3 /* dimension */, cloud, nanoflann::KDTreeSingleIndexAdaptorParams(8 /* max leaf */) );
    gettimeofday( &start, NULL );
	kd_index.buildIndex();
    gettimeofday( &end, NULL );
    std::cerr << "kdtree construction takes " << GetElapsedSeconds(&start, &end) 
              << " seconds." << std::endl;

    // Each particle has a counter 
    UINT* counter = new UINT[ nPtcToUse ];
    for( UINT i = 0; i < nPtcToUse; i++ )
        counter[i] = 0;

    // Each grid point keeps which particle is closest to it.
    UINT* pcounter = new UINT[ totalGridPts ];
    for( UINT i = 0; i < totalGridPts; i++ )
        pcounter[i] = 0;

    // Find the closest particle for each grid point 
    gettimeofday( &start, NULL );
    #pragma omp parallel for
    for( UINT z = 0; z < 512; z++ ) 
    {
        nanoflann::KNNResultSet<float, UINT> resultSet(1);
        UINT   ret_index;
        float  out_dist_sqr;
        #ifdef DEBUG
        struct timeval planeStart, planeEnd;
        gettimeofday( &planeStart, NULL );
        #endif
        UINT zOffset = z * 512 * 512;
        for( UINT y = 0; y < 512; y++ )
        {
            UINT yOffset = y * 512 + zOffset;
            for( UINT x = 0; x < 512; x++ )
            {
                resultSet.init( &ret_index, &out_dist_sqr );    // VERY IMPORTANT!!!
                float g[3]  = {(float)x, (float)y, (float)z};
                bool rt = kd_index.findNeighbors(resultSet, g, nanoflann::SearchParams());
                assert( rt );
                pcounter[ x+yOffset ] = ret_index;
            }
        }
        #ifdef DEBUG
        gettimeofday( &planeEnd, NULL );
        std::cerr << "kdtree retrieval plane " << z << " takes " << GetElapsedSeconds(&planeStart, &planeEnd) 
                  << " seconds." << std::endl;
        #endif
    }
    gettimeofday( &end, NULL );
    std::cerr << "total kdtree retrieval takes " << GetElapsedSeconds(&start, &end) 
              << " seconds." << std::endl;

    // Increase counters in serial
    for( UINT i = 0; i < totalGridPts; i++ )
        counter[pcounter[i]]++;


    //#ifdef DEBUG  /**** print diagnostic info ****/
    // What's the total count all counters have?
    UINT total = 0;
    for( UINT i = 0; i < nPtcToUse; i++ )
        total += counter[i];
    std::cout << "Total count is : " << total << std::endl;

    // How many grid points state particle 0 is their closest particle?
    total = 0;
    for( UINT i = 0; i < totalGridPts; i++ )
        if( pcounter[i] == 0 )
            total++;
    std::cout << "Total grid points live in the cell of particle #0: " << total << std::endl;

    // Print some random counters
    for( UINT i = 0; i < 10; i++ )
    {
        UINT idx = (float)rand() / RAND_MAX * nPtcToUse;
        std::cout << "A random counter value: " << counter[idx] << std::endl;
    }
    //#endif  /**** finish printing diagnostic info ****/

    
    // Each grid point calculates its own density from either a mass of 1.0 or its helicity.
    float* contribution = new float[nPtcToUse];
    if( use_helicity )
    {
        std::cout << "Distributing helicity, instead of mass, of the particles!" << std::endl;
        //FillHelicity( argv[1], nPtcToUse, contribution );
    }
    else
    {
        for( UINT i = 0; i < nPtcToUse; i++ )
            contribution[i] = 1.0f;
    }

    float* density = new float[totalGridPts];
    #pragma omp parallel for
    for( UINT z = 0; z < 512; z++ )   
    {
        UINT zOffset = z * 512 * 512;
        for( UINT y = 0; y < 512; y++ )
        {
            UINT yOffset = y * 512 + zOffset;
            for( UINT x = 0; x < 512; x++ )
            {
                UINT   idx         = x+yOffset;
                UINT   count       = counter     [ pcounter[idx] ];
                density[ idx ]     = contribution[ pcounter[idx] ] / (float)count;
            }
        }
    }

    // How many Voronoi cells do not contain a grid point?
    UINT emptyCellCount = 0;
    for( UINT i = 0; i < nPtcToUse; i++ )
    {
        if( counter[i] == 0 )
        {
            emptyCellCount++;
            float c = contribution[i];

            // Distribute the contribution of this particle to its eight enclosing grid points.
            float ptc[3] = { ptcBuf[i*3], ptcBuf[i*3+1], ptcBuf[i*3+2] };
            UINT g0[3] = { (UINT)ptc[0], (UINT)ptc[1], (UINT)ptc[2] };  // grid indices
            if( g0[0] == 512 )   g0[0]--;
            if( g0[1] == 512 )   g0[1]--;
            if( g0[2] == 512 )   g0[2]--;
            UINT g1[3] = { g0[0]+1, g0[1]  , g0[2]   };
            UINT g2[3] = { g0[0]  , g0[1]+1, g0[2]   };
            UINT g3[3] = { g0[0]+1, g0[1]+1, g0[2]   };
            UINT g4[3] = { g0[0]  , g0[1]  , g0[2]+1 };
            UINT g5[3] = { g0[0]+1, g0[1]  , g0[2]+1 };
            UINT g6[3] = { g0[0]  , g0[1]+1, g0[2]+1 };
            UINT g7[3] = { g0[0]+1, g0[1]+1, g0[2]+1 };
            float dist[8] = { CalcDist2( ptc, g0 ), CalcDist2( ptc, g1 ), 
                              CalcDist2( ptc, g2 ), CalcDist2( ptc, g3 ), 
                              CalcDist2( ptc, g4 ), CalcDist2( ptc, g5 ), 
                              CalcDist2( ptc, g6 ), CalcDist2( ptc, g7 ) };
            float total = 0.0f;
            for( int j = 0; j < 8; j++ )
                total += dist[j];

            density[ g0[2] * 512 * 512 + g0[1] * 512 + g0[0] ] += c * dist[0] / total;
            density[ g1[2] * 512 * 512 + g1[1] * 512 + g1[0] ] += c * dist[1] / total;
            density[ g2[2] * 512 * 512 + g2[1] * 512 + g2[0] ] += c * dist[2] / total;
            density[ g3[2] * 512 * 512 + g3[1] * 512 + g3[0] ] += c * dist[3] / total;
            density[ g4[2] * 512 * 512 + g4[1] * 512 + g4[0] ] += c * dist[4] / total;
            density[ g5[2] * 512 * 512 + g5[1] * 512 + g5[0] ] += c * dist[5] / total;
            density[ g6[2] * 512 * 512 + g6[1] * 512 + g6[0] ] += c * dist[6] / total;
            density[ g7[2] * 512 * 512 + g7[1] * 512 + g7[0] ] += c * dist[7] / total;
        }
    }
    std::cerr << "percentage of voronoi cell without a grid point: " 
              << 100.0f * emptyCellCount / nPtcToUse << std::endl;
    
    // Output the density field
    gettimeofday( &start, NULL );
    FILE* f = fopen( argv[2], "w" );
    size_t rt = fwrite( density, sizeof(float), totalGridPts, f );
    fclose( f );
    gettimeofday( &end, NULL );
    std::cerr << "writing density fields takes " << GetElapsedSeconds(&start, &end) 
              << " seconds." << std::endl;
    
    delete[] density;
    //delete[] contribution;
    delete[] pcounter;
    delete[] counter;
    delete[] ptcBuf;
}

