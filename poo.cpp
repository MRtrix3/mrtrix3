#include "file/mmap.h"
#include "file/gz.h"
#include "exception.h"

using namespace MR;

void cmdline_print (const std::string& msg) { std::cout << msg; }
void cmdline_error (const std::string& msg) { std::cerr << "ERROR: " << msg << "\n"; }
void cmdline_info  (const std::string& msg) { std::cerr << "INFO: " <<  msg << "\n"; }
void cmdline_debug (const std::string& msg) { std::cerr << "DEBUG: " <<  msg << "\n"; }

int main (int argc, char* argv[])
{
  MR::print = cmdline_print;
  MR::error = cmdline_error;
  MR::info = cmdline_info;
  MR::debug = cmdline_debug;

  assert (argc == 2);
  File::Entry entry (argv[1]);

  std::cout << entry << "\n";
  File::MMap mmap (entry, 737);

  std::cout << mmap << "\n";

  std::cout << "****************\n";
  std::cout.write ((const char*) mmap(), mmap.size());
  std::cout << "*****************\n";

  return (0);
}
