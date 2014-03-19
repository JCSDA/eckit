#include <limits>
#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <memory>

#include "atlas/Gmsh.hpp"
#include "atlas/Mesh.hpp"

#include "GribLoad.h"
#include "PointIndex3.h"
#include "PointSet.h"
#include "TriangleIntersection.h"
#include "Tesselation.h"

//------------------------------------------------------------------------------------------------------

#if 1
#define DBG     std::cout << Here() << std::endl;
#define DBGX(x) std::cout << #x << " -> " << x << std::endl;
#else
#define DBG
#define DBGX(x)
#endif

//------------------------------------------------------------------------------------------------------

using namespace Eigen;
using namespace atlas;
using namespace eckit;

//------------------------------------------------------------------------------------------------------

void mesh_renumber_triags( Mesh& mesh )
{
    FunctionSpace& nodes     = mesh.function_space( "nodes" );
    FieldT<double>& coords   = nodes.field<double>( "coordinates" );

    const size_t nb_nodes = nodes.bounds()[1];

    FunctionSpace& triags      = mesh.function_space( "triags" );
    FieldT<int>& triag_nodes   = triags.field<int>( "nodes" );

    const size_t nb_triags = triags.bounds()[1];

    for( int e = 0; e < nb_triags; ++e )
    {
        for( int n=0; n<3; ++n )
        {
            size_t pidx = triag_nodes(n,e);
//            triag_nodes(n,e)
        }
    }

}

//------------------------------------------------------------------------------------------------------

#define NLATS 100
#define NLONG 100

#define COMPUTE_WEIGHTS
#define FILL_MATRIX

