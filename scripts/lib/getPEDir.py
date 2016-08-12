def getPEDir(string):
  from lib.errorMessage import errorMessage
  try:
    PE_axis = abs(int(string))
    if PE_axis > 2:
      errorMessage('Phase encode axis must be either 0, 1 or 2')
    reverse = (string[0] == '-') # Allow -0
    return ( PE_axis, reverse )
  except:
    string = string.lower()
    if string == 'lr':
      return ( 0, False )
    elif string == 'rl':
      return ( 0, True )
    elif string == 'pa':
      return ( 1, False )
    elif string == 'ap':
      return ( 1, True )
    elif string == 'is':
      return ( 2, False )
    elif string == 'si':
      return ( 2, True )
    elif string == 'i':
      return ( 0, False )
    elif string == 'i-':
      return ( 0, True )
    elif string == 'j':
      return ( 1, False )
    elif string == 'j-':
      return ( 1, True )
    elif string == 'k':
      return ( 2, False )
    elif string == 'k-':
      return ( 2, True )
    else:
      errorMessage('Unrecognized phase encode direction specifier: ' + string)

