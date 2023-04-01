import os
import sys
import glob
import datetime

def GetAboutString():
    return \
"""
文字列置き換え、ファイル名変更ツール
2023/4/2  letz#8687 (bomkei)

デフォルトでは、拡張子の前に .replaced を挿入した名前で保存する

usage:
    python3 str_replace.py [options...]

options:
    --file                  ファイルを指定
    --dir                   ディレクトリを指定
    --extension     -e      拡張子を指定
    --match         -m      指定した正規表現に一致するファイル名のみ取得
    --recursive     -r      ディレクトリ内の全てのサブフォルダから再帰的にファイルを取得する
    --overwrite             上書きする  (バックアップを自動作成します フォルダ名 = "_backup/YYMMDD_H:M.S" )
    --from          -f      置き換える前の文字列・ファイル名
    --to            -t      置き換えた後の文字列・ファイル名
    --help          -h      このメッセージを表示

examples:
    ヘッダファイルを移動したとき、ディレクトリ src 以下の全ての .cpp ファイル内のプリプロセッサディレクティブを置き換える
    変更前 = header.h
    変更後 = new-header.h
        python3 str_replace.py --dir src -e .cpp -f "#include \\"header.h\\"" -t "#include \\"new-header.h\\""
"""

# TODO: Rewrite in metro

class ReplaceTool:
    def __init__(self, isRecursive, isOverwrite):
        self.isOverwrite = isOverwrite
        self.isRecursive = isRecursive
        
        dt = datetime.datetime.now()
        self.bakpath = "_backup/" + dt.strftime("%Y%m%d_%H:%M.%S")

        os.system(f"mkdir -p {self.bakpath}")

    def Initialize(self):
        pass

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
    if len(argv) == 1:
        print(GetAboutString())
        exit(0)

    if "-h" in argv:
        print(GetAboutString())
        return 0

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

sys.exit(main(sys.argv))