int main()
{    
    typedef std::numeric_limits< double > dbl;
    std::cout.precision(dbl::digits10);
    std::cout << std::fixed;

    // input grid

    std::cout << "> reading input points ..." << std::endl;

    std::unique_ptr< std::vector< KPoint3 > > readpts( read_ll_points_from_grib( "data.grib" ) );

    // generate mesh ...

    std::cout << "> generating tesselation ..." << std::endl;

    std::unique_ptr<Mesh> mesh( Tesselation::generate_from_points( *readpts ) );

    // read input field

    std::cout << "> reading input field ..." << std::endl;

    atlas::FieldT<double>& infield = read_field_into_mesh_from_grib( "data.grib", *mesh );

    // output input mesh

    std::cout << "> output input mesh ..." << std::endl;

    atlas::Gmsh::write3dsurf(*mesh, std::string("in.msh") );


    // output grid

    std::unique_ptr< std::vector< KPoint3 > > opts( Tesselation::generate_latlon_points(NLATS, NLONG) );

    atlas::Mesh* outMesh = Tesselation::generate_from_points(*opts);
    atlas::Gmsh::write3dsurf(*outMesh, std::string("out.msh") );
    delete outMesh;

#ifdef COMPUTE_WEIGHTS

    // generate baricenters of each triangle & insert the baricenters on a kd-tree

    std::cout << "> creating triangle index ..." << std::endl;

    atlas::MeshGen::create_cell_centres( *mesh );

    std::unique_ptr<PointIndex3> tree( create_point_index<PointIndex3>( *mesh ) );

    // compute weights for each point in output grid

    std::cout << "> computing weights ..." << std::endl;

    FunctionSpace& nodes     = mesh->function_space( "nodes" );
    FieldT<double>& coords   = nodes.field<double>( "coordinates" );

    const size_t inp_npts = nodes.bounds()[1];

    FunctionSpace& triags      = mesh->function_space( "triags" );
    FieldT<int>& triag_nodes   = triags.field<int>( "nodes" );

    const size_t nb_triags = triags.bounds()[1];

    //=======================================================================
    //==== POINT LOOP =======================================================
    //=======================================================================

    Eigen::SparseMatrix<double> W( opts->size(), inp_npts );

    std::vector< Eigen::Triplet<double> > weights_triplets; /* structure to fill-in sparse matrix */

    weights_triplets.reserve( opts->size() * 3 ); /* each row has 3 entries: one per vertice of triangle */

    const size_t k = 256; /* search nearest k cell centres */

    for( size_t ip = 0; ip < opts->size(); ++ip )
    {
        std::ostringstream os;

        KPoint3 p( (*opts)[ip] ); // lookup point

#if 1
        std::cout << p << std::endl;
#endif

        // search the nearest 4 triangles

        PointIndex3::NodeList cs = tree->kNearestNeighbours(p,k);

#if 0
        for( size_t i = 0; i < cs.size(); ++i )
        {
            std::cout << cs[i] << std::endl;
        }
#endif

        // find in which triangle the point is contained
        // by computing the intercetion of the point with each nearest triangle

        int idx[3]; /* indexes of the triangle that will contain the point*/

        Vector3d phi;
        Isect uvt;
        Ray ray( p.data() );

        size_t tid = std::numeric_limits<size_t>::max();

        bool found = false;
        for( size_t i = 0; i < cs.size(); ++i )
        {
            tid = cs[i].value().payload();

            KPoint3 tc = cs[i].value().point();

            ASSERT( tid < nb_triags );

            idx[0] = triag_nodes(0,tid);
            idx[1] = triag_nodes(1,tid);
            idx[2] = triag_nodes(2,tid);

            ASSERT( idx[0] < inp_npts && idx[1] < inp_npts && idx[2] < inp_npts );

            Triag triag( coords.slice(idx[0]), coords.slice(idx[1]), coords.slice(idx[2]) );

            if( triag_intersection( triag, ray, uvt ) )
            {
                found = true;
#if 0
                os << "[SUCCESS]" << std::endl
                   << "   p    " << p << std::endl
                   << "   tc   " << tc << std::endl
                   << "   d    " << KPoint3::distance(tc,p) << std::endl
                   << "   tid  " << tid << std::endl
                   << "   nidx " << idx[0] << " " << idx[1] << " " << idx[2] << std::endl
                   << "   " << KPoint3(coords.slice(idx[0])) << " / "
                            << KPoint3(coords.slice(idx[1])) << " / "
                            << KPoint3(coords.slice(idx[2])) << std::endl
                   << "   uvwt " << uvt << std::endl;
#endif
                // weights are the baricentric cooridnates u,v

                phi[0] = uvt.w();
                phi[1] = uvt.u;
                phi[2] = uvt.v;

                break;
            }
            else
            {
                os << "[FAILED] projection on triangle:" << std::endl
                   << "   p    " << p << std::endl
                   << "   tc   " << tc << std::endl
                   << "   d    " << KPoint3::distance(tc,p) << std::endl
                   << "   tid  " << tid << std::endl
                   << "   nidx " << idx[0] << " " << idx[1] << " " << idx[2] << std::endl
                   << "   " << KPoint3(coords.slice(idx[0])) << " / "
                            << KPoint3(coords.slice(idx[1])) << " / "
                            << KPoint3(coords.slice(idx[2])) << std::endl
                   << "   uvwt " << uvt << std::endl;
            }
        }

#if 0
        if( found ) { std::cout << os.str() << std::endl; }
#endif

        if( !found )
        {
            std::cout << os.str() << std::endl;
            throw SeriousBug("Could not project point into nearest triangles");
        }

        // insert the interpolant weights into the global (sparse) interpolant matrix

        for(int i = 0; i < 3; ++i)
            weights_triplets.push_back( Eigen::Triplet<double>( ip, idx[i], phi[i] ) );

    }
#endif // COMPUTE_WEIGHTS



#ifdef FILL_MATRIX
    // fill-in sparse matrix

    std::cout << "> filling matrix ..." << std::endl;

    W.setFromTriplets(weights_triplets.begin(), weights_triplets.end());

    // interpolation -- multiply interpolant matrix with field vector

    std::cout << "> interpolating ..." << std::endl;

    std::vector<double> result ( opts->size() ); /* result vector */

    VectorXd f = VectorXd::Map( &(infield.data())[0], infield.data().size() );
    VectorXd r = VectorXd::Map( &result[0], result.size() );

    r = W * f;

    {
        atlas::Mesh* outMesh = Tesselation::generate_from_points(*opts);

        FunctionSpace& nodes     = outMesh->function_space( "nodes" );
        FieldT<double>& coords   = nodes.field<double>( "coordinates" );

        const size_t nb_nodes = nodes.bounds()[1];

        FieldT<double>& field = nodes.create_field<double>("field",1);

        for( size_t i = 0; i < opts->size(); ++i )
            field[i] = r[i];

        atlas::Gmsh::write3dsurf(*outMesh, std::string("out.msh") );

        delete outMesh;
    }
#endif // FILL_MATRIX

    return 0;
}
