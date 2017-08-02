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

       ./build select debug
       ./configure -debug -assert
       ./build bin/command

   Note that this process will move your existing *MRtrix3*
   compilation into a temporary directory. This means that your
   compiled binaries will no longer be in your PATH; but it also
   means that later we can restore them quickly without re-compiling
   all of *MRtrix3*. In addition, we only compile the command that we
   need to test (replace "``command``" with the name of the command
   you are testing).

4. Execute the problematic command within ``gdb``:

   ::
   
       gdb --args bin/command (arguments) (-options) -debug

   The preceding ``gdb --args`` at the beginning of the
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

       valgrind bin/command (arguments) (-options)
      
10. When you have finished debugging, restore your default *MRtrix3*
    compilation:

    ::
    
       ./build select default

    Binaries compiled in debug mode run considerably slower than those
    compiled using the default settings (even if not running within ``gdb``),
    due to the inclusion of various symbols that assist in debugging and the
    removal of various optimisations. Therefore it's best to restore the
    default configuration for your ongoing use.

We greatly appreciate any contribution that the community can make
toward making *MRtrix3* as robust as possible, so please don't hesitate to
report any issues you encounter.

