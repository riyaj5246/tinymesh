#!/usr/bin/env bash
# Run a command with a faster allocator (jemalloc or tcmalloc) if available.
# Usage: ./scripts/run_with_allocator.sh [jemalloc|tcmalloc] -- <command> [args...]

set -euo pipefail

if [ "$#" -lt 3 ]; then
  echo "Usage: $0 [jemalloc|tcmalloc] -- <command> [args...]"
  exit 2
fi

ALLOC="$1"
shift
if [ "$1" != "--" ]; then
  echo "Missing -- separator"
  exit 2
fi
shift

CMD=("$@")

# Common library paths to check
LIBCANDIDATES=(
  "/usr/lib/x86_64-linux-gnu/libjemalloc.so.2"
  "/usr/lib/libjemalloc.so.2"
  "/usr/lib64/libjemalloc.so.2"
  "/usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4"
  "/usr/lib/libtcmalloc_minimal.so.4"
  "/usr/lib64/libtcmalloc_minimal.so.4"
  "/usr/lib/libtcmalloc.so"
)

LD_PRELOAD_LIB=""
if [ "$ALLOC" = "jemalloc" ]; then
  for p in "${LIBCANDIDATES[@]}"; do
    if [ -f "$p" ]; then
      LD_PRELOAD_LIB="$p"
      break
    fi
  done
elif [ "$ALLOC" = "tcmalloc" ]; then
  for p in "${LIBCANDIDATES[@]}"; do
    if [[ "$p" == *tcmalloc* ]] && [ -f "$p" ]; then
      LD_PRELOAD_LIB="$p"
      break
    fi
  done
else
  echo "Unknown allocator: $ALLOC"
  exit 2
fi

if [ -z "$LD_PRELOAD_LIB" ]; then
  echo "Allocator library not found for $ALLOC. Running command without LD_PRELOAD."
  exec "${CMD[@]}"
else
  echo "Running with LD_PRELOAD=$LD_PRELOAD_LIB"
  LD_PRELOAD="$LD_PRELOAD_LIB" "${CMD[@]}"
fi
