/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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



