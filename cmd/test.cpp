#include "command.h"
#include "debug.h"
#include "image.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test ";

  ARGUMENTS
  + Argument ("bogus", "s").type_float ();
}

template<typename T>
struct has_method
{
private:
  typedef std::true_type yes;
  typedef std::false_type no;

  template<typename U> static auto test(bool) -> decltype(std::declval<U>().robust_estimate() == 1, yes());

  template<typename> static no test(...);

public:

  static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
};


void run ()
{
  class x
  {
    public:
      bool robust_estimate() const { return true; }
  };

  class y : x {};

  class z
  {
    public:
      bool non_robust_estimate() const { return false; }
  };

  Registration::Transform::Affine A;
  Registration::Transform::Rigid R;

  std::cout << has_method<x>::value << ", " << has_method<y>::value << ", " << has_method<z>::value << std::endl; // 1, 0, 0
  // std::cout << has_method<A>::value << std::endl; // error: the value of 'A' is not usable in a constant expression
                                                     // note: 'A' was not declared 'constexpr'
}
