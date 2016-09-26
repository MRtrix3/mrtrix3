Advanced debugging
===================

On rare occasions, a user may encounter a critical error (e.g.
"Segmentation fault") within an *MRtrix3* command that does not give
sufficient information to identify the cause of the problem, and that
the developers are unable to reproduce. In these cases, we will often
ask to be provided with example data that can consistently reproduce the
problem in order to localise the issue. An alternative is for the user
to perform an initial debugging experiment, and provide us with the
resulting information. The instructions for doing so are below.

1. If required, install ``gdb``; the
   `GNU Debugging Tool <https://www.gnu.org/software/gdb/>`_
   (specific instructions for this installation will depend on your
   operating system)

2. *Make sure* you are using the most up-to-date *MRtrix3* code!
   (``git pull``)

3. Configure and compile *MRtrix3* in debug mode:

   ::
   
       ./configure -debug -assert debug; ./build debug
       
   Note that this compilation will reside *alongside* your existing *MRtrix3*
   installation, but will not interfere with it in any way. Commands
   that are compiled in debug mode will reside in the ``debug/bin``
   directory.

4. Execute the problematic command within ``gdb``:

   ::
   
       gdb --args debug/bin/command (arguments) (-options) -debug
       
   Note that the version of the command that is compiled in debug mode
   resides in the ``debug/bin`` directory; you must provide this full
   path *explicitly* to ensure that this is the version of the command that
   is executed. The preceding ``gdb --args`` at the beginning of the
   line is simply the easiest way to execute the command within ``gdb``.
   Include all of the file paths, options etc. that you used previously
   when the problem occurred. It is also recommended to use the *MRtrix3*
   ``-debug`` option so that *MRtrix3* produces more verbose information
   at the command-line.

5. If running on Windows, once ``gdb`` has loaded, type the following into
   the terminal:
   
   ::
   
       b abort
       b exit

   These 'breakpoints' must be set explicitly in order to prevent the command
   from being terminated completely on an error, which would otherwise
   preclude debugging once an error is actually encountered.

6. At the ``gdb`` terminal, type ``r`` and hit ENTER to run the command.

7. If an error is encountered, ``gdb`` will print an error, and then provide
   a terminal with ``(gdb)`` shown on the left hand side. Type ``bt``
   and hit ENTER: This stands for 'backtrace', and will print details on
   the internal code that was running when the problem occurred.

8. Copy all of the raw text, from the command you ran in instruction 3
   all the way down to the bottom of the backtrace details, and send it
   to us. The best place for these kind of reports is to make a new
   issue in the `Issues <https://github.com/MRtrix3/mrtrix3/issues>`__
   tracker for the GitHub repository.

9. If ``gdb`` does not report any error, it is possible that a memory error
   is occurring, but even the debug version of the software is not performing
   the necessary checks to detect it. If this is the case, you can also try
   using `Valgrind <http://valgrind.org/>`_, which will perform a more
   exhaustive check for memory faults (and correspondingly, the command will
   run exceptionally slowly):
   
   ::
   
       valgrind debug/bin/command (arguments) (-options)

We greatly appreciate any contribution that the community can make
toward making *MRtrix3* as robust as possible, so please don't hesitate to
report any issues you encounter.

