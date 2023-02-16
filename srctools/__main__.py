import enum
import os
import sys
import glob
from duplicate_checker import *
from line_counter import *

#
# trim_with_c()
#
# 文字列をしていした文字で区切る
def trim_with_c(s: str, c: str):
  return s.split(c)

#
# trim_with_comma()
#
# 文字列を カンマで区切る
def trim_with_comma(s, line):
  return line[len(s):].split(',')

def error(msg):
  print(msg)
  os._exit(1)

# ------------------------------
#  checker
# ------------------------------
def main() -> int:
  COM_CHECK_DUPLICATE     = '-dup.'
  COM_LINE_COUNT          = '-lcount.'

  try:
    for i, arg in enumerate(sys.argv):
      if i == 0:
        continue

      # check duplication of file name
      if arg.startswith(COM_CHECK_DUPLICATE):
        com_check_duplicate(trim_with_comma(COM_CHECK_DUPLICATE, arg))

      # line counter
      elif arg.startswith(COM_LINE_COUNT):
        trimmed = trim_with_comma(COM_LINE_COUNT, arg)
        extlist = [ ]

        if trimmed[0].startswith('ext='):
          extlist = trimmed[0][4:].split('.')
          trimmed = trimmed[1:]
        else:
          error('expected extensions list at first of arguments')

        com_line_count(trimmed, extlist)

      else:
        print(f'unknown argument: {arg}')
  except:
    print('invalid arguments')
    return 1

  return 0

if __name__ == "__main__":
  main()