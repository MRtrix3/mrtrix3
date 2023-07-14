#!/usr/bin/python3

# Copyright (c) 2008-2023 the MRtrix3 contributors.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Covered Software is provided under this License on an "as is"
# basis, without warranty of any kind, either expressed, implied, or
# statutory, including, without limitation, warranties that the
# Covered Software is free of defects, merchantable, fit for a
# particular purpose or non-infringing.
# See the Mozilla Public License v. 2.0 for more details.
#
# For more details, see http://www.mrtrix.org/.

import sys, os.path

if len (sys.argv) != 3:
  sys.stderr.write("usage: convert_bruker 2dseq header.mih\n")
  sys.exit (0)


#if os.path.basename (sys.argv[1]) != '2dseq':
  #print ("expected '2dseq' file as first argument")
  #sys.exit (1)

if not sys.argv[2].endswith ('.mih'):
  sys.stderr.write("expected .mih suffix as the second argument\n")
  sys.exit (1)



def main():

  with open (os.path.join (os.path.dirname (sys.argv[1]), 'reco'), encoding='utf-8') as file_reco:
    lines = file_reco.read().split ('##$')

  with open (os.path.join (os.path.dirname (sys.argv[1]), '../../acqp'), encoding='utf-8') as file_acqp:
    lines += file_acqp.read().split ('##$')

  with open (os.path.join (os.path.dirname (sys.argv[1]), '../../method'), encoding='utf-8') as file_method:
    lines += file_method.read().split ('##$')


  for line in lines:
    line = line.lower()
    if line.startswith ('reco_size='):
      mat_size = line.splitlines()[1].split()
      print ('mat_size', mat_size)
    elif line.startswith ('nslices='):
      nslices = line.split('=')[1].split()[0]
      print ('nslices', nslices)
    elif line.startswith ('acq_time_points='):
      nacq = len (line.split('\n',1)[1].split())
      print ('nacq', nacq)
    elif line.startswith ('reco_wordtype='):
      wtype = line.split('=')[1].split()[0]
      print ('wtype', wtype)
    elif line.startswith ('reco_byte_order='):
      byteorder = line.split('=')[1].split()[0]
      print ('byteorder', byteorder)
    elif line.startswith ('pvm_spatresol='):
      res = line.splitlines()[1].split()
      print ('res', res)
    elif line.startswith ('pvm_spackarrslicedistance='):
      slicethick = line.splitlines()[1].split()[0]
      print ('slicethick',  slicethick)
    elif line.startswith ('pvm_dweffbval='):
      bval = line.split('\n',1)[1].split()
      print ('bval', bval)
    elif line.startswith ('pvm_dwgradvec='):
      bvec = line.split('\n',1)[1].split()
      print ('bvec', bvec)


  with open (sys.argv[2], 'w', encoding='utf-8') as file_out:
    file_out.write ('mrtrix image\ndim: ' + mat_size[0] + ',' + mat_size[1])
    if len(mat_size) > 2:
      file_out.write (',' + str(mat_size[2]))
    else:
      try:
        nslices #pylint: disable=pointless-statement
        file_out.write (',' + str(nslices))
      except NameError:
        pass

    try:
      nacq #pylint: disable=pointless-statement
      file_out.write (',' + str(nacq))
    except NameError:
      pass

    file_out.write ('\nvox: ' + str(res[0]) + ',' + str(res[1]))
    if len(res) > 2:
      file_out.write (',' + str(res[2]))
    else:
      try:
        slicethick #pylint: disable=pointless-statement
        file_out.write (',' + str(slicethick))
      except NameError:
        pass
    try:
      nacq #pylint: disable=pointless-statement
      file_out.write (',')
    except NameError:
      pass

    file_out.write ('\ndatatype: ')
    if wtype == '_16bit_sgn_int':
      file_out.write ('int16')
    elif wtype == '_32bit_sgn_int':
      file_out.write ('int32')

    if byteorder=='littleendian':
      file_out.write ('le')
    else:
      file_out.write ('be')

    file_out.write ('\nlayout: +0,+1')
    try:
      nslices #pylint: disable=pointless-statement
      file_out.write (',+2')
    except NameError:
      pass
    try:
      nacq #pylint: disable=pointless-statement
      file_out.write (',+3')
    except NameError:
      pass

    file_out.write ('\nfile: ' + sys.argv[1] + '\n')

    try:
      assert len(bvec) == 3*len(bval)
      bvec = [ bvec[n:n+3] for n in range(0,len(bvec),3) ]
      for direction, value in zip(bvec, bval):
        file_out.write ('dw_scheme: ' + direction[0] + ',' + direction[1] + ',' + str(-float(direction[2])) + ',' + value + '\n')
    except AssertionError:
      pass

main()
