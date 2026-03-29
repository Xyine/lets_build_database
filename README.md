# Let's Build a Database

![C](https://img.shields.io/badge/language-C-blue)
![Status](https://img.shields.io/badge/status-in%20progress-yellow)
![Architecture](https://img.shields.io/badge/structure-B--Tree-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

## Overview

This project is a **from-scratch database engine** written in C, inspired by SQLite.

It implements core database internals such as:
- a pager for disk I/O
- row serialization
- and a **B-tree index structure**

The goal is to understand how real databases work at a low level.

## Features

### Supported
- Insert rows  
- Select all rows  
- Persistent storage (file-based)  
- B-tree storage engine:
  - Leaf nodes
  - Internal nodes
  - Node splitting
  - Multi-level trees
- Duplicate key detection  
- Tree visualization (`.btree`)  

### Not yet implemented
- DELETE / UPDATE
- Indexed search optimization
- Transactions
- Concurrency

## Getting Started

### Compile

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror db.c -o db
```

Run
```bash
./db database.db
```

### Usage

SQL-like Commands
```bash
insert <id> <username> <email>
select
```

Example:
```bash
db > insert 1 user1 person1@example.com
Executed.
db > select
(1, user1, person1@example.com)
```

### Meta Commands
```bash
.exit        # Exit the database
.constants   # Show internal constants
.btree       # Display B-tree structure
```

## Example: B-tree Output
```bash
Tree:
- internal (size 1)
  - leaf (size 7)
    - 1
    - 2
    - 3
    - 4
    - 5
    - 6
    - 7
  - key 7
  - leaf (size 7)
    - 8
    - 9
    - 10
    - 11
    - 12
    - 13
    - 14
```

## Architecture
```
+------------------+
|     Table        |
+------------------+
         |
         v
+------------------+
|     Pager        |  <-- Handles disk I/O
+------------------+
         |
         v
+------------------+
|     B-Tree       |
|  (Pages/Nodes)   |
+------------------+
     /        \
 Leaf nodes   Internal nodes
```

## Project Structure

| Component | Role |
|----------|------|
| `Pager`  | Handles reading/writing pages to disk |
| `Table`  | Entry point of the database |
| `Cursor` | Iterates through rows |
| `Row`    | Data representation |
| `B-tree` | Storage & indexing |

## Current Progress

- Row format & serialization

- Pager system

- Leaf node insertion

- Leaf node splitting

- Internal nodes

- Multi-level B-tree

- Duplicate key detection

- Tree visualization

## Credits

This project follows the tutorial by Connor Stack:
https://cstack.github.io/db_tutorial/

## Why this project?

To deeply understand:

- how databases store data

- how indexing works (B-tree)

- how disk I/O is managed

- how query execution works internally

## Tests

The project includes Python tests that:

- simulate user input

- validate output

- ensure correctness of the B-tree structure

Run tests (after compiling the c code) with:
```bash
python3 test_db.py
```

## Next Steps

- Improve internal node splitting

- Implement search optimization

- Add DELETE / UPDATE

- Add persistence improvements

- Improve error handling
