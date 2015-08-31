#include "command.h"
#include "debug.h"
#include "image.h"
#include "filter/smooth.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test save";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ();
}


void run ()
{	
	auto template_image = Image<float>::open (argument[0]);


	Filter::Smooth template_smooth_filter (template_image);
	auto template_smoothed = Image<float>::scratch (template_smooth_filter);
	template_smooth_filter (template_image, template_smoothed);

	save(template_image, "template.mif");
	save(template_smoothed, "template_smoothed.mif"); // mrview segfaults
}