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
struct has_robust_estimator
{
private:
  typedef std::true_type yes;
  typedef std::false_type no;
  template<typename U> static auto test(bool) -> decltype(std::declval<U>().robust_estimate() == 1, yes());
  template<typename> static no test(...);
public:
  static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
};

template <class TransformType_>
  typename std::enable_if<has_robust_estimator<TransformType_>::value, void>::type
  evaluate (TransformType_&& trafo, bool dummy = true)
  {
    trafo.robust_estimate();
    VAR(dummy);
  }

template <class TransformType_>
  typename std::enable_if<!has_robust_estimator<TransformType_>::value, void>::type
  evaluate (TransformType_&& trafo, bool dummy = false)
  {
    VAR(dummy);
  }


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

  std::cout << has_robust_estimator<x>::value << ", " << has_robust_estimator<y>::value << ", " << has_robust_estimator<z>::value << std::endl; // 1, 0, 0
  std::cout << has_robust_estimator<decltype(A)>::value << ", " << has_robust_estimator<decltype(R)>::value << std::endl;
  evaluate(A);
  evaluate(R);

}
