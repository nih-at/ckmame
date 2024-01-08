#!/usr/bin/env python3

import argparse
import re

parser = argparse.ArgumentParser(description='Create a dir-variant test from one supporting both types.')
parser.add_argument('input', type=str, help='input test case')
parser.add_argument('output', type=str, help='output test case')

args = parser.parse_args()

with open(args.input, 'r', encoding='utf-8') as input_file:
    inline_data = False
    with open(args.output, 'w', encoding='utf-8') as output_file:
        for line in input_file.readlines():
            if line in ('stdout\n', 'stderr\n'):
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
                line = re.sub(r'^(arguments )', r'\1 --roms-unzipped ', line)
                line = re.sub(r'(\.ckmamedb)>', r'\1-unzipped>', line)
            output_file.write(line)
