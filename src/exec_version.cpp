#include "exec_version.h"


namespace MR {
  namespace App {
    extern const char* executable_uses_mrtrix_version;
    void set_executable_uses_mrtrix_version () { executable_uses_mrtrix_version = "3.0.4"; }
  }
}