#include "command.h"
#include "image.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "me";

  DESCRIPTION
  + "Evaluate the amplitude of an image of spherical harmonic functions "
    "along the specified directions";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}



template <class T> size_t actual_alignof () {
  size_t alignment = 10000;
  for (size_t n = 0; n < 100; ++n) { 
    auto* x = new T; 
    size_t this_align = reinterpret_cast<size_t>(x) & 127U;
    if (this_align && this_align < alignment)
      alignment = this_align;
    auto ununsed = new int; // just to jitter alignment around a bit
  } 
  if (alignment < alignof(T))
    WARN ("memory alignment failure!");
  return alignment;
}


// test classes:
// just try commenting out the MEMALIGN statement to check:
// - whether compiler catches the issue if it exists 
//   (i.e. alignment required > default, no operator new defined)
// - whether the class has an operator new method
// - what the stated compile-time alignment of the class is
// - whether the runtime alignment of the class is as expected

class AlignedMember { MEMALIGN (AlignedMember)
  public:
    Eigen::Matrix4d M;
};

template <class T> 
class AlignedMemberTemplate { MEMALIGN (AlignedMemberTemplate<T>)
  public:
    Eigen::Matrix4d M;
    T t;
};

template <class T>
class InheritsAlignedClass : public AlignedMember { MEMALIGN(InheritsAlignedClass<T>)
  public:
    double x;
};

// looks like operator new is inherited by derived classes:
template <class T>
class InheritsAlignedClassNoOperatorNew : public AlignedMember { 
  public:
    double x;
};

// but this does not apply if class is included as a regular member:
template <class T>
class IncludesAlignedClassNoOperatorNew { MEMALIGN(IncludesAlignedClassNoOperatorNew<T>)
  public:
    AlignedMember a;
    double x;
};


#define RUNCHECKS(classname) \
  std::cerr << "\n######### " #classname " ##################\n"; \
  CHECK_MEM_ALIGN (classname); \
  VAR (alignof(classname)); \
  VAR (actual_alignof<classname>()); \
  VAR (__has_custom_new_operator<classname>::value)

void run ()
{
  RUNCHECKS (int);
  RUNCHECKS (Eigen::Vector2f);
  RUNCHECKS (Eigen::Vector3f);
  RUNCHECKS (Eigen::Vector4f);
  RUNCHECKS (Eigen::Vector2d);
  RUNCHECKS (Eigen::Vector3d);
  RUNCHECKS (Eigen::Vector4d);
  RUNCHECKS (Eigen::Matrix2f);
  RUNCHECKS (Eigen::Matrix3f);
  RUNCHECKS (Eigen::Matrix4f);
  RUNCHECKS (Eigen::Matrix2d);
  RUNCHECKS (Eigen::Matrix3d);
  RUNCHECKS (Eigen::Matrix4d);

  RUNCHECKS (AlignedMember);
  RUNCHECKS (AlignedMemberTemplate<float>);
  RUNCHECKS (InheritsAlignedClass<float>);
  RUNCHECKS (InheritsAlignedClassNoOperatorNew<float>);
  RUNCHECKS (IncludesAlignedClassNoOperatorNew<float>);

  RUNCHECKS (Header);
  RUNCHECKS (Image<float>);
  RUNCHECKS (Image<float>::Buffer);
}
