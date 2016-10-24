import os, sys
import lib.app


class progressBar:

  def _update(self):
    sys.stderr.write('\r' + lib.app.colourPrint + os.path.basename(sys.argv[0]) + ': ' + lib.app.colourClear + '[{0:>3}%] '.format(int(round(100.0*self.counter/self.target))) + self.message + '...' + lib.app.clearLine + self.newline)

  def __init__(self, msg, target):
    self.counter = 0
    self.message = msg
    self.newline = '\n' if lib.app.verbosity > 1 else '' # If any more than default verbosity, may still get details printed in between progress updates
    self.orig_verbosity = lib.app.verbosity
    self.target = target
    lib.app.verbosity = lib.app.verbosity - 1 if lib.app.verbosity else 0
    self._update()

  def increment(self, msg=''):
    if msg:
      self.message = msg
    self.counter += 1
    self._update()

  def done(self):
    self.counter = target
    sys.stderr.write('\r' + lib.app.colourPrint + os.path.basename(sys.argv[0]) + ': ' + lib.app.colourClear + '[100%] ' + self.message + lib.app.clearLine + '\n')
    lib.app.verbosity = self.orig_verbosity
    