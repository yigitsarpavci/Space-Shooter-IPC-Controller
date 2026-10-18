#!/bin/bash
# CMPE322 Operating Systems Project: Shooter
# Author: Mehmet Saraçoğlu
# Designed as course material for use in the CMPE322 Operating Systems lectures.

# Builds and runs the game.

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 1
USAGE="usage: ./run.sh <student_id: 10 digits> [fifo|keyboard] [level] [control]"
ID="$1"
[[ "$ID" =~ ^[0-9]{10}$ ]] || { echo "$USAGE" >&2; exit 1; }
MODE="${2:-fifo}"
N="${3:-1}"
LEVEL="levels/level$(printf '%02d' "${N#0}" 2>/dev/null).txt"
[ -r "$LEVEL" ] || { echo "run.sh: cannot read $LEVEL" >&2; exit 1; }
CONTROL="implementation/${4:-ctl}.sh"

case "$MODE" in
    fifo)
        [ -r "$CONTROL" ] || { echo "run.sh: cannot read $CONTROL" >&2; exit 1; }
        rm -f implementation/game.fifo # Stale fifo.
        make fifo && { bash "$CONTROL" & ./game "$ID" "$LEVEL"; } ;;
    keyboard)
        make keyboard && ./game "$ID" "$LEVEL" ;;
    *)
        echo "$USAGE" >&2; exit 1 ;;
esac
