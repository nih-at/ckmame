#!/usr/bin/env python3

import argparse
import re


def create_zip_version(name):
    """Create zip version of test case."""
    with open(name, 'r', encoding='utf-8') as input_file:
        output_name = name[:-5] + '.zip.test'
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
        output_name = name[:-5] + '.dir.test'
        with open(output_name, 'w', encoding='utf-8') as output_file:
            for line in input_file.readlines():
                if line.startswith('<zip>'):
                    continue
                if line.startswith('<dir>'):
                    line = line[6:]
                elif line in ('stdout\n', 'stderr\n'):
                    inline_data = True
                elif line in ('end-of-inline-data\n'):
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


parser = argparse.ArgumentParser(description='Create a dir-variant test from '
                                 + 'one supporting both types.')
parser.add_argument('input', type=str, help='input test case', nargs='+')

args = parser.parse_args()

for file_name in args.input:
    create_zip_version(file_name)
    create_dir_version(file_name)
