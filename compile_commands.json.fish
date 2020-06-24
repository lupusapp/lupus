#!/bin/fish
ninja -C build -t compdb > compile_commands.json; and mv build/compile_commands.json .
