## dwdiff Developer Overview

### Terminology note
Most references to "diff" like `prepareAndExecuteDiff` are not referring the standard `diff` tool but instead referring to the "diffing" that dwdif itself does.

### File Notes

There are two primary files in dwdiff.

`dwdiff.c` primarily handles taking diff input and stripping many of the special characters for later processing by the heart of the program: `doDiff.c`

