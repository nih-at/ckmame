#!/usr/bin/env python

import re
import sys

table_header_re = re.compile(r'^>>> table (\w+) \((.*)\)$')

class Dump:
    class Row:
        def __init__(self, columns: list[str], values: list[str]):
            if len(columns) != len(values):
                raise ValueError(f"row has {len(values)} values but expected {len(columns)}")
            self.columns = columns
            self.values = values
        
        def __getitem__(self, key):
            index = self.columns.index(key)
            return self.values[index]
        
        def __setitem__(self, key, value):
            index = self.columns.index(key)
            self.values[index] = value
        
        def __lt__(self, other):
            return self.values < other.values

    class Table:
        def __init__(self, name: str, columns: list[str]):
            self.name = name
            self.columns = columns
            self.rows: list[Dump.Row] = []
        
        def append(self, row: list[str]):
            if len(row) != len(self.columns):
                raise ValueError(f"row has {len(row)} columns but expected {len(self.columns)}")
            self.rows.append(Dump.Row(self.columns, row))
        
        def sort(self):
            self.rows.sort()
        
        def __iter__(self):
            return iter(self.rows)

    def __init__(self, filename: str|None = None):
        self.tables = []
        if filename:
            self.load(filename)
    
    def load(self, filename: str):
        with open(filename, 'r') as f:
            table = None
            for line in f:
                line = line.strip()
                match = table_header_re.fullmatch(line)
                if match:
                    name = match.group(1)
                    columns = [col.strip() for col in match.group(2).split(',')]
                    table = self.Table(name, columns)
                    self.add_table(table)
                else:
                    if table is None:
                        raise ValueError(f"row outside of table")
                    table.append(line.split('|'))
    
    def save(self, filename: str):
        with open(filename, 'w') as f:
            for table in self.tables:
                f.write(f">>> table {table.name} ({', '.join(table.columns)})\n")
                for row in table.rows:
                    f.write('|'.join(map(str, row.values)) + '\n')

    def add_table(self, table: Table):
        self.tables.append(table)
    
    def __getitem__(self, name: str) -> Table:
        for table in self.tables:
            if table.name == name:
                return table
        raise KeyError(f"table '{name}' not found")


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <dump file>")
        sys.exit(1)
    
    filename = sys.argv[1]
    dump = Dump(filename)

    original_game_indices = {}
    for row in dump['game']:
        game_id = int(row['game_id'])
        original_game_indices[row['name']] = game_id

    game_index_map = {}
    for (new_index, name) in enumerate(sorted(original_game_indices.keys())):
        original_index = original_game_indices[name]
        game_index_map[original_index] = new_index + 1

    for row in dump['game']:
        row['game_id'] = game_index_map[int(row['game_id'])]
    for row in dump['file']:
        row['game_id'] = game_index_map[int(row['game_id'])]
        row['file_idx'] = int(row['file_idx'])

    dump['game'].sort()
    dump['file'].sort()

    dump.save(filename)

if __name__ == '__main__':
    main()
