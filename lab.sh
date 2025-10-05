#!/bin/sh
set -u

error() {
  printf '%s\n' "$1" >&2
}

usage ()
{
  error "Usage: $0 /path/to/source.file"
  exit 1
}

# количество аргументов = 1
[ $# -eq 1 ] || usage

SRC_PATH=$1
[ -f "$SRC_PATH" ] || { error "Source file not found: $SRC_PATH"; exit 1; }

SRC_ABS=$(cd "$(dirname "$SRC_PATH")" && pwd)/$(basename "$SRC_PATH")
SRC_DIR=$(dirname "$SRC_ABS")
SRC_BASE=$(basename "$SRC_ABS")

# Извлекаем имя выходного файла
OUTNAME=$(sed -n 's/.*&Output:[[:space:]]*\([^[:space:]]*\).*/\1/p' "$SRC_ABS" | head -n 1)
[ -n "$OUTNAME" ] || { error "'&Output: name' not found in the source."; exit 3; }

# Создаем временную директорию
TMPDIR=$(mktemp -d 2>/dev/null) || { error "mktemp failed"; exit 2; }

cleanup () 
{
  [ -n "${TMPDIR:-}" ] && [ -d "$TMPDIR" ] && rm -rf "$TMPDIR"
}

trap_handler(){
  exit_code=$?
  cleanup
  exit "$exit_code"
}
trap 'trap_handler' EXIT INT TERM HUP QUIT
 
# Копируем файл
cp "$SRC_ABS" "$TMPDIR/" || { error "Failed to copy source"; exit 6; }
# Строим команду 
case "$SRC_BASE" in
  *.c)
    COMPILER=${CC:-cc} 
    BUILD_CMD="$COMPILER -o \"$OUTNAME\" \"$SRC_BASE\""
    OUT_EXT=""
    ;;
  *.cc|*.cpp|*.cxx|*.C)
    COMPILER=${CXX:-c++}
    BUILD_CMD="$COMPILER -o \"$OUTNAME\" \"$SRC_BASE\""
    OUT_EXT=""
    ;;
  *.tex)
    if command -v pdflatex >/dev/null 2>&1; then 
      BUILD_CMD="pdflatex -jobname=\"$OUTNAME\" -interaction=nonstopmode -halt-on-error \"$SRC_BASE\" >/dev/null"
      OUT_EXT=".pdf"
    else
      error "pdflatex not found in PATH"
      exit 5
    fi
    ;;
  *)
    error "Unsupported file type: $SRC_BASE"
    exit 4
    ;;
esac

# выполняем внутри временного каталога 
(
  cd "$TMPDIR" || exit 8
  [ -f "$SRC_BASE" ] || cp "$SRC_ABS" . || { error "Failed to copy source"; exit 6; }
  sh -c "$BUILD_CMD"
  build_rc=$?
  [ "$build_rc" -eq 0 ] || { error "Compilation failed with exit code $build_rc."; exit 7; }
)

PRODUCED_FILE="$TMPDIR/$OUTNAME$OUT_EXT"
[ -f "$PRODUCED_FILE" ] || { error "Output '$OUTNAME' not produced"; exit 6; }

DEST="$SRC_DIR/$OUTNAME$OUT_EXT"
mv -f "$PRODUCED_FILE" "$DEST" || { error "Failed to move output to $DEST"; exit 9; }

printf 'Build successful: %s\n' "$DEST"
exit 0