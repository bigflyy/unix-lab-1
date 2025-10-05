#!/bin/sh
# шебанг путь до интерпетатора который используется для выполнения команд 

# чтобы использование необъявленных переменных считались ошибкой 
set -u

# Переменные в двойных кавычках для случая когда есть пробелы в значении чтобы оно не разбилось на слова отдельные в команде 

error() {
  printf '%s\n' "$1" >&2
}

usage ()
{
  error "Usage: $0 /path/to/source.file"
  exit 1
}

# Количество аргументов должно быть один, если это не так, то вызывать usage
[ $# -eq 1 ] || usage

# Проверить что файл существует, вывести ошибку если  не существует 
SRC_PATH=$1
[ -f "$SRC_PATH" ] || { error "Source file not found: $SRC_PATH"; exit 1; }

# Командная подстановка (заменяет выражение на вывод выполненной команды)
SRC_ABS=$(cd "$(dirname "$SRC_PATH")" && pwd)/$(basename "$SRC_PATH")
SRC_DIR=$(dirname "$SRC_ABS")
SRC_BASE=$(basename "$SRC_ABS")

# потоковый редактор, для обработки текста
# -n чтобы не выводилось строки
# 's/шаблон/замена/p' p - флаг распечатать строку которая подходит под шаблон при успешном выполнении 
# .* - любые символы до и после 
# Наш литерал 
# затем любое количество пробелов (0 и больше)
# затем группа 1 \(...\)
# затем [^[:space]]* - любые символы кроме пробела
# любые символы
# заменяем это все на то что было в группе 1, и выводим
# Путь к файлу который читается 
# труба, передает стандартный вывод команды слева и передает ее как стандартный ввод команде справа и только первую берет 
OUTNAME=$(sed -n 's/.*&Output:[[:space:]]*\([^[:space:]]*\).*/\1/p' "$SRC_ABS" | head -n 1)
[ -n "$OUTNAME" ] || { error "'&Output: name' not found in the source."; exit 3; }

# d is for directory, перенаправление потока ошибок 
TMPDIR=$(mktemp -d 2>/dev/null) || { error "mktemp failed"; exit 2; }

# Проверка что TMPDIR - не пустая строка, что существует каталог TMPDIR, удаляем каталог
cleanup () 
{
  [ -n "${TMPDIR:-}" ] && [ -d "$TMPDIR" ] && rm -rf "$TMPDIR"
}
# $? - это код завершения последней команды, храним в rc, вызываем cleanup и возвращаем код rc, если происходит одно из EXIT INT TERM HUP QUIT. 
# EXIT - скрипт завершает работу 
# INT - Ctrl + C
# TERM - сигнал завершиться, вежливая просьба 
# HUP - hang up, закрытие терминала 
# QUIT - Ctrl + \, завершает работу и создает дамп ядра для отладки 
trap_handler(){
  exit_code=$?
  cleanup
  exit "$exit_code"
}
trap 'trap_handler' EXIT INT TERM HUP QUIT
 
# Копируем файл
cp "$SRC_ABS" "$TMPDIR/" || { error "Failed to copy source"; exit 6; }

case "$SRC_BASE" in
  *.c)
    # если слева пусто то значение по умолчанию cc 
    COMPILER=${CC:-cc} 
    BUILD_CMD="$COMPILER -o \"$OUTNAME\" \"$SRC_BASE\""
    OUT_EXT=""
    ;;
  # если совпадает с любым из них логическое или в строках 
  *.cc|*.cpp|*.cxx|*.C)
    COMPILER=${CXX:-c++}
    BUILD_CMD="$COMPILER -o \"$OUTNAME\" \"$SRC_BASE\""
    OUT_EXT=""
    ;;
  *.tex)
    if command -v pdflatex >/dev/null 2>&1; then 
      # -interaction=nonstopemode - чтобы комплиятор не прерывался на ввод пользователя если будут предупреждения 
      # hoe - чтобы прекращал работу
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

# printf для управляющих последовательностей
printf 'Build successful: %s\n' "$DEST"
exit 0