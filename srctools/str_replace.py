import os
import sys
import glob
import datetime

def GetAboutString():
    return \
"""
ソースコード上の文字列を任意の文字列で置き換える

デフォルトでは、拡張子の前に .replaced を挿入した名前で保存する

usage:
    python3 str_replace.py <source-file> <from> <to> [options...]

options:
    -r      フォルダを指定した場合、再帰で全てのファイルを対象にする
    -w      上書きする  バックアップを自動作成します
"""

class ReplaceTool:
    def __init__(self, isRecursive, isOverwrite):
        self.isOverwrite = isOverwrite
        self.isRecursive = isRecursive
        
        dt = datetime.datetime.now()
        self.bakpath = "_backup/" + dt.strftime("%Y%m%d_%H:%M.%S")

        os.system(f"mkdir -p {self.bakpath}")

    def ReplaceInFile(self, Path, From, To):
        data = ""
        replaced = ""
        
        with open(Path, mode="r") as fs:
            data = fs.read()
            replaced = data.replace(From, To)

        if not self.isOverwrite:
            NewPath = Path[Path.rfind("."):]
            NewPath = os.path.basename(Path[:Path.rfind(".")]) + ".replaced" + NewPath
            
            with open(NewPath, mode="w+") as fs:
                fs.write(replaced)
        else:
            with open(f"{self.bakpath}/{os.path.basename(Path)}", "w") as fs:
                fs.write(data)
    
            with open(Path, mode="w+") as fs:
                fs.write(replaced)

    def ReplaceInDirectory(self, Dir, From, To):
        if self.isRecursive:
            files = glob.glob(f"{Dir}/**/*", recursive=True)
        else:
            files = glob.glob(f"{Dir}/*")

        for file in files:
            if not os.path.isdir(file):
                self.ReplaceInFile(file, From, To)

def main(argv):
    if len(argv) < 4:
        print(GetAboutString())
        exit(0)

    Path      = argv[1]
    From      = argv[2]
    To        = argv[3]
    
    isRecursive = "-r" in argv
    isOverWrite = "-w" in argv
    
    repl = ReplaceTool(isRecursive=isRecursive, isOverwrite=isOverWrite)
    
    if os.path.isdir(Path):
        repl.ReplaceInDirectory(Path, From, To)
    else:
        repl.ReplaceInFile(Path, From, To)

main(sys.argv)