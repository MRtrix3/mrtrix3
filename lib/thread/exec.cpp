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

