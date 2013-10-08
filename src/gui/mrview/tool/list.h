# ifndef TOOL

// place #include files in here:
#include "gui/mrview/tool/view.h"
#include "gui/mrview/tool/lighting.h"
//#include "gui/mrview/tool/roi_analysis.h"
#include "gui/mrview/tool/overlay.h"
#include "gui/mrview/tool/odf.h"
#include "gui/mrview/tool/screen_capture.h"
#include "gui/mrview/tool/tractography/tractography.h"

#else

/* Provide a corresponding line for each mode here:
The first argument is the name of the corresponding Mode class.
The second argument is the name of the mode as displayed in the menu.
The third argument is the text to be shown in the menu tooltip. */

TOOL(View, View options, Adjust view settings)
TOOL(Lighting, Lighting options, Adjust lighting settings for those modes that support it)
//TOOL(ROI, ROI analysis, Draw & analyse regions of interest)
TOOL(Overlay, Overlay, Overlay other images over the current image)
TOOL(Tractography, Tractography, Display tracks over the current image)
TOOL(ODF, ODF Display, Display orientation density functions)
TOOL(ScreenCapture, Screen capture, Capture the screen as a png file)

#endif



