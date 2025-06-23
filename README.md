# Git Implementation (C++)  
*A minimal implementation of core Git mechanics*

<!-- [![Build Status](https://img.shields.io/github/actions/workflow/status/ahhmed-hassan/codecrafters-git-cpp/build.yml?style=flat-square)](https://github.com/ahhmed-hassan/codecrafters-git-cpp/actions)
 -->
This project implements foundational Git operations from scratch in C++, focusing on storage mechanics and tree-based version control. It serves as an educational resource for understanding Git's internal architecture.

## Implemented Features
| Command                 | Functionality                                      |
|-------------------------|----------------------------------------------------|
| `git init`              | Initialize .git directory structure                |
| `git cat-file -p <sha>` | Pretty-print blobs            |
| `git hash-object`       | Compute SHA-1 hashes and store objects             |
| `git write-tree`        | Serialize directory structure to tree objects      |
| `git ls-tree --names-only` | List filenames in a tree (without metadata)      |
| `git clone <url>`       | Clone remote repositories (local path support)     |

## Key Technical Insights
- **Git Object Model**: Implements blobs, trees, and commits with proper SHA-1 serialization
- **Packfile-Free Storage**: Objects stored in `objects/` using Git's "loose object" format
- **Ref Handling**: Simple ref storage under `refs/heads/`
- **Tree Resolution**: Recursive directory traversal for tree writing/lookup

## Build & Run
**Prerequisites**: C++23 compiler, CMake ≥3.13, zlib (for deflate)

<!-- ```bash
$ cmake -B build -DCMAKE_BUILD_TYPE=Release
$ cmake --build build
$ ./build/git-cpp <command> [args] --> 
