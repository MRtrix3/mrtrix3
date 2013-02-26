'''
A class that creates a text-based progress bar instance.
example progress bar object:
pb_ex = ProgressMeter(total=6000,unit='operations',ticks=25)
examples progress bar format:
[=========|---------------] 17%
[=========================] 100%

Licensed under the PSF License
source: http://code.activestate.com/recipes/473899-progress-meter/
credits:
2006-02-16 vishnubob: original code
2006-02-20 Denis Barmenkov: ANSI codes replaced by Backspace (0x08) characters
2010-05-17 Alexis Lopez: total time taken and time-to-completion estimation
------------------------------------------------------------------------------
Usage: from progressmeter import ProgressMeter
'''

import sys
import time

class ProgressMeter(object):
    def __init__(self, *args, **kw):
        '''Initiates the progress bar object.'''
        # What time do we start tracking our progress from?
        self.timestamp = kw.get('timestamp', None)
        # What kind of unit are we tracking?
        self.unit = str(kw.get('unit', ''))
        # Number of units to process
        self.total = int(kw.get('total', 100))
        # Number of units already processed
        self.count = int(kw.get('count', 0))
        # Refresh rate in seconds
        self.rate_refresh = float(kw.get('rate_refresh', .5))
        # Number of ticks in meter
        self.meter_ticks = int(kw.get('ticks', 60))
        self.meter_division = float(self.total) / self.meter_ticks
        self.meter_value = int(self.count / self.meter_division)
        self.last_update = None
        self.rate_history_idx = 0
        self.rate_history_len = 10
        self.rate_history = [None] * self.rate_history_len
        self.rate_current = 0.0
        self.last_refresh = 0
        self.prev_meter_len = 0
        self.switch_off = False
        self.estimated_duration = []

    def update(self, count, *args, **kw):
        '''Updates the progress bar internal counter by adding the value of the
           "count" variable to the internal counter.'''
        if not self.timestamp: self.timestamp = time.time()
        now = time.time()
        # Calculate rate of progress
        rate = 0.0
        # Add count to Total
        self.count += count
        self.count = min(self.count, self.total)
        if self.last_update:
            delta = now - float(self.last_update)
            if delta:
                rate = count / delta
            else:
                rate = count
            self.rate_history[self.rate_history_idx] = rate
            self.rate_history_idx += 1
            self.rate_history_idx %= self.rate_history_len
            cnt = 0
            total = 0.0
            # Average rate history
            for rate in self.rate_history:
                if rate == None:
                    continue
                cnt += 1
                total += rate
            rate = total / cnt
            self.estimated_duration.append((self.total - self.count) / rate)
        self.rate_current = rate
        self.last_update = now
        # Device Total by meter division
        value = int(self.count / self.meter_division)
        if value > self.meter_value:
            self.meter_value = value
        if self.last_refresh:
            if (now - self.last_refresh) > self.rate_refresh or (self.count >= self.total):
                    self._refresh()
        else:
            self._refresh()

    def set(self, percentage, *args, **kw):
        '''Sets the progress bar internal counter to a value such that the
           percentage progress is close to the specified value of the
           "percentage" variable.'''
        if percentage < 100:
            count = int((float(percentage) / 100) * self.total)
            count = count - self.count
        else:
            self.count = self.total
            count = 0
        self.update(count)

    def start(self, *args, **kw):
        '''Sets the progress bar timestamp.
           If this method is not called then ProgressMeter.update() or
           ProgressMeter.set() automatically sets the progress bar timestamp on
           its first usage. Does NOT override user specified __init__ settings
           same as with ProgressMeter.update() or ProgressMeter.set()'''
        if not self.timestamp: self.timestamp = time.time()

    def reset(self, *args, **kw):
        '''Resets the progress bar to the initial settings with self.count
           set to 0'''
        # What time do we start tracking our progress from?
        timestamp = kw.get('timestamp', None)
        # What kind of unit are we tracking?
        unit = str(kw.get('unit', self.unit))
        # Number of units to process
        total = int(kw.get('total', self.total))
        # Number of units already processed
        count = int(kw.get('count', 0))
        # Refresh rate in seconds
        rate_refresh = float(kw.get('rate_refresh', self.rate_refresh))
        # Number of ticks in meter
        meter_ticks = int(kw.get('ticks', self.meter_ticks))
        self.__init__(timestamp=timestamp, unit=unit, total=total, count=count, rate_refres=rate_refresh,
                  ticks=meter_ticks)

    def _get_meter(self, *args, **kw):
        perc = (float(self.count) / self.total) * 100
        bar = '=' * self.meter_value
        if self.count >= self.total:
            pad = '=' * (self.meter_ticks - self.meter_value)
            # Time taken to completion
            dur = time.time() - self.timestamp
            # Converting into hours, minutes and seconds
            hours, remainder = divmod(dur,3600)
            minutes, seconds = divmod(remainder,60)
            if dur < 60:
                est = ' completed in %.f sec' % seconds
            elif dur >= 60 and dur < 3600:
                est = ' completed in %.f min %.f sec' % (minutes, seconds)
            else:
                if hours == 1:
                    est = ' completed in %.f hour %.f min %.f sec' % (hours, minutes, seconds)
                else:
                    est = ' completed in %.f hours %.f min %.f sec' % (hours, minutes, seconds)
            return '>[%s=%s] %d%%%s' % (bar, pad, perc, est)

        else:
            pad = '-' * (self.meter_ticks - self.meter_value)
            if len(self.estimated_duration) >= 1:
                # parameters to refine time-remaining estimation
                if self.estimated_duration[-1] < 15: exp = 1.75; dat = 10
                elif self.estimated_duration[-1] >= 15 and self.estimated_duration[-1] < 30: exp = 1.5; dat = 15
                elif self.estimated_duration[-1] >= 30 and self.estimated_duration[-1] < 90: exp = 1.25; dat = 50
                else: exp = 1.00; dat = 50
                # Calculation of time-remaining estimation
                wght_num, wght_den = (0 , 0)
                for i in range(0,min(len(self.estimated_duration),dat)):
                    wght_num += self.estimated_duration[-dat:][i] * ((i+1)**exp)
                    wght_den += (i+1)**exp
                est_dur = int(wght_num / wght_den)
                # Converting into hours, minutes and seconds
                hours,  remainder = divmod(est_dur,3600)
                minutes, seconds = divmod(remainder,60)
                if est_dur < 60:
                    est = ' %02.f seconds remaining' % seconds
                elif est_dur >= 60 and est_dur < 3600:
                    est = ' %02.f min %02.f sec remaining' % (minutes, seconds)
                else:
                    if hours == 1:
                        est = ' %.f hour %02.f min %02.f sec remaining' % (hours, minutes, seconds)
                    else:
                        est = ' %.f hours %02.f min %02.f sec remaining' % (hours, minutes, seconds)
                return '>[%s|%s] %d%%' % (bar, pad, perc)
            else:
                return '>[%s|%s] %d%% ' % (bar, pad, perc)
    def _refresh(self, *args, **kw):
        if self.switch_off:
            return
        else:
            # Clear line and return cursor to start-of-line
            sys.stdout.write(' ' * self.prev_meter_len + '\x08' * self.prev_meter_len)
            # Get meter text
            meter_text = self._get_meter(*args, **kw)
            # Write meter and return cursor to start-of-line
            sys.stdout.write(meter_text + '\x08'*len(meter_text))
            self.prev_meter_len = len(meter_text)

            # Are we finished?
            if self.count >= self.total:
                sys.stdout.write('\n')
                # Switches off method refresh (equivalent to killing the meter)
                # this is a safety measure in case of wrong usage where loops
                # continue beyond ProgressMeter.set(100) or ProgressMeter.update(total)
                self.switch_off = True
            sys.stdout.flush()

            # Timestamp
            self.last_refresh = time.time()

