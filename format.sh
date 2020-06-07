#!/bin/bash
find controllers/ src/ include/ scriptrunner/ -name "*.hpp" -o -name "*.cpp" |xargs clang-format -i -style=file
