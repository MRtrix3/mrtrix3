#include "command.h"
#include "debug.h"
#include "image.h"
#include "image/average_space.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test average space calculation";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple();
}


void run ()
{
	const size_t num_inputs = argument.size();
	typedef double ComputeType;
	// typedef Eigen::Transform< ComputeType, 3, Eigen::Projective> TransformType;
	double voxel_subsampling = 1.0;
	Eigen::Matrix<ComputeType, 4, 1>  padding(1,1,1,0);

	std::vector<Header> headers_in;
	
	for (size_t i = 0; i != num_inputs; ++i) {
		headers_in.push_back (Header::open (argument[i]));
		const Header& temp (headers_in[i]);
		if (temp.ndim() < 3)
			throw Exception ("Please provide 3 dimensional images");
	}


	for(auto temp: headers_in){
		std::cerr << temp << std::endl;
		auto trafo = temp.transform();
		std::cerr << trafo.matrix() << std::endl;
		
    	auto bbox = get_bounding_box<ComputeType,decltype(trafo)>(temp, trafo);
    	std::cerr << bbox << std::endl;
	}

	auto trafo = headers_in[0].transform();
	std::vector<decltype(trafo)> transform_header_with;

	auto H = compute_minimum_average_header<double,decltype(trafo)>(headers_in, voxel_subsampling, padding, transform_header_with);
}
