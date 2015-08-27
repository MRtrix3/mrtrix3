#include "command.h"
#include "debug.h"
#include "image.h"
#include "image/average_space.h"

namespace MR {
}


using namespace MR;
using namespace App;

typedef double ComputeType;
typedef Eigen::VectorXd VectorType;
typedef Eigen::RowVectorXd RowVectorType;
ComputeType PADDING_DEFAULT     = 0.0;
ComputeType TEMPLATE_RESOLUTION = 0.9;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test average space calculation";

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple();
  + Argument ("output", "the output image").type_image_out ();

  OPTIONS
  + Option ("padding", " template boundary box padding in voxels. Default: " + str(PADDING_DEFAULT))
  +   Argument ("value").type_float (0.0, PADDING_DEFAULT, std::numeric_limits<ComputeType>::infinity())
  + Option ("template_res", " subsampling of template compared to smallest voxel size in any input image. Default: " + str(TEMPLATE_RESOLUTION))
  +   Argument ("value").type_float (ComputeType(0.0), TEMPLATE_RESOLUTION, ComputeType(1.0))
  + DataType::options();
}


void run ()
{

	const size_t num_inputs = argument.size()-1;
	// typedef Eigen::Transform< ComputeType, 3, Eigen::Projective> TransformType;



	auto opt = get_options ("padding");
	const ComputeType p = opt.size() ? ComputeType(opt[0][0]) : PADDING_DEFAULT;
	// VectorType padding = RowVectorType(4);
	auto padding = Eigen::Matrix<ComputeType, 4, 1>(p, p, p, 1.0);
	INFO("padding in template voxels: " + str(padding.transpose().head<3>()));

	opt = get_options ("template_res");
	const ComputeType template_res = opt.size() ? ComputeType(opt[0][0]) : TEMPLATE_RESOLUTION;
	INFO("template voxel subsampling: " + str(template_res));

	std::vector<Header> headers_in;
	
	for (size_t i = 0; i != num_inputs; ++i) {
		headers_in.push_back (Header::open (argument[i]));
		const Header& temp (headers_in[i]);
		if (temp.ndim() < 3)
			throw Exception ("Please provide 3 dimensional images");
	}


	for(auto temp: headers_in){
		std::cerr << temp << std::endl;
		auto trafo = (Eigen::Transform<ComputeType, 3, Eigen::Projective>) Transform(temp).voxel2scanner;
		std::cerr << trafo.matrix() << std::endl;
		
    	auto bbox = get_bounding_box<ComputeType,decltype(trafo)>(temp, trafo);
    	std::cerr << bbox << std::endl;
	}

	auto trafo = headers_in[0].transform();
	std::vector<decltype(trafo)> transform_header_with;

	auto H = compute_minimum_average_header<double,decltype(trafo)>(headers_in, template_res, padding, transform_header_with);

	// std::cerr << H << std::endl;
	std::cerr << "template header trafo:\n" << H.transform().matrix() << std::endl;

	auto out = Header::create(argument[argument.size()-1],H);
	// auto out = Image<float>::create(argument[argument.size()-1],H);
	// for (size_t i = 0; i<num_inputs; ++i){	
	// 	auto in = headers_in[i].get_image<float>();
	// 	for (auto j = Loop() (in, out); j ;++j){
	// 		out.value() += in.value()/num_inputs;
	// 	}
	// }
}
