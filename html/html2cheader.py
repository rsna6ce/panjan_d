#!/usr/bin/env python3
import os
import argparse

def main():
    parser = argparse.ArgumentParser(prog='html2cheader.py')
    parser.add_argument('-i', '--input', help='input html file', required=True, dest='input')
    parser.add_argument('-o', '--output', help='output html file', required=True, dest='output')
    args = parser.parse_args()
    html2cheader(args)

def html2cheader(args):
    basename = os.path.basename(args.input)
    lines = []
    with open(args.input, 'r', encoding="utf-8") as f:
        lines = f.readlines()
        for i in range(len(lines)):
            line = '"{}\\n"'.format(lines[i].rstrip().replace('"', '\\"'))
            lines[i] = line
    with open(args.output, 'w', encoding='utf-8') as f:
        lines.insert(0, '#pragma once')
        lines.insert(1, 'String {} = '.format(basename.replace('.','_')))
        s = "\n".join(lines) + ';'
        f.write(s)

if __name__ == '__main__':
    main()