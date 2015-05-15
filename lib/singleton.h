#ifndef __singleton_h__
#define __singleton_h__


namespace MR
{


template < class T > 
class Singleton
{

  public:

    static T& getInstance() 
    {
    
      static T instance;
      return instance;

    }

};


}


#endif

