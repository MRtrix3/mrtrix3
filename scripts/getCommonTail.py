def get(inputFiles):
  first = inputFiles[0];
  cursor = 0
  found = False;
  common = ''
  for i in reversed(first):
    if found == False:
      for j in inputFiles:
        if j[len(j)-cursor-1] != first[len(first)-cursor-1]:
          found = True
          break
      if found == False:
        common = first[len(first)-cursor-1] + common
      cursor += 1
  return common