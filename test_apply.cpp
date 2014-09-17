#include <iostream>
#include <string>
#include "lib/apply.h"

struct print 
{
  template <typename T>
    void operator() (const T& t) const { std::cout << t << std::endl; }
};


struct print_all 
{
  template <typename A, typename B, typename C>
    void operator() (const A& a, const B& b, const C& c) const {
      std::cout << a << " " << b << " " << c << std::endl;
    }
};

struct add3 {
  template <typename T>
    T operator() (const T& a, const T& b, const T& c) const {
      return a+b+c;
    }
};


using namespace MR;

int main (int argc, char* argv[]) 
{
  
  float f = 3.212;
  int i = 5;
  std::string s = "text";


  auto t = std::make_tuple (f,i,s);
  print p;

  apply (print(), std::make_tuple (f, i, s));
  apply (p, std::make_tuple (f, i, s));
  apply (print(), t);
  apply (p, t);

  unpack (print_all(), std::make_tuple (f, i, s));
  unpack (print_all(), t);

  std::cout << unpack (add3(), std::make_tuple (1.2, 4.2, 8.5)) << std::endl;
  std::cout << unpack ([](float a, float b, float c) { return a*b*c; }, std::make_tuple (1.2, 4.2, 8.5)) << std::endl;
}