if __name__ == '__main__':

    import random

    print "this is a test of method ProgressMeter.update"

    total=20
    pba = ProgressMeter(total=total,unit='apples')
    while total > 0:
        cnt = random.randint(1, 2)
        pba.update(cnt)
        total -= cnt
        time.sleep(random.uniform(.25,0.75))

    print "this is a test of method ProgressMeter.set"

    pct = 0
    pbb = ProgressMeter(total=997,unit='oranges',ticks=80)
    while pct < 105:
        pct += random.randint(2, 5)
        pbb.set(pct)
        time.sleep(random.uniform(.25,0.75))

    print "this is a test of method ProgressMeter.reset and ProgressMeter.start and ProgressMeter.set(100)"

    pct = 0
    pbb.reset(unit='bananas',ticks=40)
    pbb.start()
    time.sleep(2)
    pbb.set(0)
    while pct < 94:
        pct += random.randint(2, 5)
        pbb.set(pct)
        time.sleep(random.uniform(.25,0.75))
    pbb.set(100)

    # Checks switch off in case of wrong usage (no console output expected)
    pbb.set(110)
    pbb.update(100000)

    print "this is a test message"
    print "the program will shutdown automatically"
    time.sleep(5)
    print "shutting down"
    time.sleep(0.15)
    sys.exit()
