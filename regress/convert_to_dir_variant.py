#!/usr/bin/env python3

import argparse
import re


def create_zip_version(name):
    """Create zip version of test case."""
    with open(name, 'r', encoding='utf-8') as input_file:
        last_slash = name.rfind('/')
        if last_slash != -1:
            name = name[last_slash+1:]
        last_dot = name.rfind('.')
        output_name = name[:last_dot] + '.zip.test'
        with open(output_name, 'w', encoding='utf-8') as output_file:
            for line in input_file.readlines():
                if line.startswith('<dir>'):
                    continue
                if line.startswith('<zip>'):
                    line = line[6:]
                output_file.write(line)


def create_dir_version(name):
    """Create dir version of test case."""
    with open(name, 'r', encoding='utf-8') as input_file:
        inline_data = False
        last_slash = name.rfind('/')
        if last_slash != -1:
            name = name[last_slash+1:]
        last_dot = name.rfind('.')
        output_name = name[:last_dot] + '.dir.test'
        with open(output_name, 'w', encoding='utf-8') as output_file:
            for line in input_file.readlines():
                if line.startswith('<zip>'):
                    continue
                if line.startswith('<dir>'):
                    line = line[6:]
                elif line in ('stdout\n', 'stderr\n'):
                    inline_data = True
                elif line == 'end-of-inline-data\n':
                    inline_data = False
                elif inline_data:
                    line = re.sub(r'\.zip([:/])', r'\1', line)
                else:
                    # remove 'hashes.*cheap' lines
                    if re.match(r'^hashes\s.*\scheap$', line):
                        continue
                    line = re.sub(r'^(file \S*).zip', r'\1', line)
                    line = re.sub(r'^(hashes \S*).zip', r'\1', line)
                    line = re.sub(r'^(arguments )', r'\1 --roms-unzipped ',
                                  line)
                    line = re.sub(r'(\.ckmamedb)>', r'\1-unzipped>', line)
                output_file.write(line)


parser = argparse.ArgumentParser(description='Create a dir- and/or zip-variant test from '
                                 + 'one supporting both types. An input file of name.test will have name.dir.test and/or name.zip.test as output file.')
parser.add_argument('--dir', action='store_true', help="convert to directory test case (default)")
parser.add_argument('--zip', action='store_true', help="convert to zip test case")
parser.add_argument('input', type=str, help='input test case', nargs='+')

args = parser.parse_args()

# default to '--dir' if no arguments are given
if not args.dir and not args.zip:
    args.dir = True

for file_name in args.input:
    if args.dir:
        create_dir_version(file_name)
    if args.zip:
        create_zip_version(file_name)
