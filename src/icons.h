#ifndef __icons_h__
#define __icons_h__

namespace MR {

  class Icon {
    public:
      Icon (const unsigned char* d, int w, int h) : 
            data (d), width (w), height (h) { }
      const unsigned char* data;
      int width, height;

    static const Icon pan_crosshair;
    static const Icon forward_backward;
    static const Icon mrtrix;
    static const Icon pan;
    static const Icon window;
    static const Icon crosshair;
    static const Icon zoom;
  };
}

#endif
