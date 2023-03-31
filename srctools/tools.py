import enum
import os
import sys
import glob
from collections import defaultdict

COL_BLACK           = '\033[30m'
COL_RED             = '\033[31;5m'
COL_GREEN           = '\033[32m'
COL_YELLOW          = '\033[33m'
COL_BLUE            = '\033[34m'
COL_MAGENTA         = '\033[35;5m'
COL_CYAN            = '\033[36;5m'
COL_WHITE           = '\033[37;5m'
COL_DEFAULT         = '\033[39m'

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

def check_duplication(path: str) -> list:
    if path[-1] == '/':
        path += '**/*'
    else:
        path += '/**/*'

    all_files = glob.glob(path, recursive=True)
    dictvec = defaultdict(list)
    ret = [ ]

    for file in all_files:
        s = os.path.basename(file)

        dictvec[s].append(file)

    for k in dictvec.keys():
        if len(dictvec[k]) > 1:
            ret.append(dictvec[k])

    return ret

def com_check_duplicate(folders: list):
    for folder in folders:
        print(f'{COL_CYAN}check filename duplication in {COL_GREEN}"{folder}" {COL_CYAN}...{COL_DEFAULT}')

        if not os.path.isdir(folder):
            print(f'{COL_MAGENTA}folder not found')
            continue

        res = check_duplication(folder)

        if len(res) != 0:
            print(COL_RED + 'file name duplicated in folder ' + COL_YELLOW + '"' + folder + COL_DEFAULT + \
                '":\n    ' + '\n'.join(['\n    '.join(v) for v in res]))
        else:
            print('no duplicated files.')

def com_line_count(folders: list, ext: list):
    total = 0

    for folder in folders:
        path = f'./{folder}/'
        files = glob.glob(f'{path}/**/*', recursive=True)
        print(f'\n{folder}:')

        tmp = [ ]

        for file in files:
            if os.path.splitext(file)[1][1:] in ext:
                count = sum([1 for _ in open(file)])
                total += count
                tmp.append([count, f'    {os.path.basename(file)}: {count}'])

        tmp.sort()
        [print(x[1]) for x in tmp[::-1]]

    print(f'\ntotal: {total}')

# ------------------------------
#    srctools
# ------------------------------
def main() -> int:
    COM_CHECK_DUPLICATE         = '-dup.'
    COM_LINE_COUNT              = '-lcount.'

    try:
        for i, arg in enumerate(sys.argv):
            if i == 0:
                continue

            # ソースファイル名の重複がないか確認する
            if arg.startswith(COM_CHECK_DUPLICATE):
                com_check_duplicate(trim_with_comma(COM_CHECK_DUPLICATE, arg))

            # 行数カウント
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