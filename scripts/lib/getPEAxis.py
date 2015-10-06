def getPEAxis(string):
  from lib.errorMessage import errorMessage
  if string.isdigit():
    PE_axis = int(string)
    if PE_axis < 0 or PE_axis > 2:
      errorMessage('Phase encode axis must be either 0, 1 or 2')
    return PE_axis
  else:
    string = string.lower()
    if string == 'lr' or string == 'rl':
      return 0
    elif string == 'ap' or string == 'pa':
      return 1
    elif string == 'is' or string == 'si':
      return 2
    else:
      errorMessage('Unrecognized phase encode direction specifier')

