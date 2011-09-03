#ifndef __gui_icons_h__
#define __gui_icons_h__

namespace MR
{
  namespace GUI
  {

    class Icon
    {
      public:
        Icon (const unsigned char* d, int w, int h) :
          data (d), width (w), height (h) { }
        const unsigned char* data;
        int width, height;

        static const Icon pan_crosshair;
        static const Icon inplane_rotate;
        static const Icon forward_backward;
        static const Icon mrtrix;
        static const Icon throughplane_rotate;
        static const Icon pan;
        static const Icon window;
        static const Icon crosshair;
        static const Icon zoom;
    };

  }
}

#endif
