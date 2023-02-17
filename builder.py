import os
import sys
import glob

class Builder:

  def __init__(self):
    self.target = ""

    self.incl_dir = "include"
    self.srcdir = "src"

    self.extension = {
      "c": "c",
      "cpp": "cc"
    }

  def get_source_files(self) -> list:
    ret = list()

    for k in self.extension:
      ret.extend(glob.glob(f"{self.srcdir}/**/*.{self.extension[k]}"))

    return ret

  #
  # do building !!!
  def build(self):

    pass



def main(args: list) -> int:
  

  return 0


main(sys.argv)

