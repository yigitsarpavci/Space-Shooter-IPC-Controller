#!/bin/bash
# CMPE322 Operating Systems Project: Shooter
# Control script for Student ID: 2023400048

# Ignore SIGPIPE in case the reader process closes early
trap '' PIPE

FIFO="implementation/game.fifo"
if [ ! -d "implementation" ]; then
    FIFO="game.fifo"
fi

# Wait for the FIFO to be created by the game
while [ ! -p "$FIFO" ]; do
    if [ -p "implementation/game.fifo" ]; then
        FIFO="implementation/game.fifo"
        break
    elif [ -p "game.fifo" ]; then
        FIFO="game.fifo"
        break
    fi
    sleep 0.02
done

# Brief pause to ensure the game engine has completed initialization
sleep 0.2

# Path reaching the exit station adjacent to (33, 0)
MOVES="uuuuuuuuuuulluuuuuuuuuuuuuuluuuuruuuuuuuuuuuuuuuuurruuuuuuuurruuuuuuuul"

# Write each movement command into the FIFO
while [ -n "$MOVES" ]; do
    char="${MOVES:0:1}"
    MOVES="${MOVES:1}"
    printf '%s' "$char" > "$FIFO" 2>/dev/null
    sleep 0.13
done

# Keep running without exiting so the game stays on the YOU WIN screen
while true; do
    sleep 3600
done
