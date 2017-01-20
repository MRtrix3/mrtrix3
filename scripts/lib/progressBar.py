import os, sys
import lib.app
import lib.message


class progressBar:

  def _update(self):
    sys.stderr.write('\r' + lib.message.colourConsole + os.path.basename(sys.argv[0]) + ': ' + lib.message.colourClear + '[{0:>3}%] '.format(int(round(100.0*self.counter/self.target))) + self.message + '...' + lib.message.clearLine + self.newline)
    sys.stderr.flush()

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
    self.counter = self.target
    sys.stderr.write('\r' + lib.message.colourConsole + os.path.basename(sys.argv[0]) + ': ' + lib.message.colourClear + '[100%] ' + self.message + lib.message.clearLine + '\n')
    sys.stderr.flush()
    lib.app.verbosity = self.orig_verbosity
    
