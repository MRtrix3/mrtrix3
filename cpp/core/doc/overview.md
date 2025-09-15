Overview          {#mainpage}
========

MRtrix was developed with simplicity, performance, flexibility and consistency
in mind, which has led to a number of fundamental design decisions. The main
concepts are explained in the following pages:

- The build process is based on a Python script rather than Makefiles, and all
  dependencies are resolved at build-time. This is explained in @ref build_page.

- The configure script allows you to create different co-existing
  configurations, for example to easily switch from a release build to a debug
  build.  This is explained in @ref configure_page.

- You are encouraged to set up your own, separate module on top of the MRtrix
  codebase. This allows you to write your own code, stored on your own
  (potentially private) repository, without affecting the MRtrix core repository.
  This is explained in the section @ref module_howto.

- The basic steps for writing applications based on MRtrix are explained in the
  section @ref command_howto.

- The concepts behind accessing and processing %Image data are outlined in @ref
  image_access.

- There are a number of convenience classes to simplify development of
  multi-threaded applications, in particular the Image::ThreadedLoop and
  Thread::Queue classes. These are outlined in the section @ref multithreading.


