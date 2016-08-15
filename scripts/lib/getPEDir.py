def getPEDir(string):
  from lib.debugMessage import debugMessage
  from lib.errorMessage import errorMessage
  pe_dir = ''
  try:
    PE_axis = abs(int(string))
    if PE_axis > 2:
      errorMessage('Phase encode axis must be either 0, 1 or 2')
    reverse = (string[0] == '-') # Allow -0
    pe_dir = ( PE_axis, reverse )
  except:
    string = string.lower()
    if string == 'lr':
      pe_dir = ( 0, False )
    elif string == 'rl':
      pe_dir = ( 0, True )
    elif string == 'pa':
      pe_dir = ( 1, False )
    elif string == 'ap':
      pe_dir = ( 1, True )
    elif string == 'is':
      pe_dir = ( 2, False )
    elif string == 'si':
      pe_dir = ( 2, True )
    elif string == 'i':
      pe_dir = ( 0, False )
    elif string == 'i-':
      pe_dir = ( 0, True )
    elif string == 'j':
      pe_dir = ( 1, False )
    elif string == 'j-':
      pe_dir = ( 1, True )
    elif string == 'k':
      pe_dir = ( 2, False )
    elif string == 'k-':
      pe_dir = ( 2, True )
    else:
      errorMessage('Unrecognized phase encode direction specifier: ' + string)
  debugMessage(string + ' -> ' + str(pe_dir) + ' (axis, reversed)')
  return pe_dir
