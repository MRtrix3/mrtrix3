def getPEDir(string):
  from lib.debugMessage import debugMessage
  from lib.errorMessage import errorMessage
  pe_dir = ''
  try:
    PE_axis = abs(int(string))
    if PE_axis > 2:
      errorMessage('When specified as a number, phase encode axis must be either 0, 1 or 2')
    reverse = (string.contains('-')) # Allow -0
    pe_dir = [0,0,0]
    if reverse:
      pe_dir[PE_axis] = -1
    else:
      pe_dir[PE_axis] = 1
  except:
    string = string.lower()
    if string == 'lr':
      pe_dir = [1,0,0]
    elif string == 'rl':
      pe_dir = [-1,0,0]
    elif string == 'pa':
      pe_dir = [0,1,0]
    elif string == 'ap':
      pe_dir = [0,-1,0]
    elif string == 'is':
      pe_dir = [0,0,1]
    elif string == 'si':
      pe_dir = [0,0,-1]
    elif string == 'i':
      pe_dir = [1,0,0]
    elif string == 'i-':
      pe_dir = [-1,0,0]
    elif string == 'j':
      pe_dir = [0,1,0]
    elif string == 'j-':
      pe_dir = [0,-1,0]
    elif string == 'k':
      pe_dir = [0,0,1]
    elif string == 'k-':
      pe_dir = [0,0,-1]
    else:
      errorMessage('Unrecognized phase encode direction specifier: ' + string)
  debugMessage(string + ' -> ' + str(pe_dir))
  return pe_dir
