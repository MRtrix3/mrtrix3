#include <fstream>
#include <iostream>
#include <vector>
#include "command.h"
#include "image.h"
#include "header.h"
using namespace std;
using namespace MR;
using namespace App;

void usage() {
	AUTHOR = "Thea Bautista";

	SYNOPSIS = "Removal of Gibbs Ringing in 3D";

	DESCRIPTION
	+ "This reads an input nifti file and outputs an image after running fft function.";

	ARGUMENTS
	+ Argument ("inImg", "input image to be read").type_image_in()
	+ Argument ("outImg", "outuput image").type_image_out();

}

void run() {

	// opening image which is specified by first argument
	auto input = Image<float>::open (argument[0]);
	// argument[0] retrieves values as a string
	// The auto keyword specifies that the type of the variable that is being declared will be automatically deducted from its initializer


	Header header (input); // creating an instance of Header as it is a required argument when creating output image
	header.datatype() = DataType::from_command_line (DataType::Float64);
	// defining data type of header
	auto output = Image<float>::create (argument[1], header);
	//MR::Image::create creates output file using second argument and Header


	// insert code here to use fft

       threaded_copy_with_progress_message ("copying from input to output", input, output);



}
