#include <fstream>
#include <iostream>
#include <vector>
#include "command.h"
#include "image.h"
#include "header.h"
#include "filter/fft.h"
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


	// insert code here to use fft
	auto input = Image<cdouble>::open(argument[0]);
	Filter::FFT fft (input,false);
	auto output = Image<cdouble>::create (argument[1], fft);
	fft (input, output);



}
