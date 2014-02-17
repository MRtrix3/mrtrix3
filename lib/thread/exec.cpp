#include "app.h"
#include "thread/exec.h"
#include "thread/mutex.h"
#include "file/config.h"

namespace MR
{
  namespace Thread
  {

    namespace {

      size_t __number_of_threads = 0;
      
    }

    size_t number_of_threads ()
    {
      if (__number_of_threads)
        return __number_of_threads;
      const App::Options opt = App::get_options ("nthreads");
      __number_of_threads = opt.size() ? opt[0][0] : File::Config::get_int ("NumberOfThreads", 1);
      return __number_of_threads;
    }




    void (*Exec::Common::previous_print_func) (const std::string& msg) = NULL;
    void (*Exec::Common::previous_report_to_user_func) (const std::string& msg, int type) = NULL;

    Exec::Common::Common () :
      refcount (0), attributes (new pthread_attr_t) {
        DEBUG ("initialising threads...");
        pthread_attr_init (attributes);
        pthread_attr_setdetachstate (attributes, PTHREAD_CREATE_JOINABLE);

        previous_print_func = print;
        previous_report_to_user_func = report_to_user_func;

        print = thread_print_func;
        report_to_user_func = thread_report_to_user_func;
      }

    Exec::Common::~Common () 
    {
      print = previous_print_func;
      report_to_user_func = previous_report_to_user_func;

      DEBUG ("uninitialising threads...");

      pthread_attr_destroy (attributes);
      delete attributes;
    }

    void Exec::Common::thread_print_func (const std::string& msg)
    {
      Thread::Mutex::Lock lock (common->mutex);
      previous_print_func (msg);
    }

    void Exec::Common::thread_report_to_user_func (const std::string& msg, int type)
    {
      Thread::Mutex::Lock lock (common->mutex);
      previous_report_to_user_func (msg, type);
    }


    Exec::Common* Exec::common = NULL;




  }
}

