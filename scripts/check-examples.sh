#!/bin/bash

set -e

CCLYNX="./bin/cclynx"
AS="aarch64-linux-gnu-as"
GCC="aarch64-linux-gnu-gcc"
QEMU="qemu-aarch64"
QEMU_FLAGS="-L /usr/aarch64-linux-gnu"
DEFAULT_WRAPPER="./scripts/int_wrapper.c"
TMPDIR=$(mktemp -d)

trap "rm -rf $TMPDIR" EXIT

passed=0
failed=0
errors=0

for src in ./examples/*.c; do
    filename=$(basename "$src")

    # parse "// expected return: <value>" from first line
    header=$(head -1 "$src")
    if ! echo "$header" | grep -q "// expected return:"; then
        echo "SKIP: $filename (no expected return comment)"
        continue
    fi

    expected=$(echo "$header" | sed 's|// expected return: *||')

    # parse optional "// wrapper: <path>" from second line
    wrapper_line=$(sed -n '2p' "$src")
    if echo "$wrapper_line" | grep -q "// wrapper:"; then
        wrapper=$(echo "$wrapper_line" | sed 's|// wrapper: *||')
        skip_lines=2
    else
        wrapper="$DEFAULT_WRAPPER"
        skip_lines=1
    fi

    asm_file="$TMPDIR/${filename%.c}.s"
    obj_file="$TMPDIR/${filename%.c}.o"
    bin_file="$TMPDIR/${filename%.c}"

    # step 1: compile with cclynx (strip comment lines first)
    stripped_file="$TMPDIR/${filename}"
    tail -n +$((skip_lines + 1)) "$src" > "$stripped_file"
    if ! $CCLYNX "$stripped_file" > "$asm_file" 2>&1; then
        echo "ERROR: $filename — cclynx compilation failed"
        errors=$((errors + 1))
        continue
    fi

    # step 2: assemble with aarch64 assembler
    if ! $AS -o "$obj_file" "$asm_file" 2>&1; then
        echo "ERROR: $filename — assembler rejected output"
        errors=$((errors + 1))
        continue
    fi

    # step 3: link and run
    if [ "$wrapper" = "$DEFAULT_WRAPPER" ]; then
        # default wrapper: check via exit code
        if ! $GCC -static -o "$bin_file" "$wrapper" "$obj_file" 2>&1; then
            echo "ERROR: $filename — linking failed"
            errors=$((errors + 1))
            continue
        fi

        set +e
        $QEMU $QEMU_FLAGS "$bin_file" 2>/dev/null
        actual=$?
        set -e

        if [ "$actual" -eq "$expected" ]; then
            echo "PASS: $filename (expected $expected, got $actual)"
            passed=$((passed + 1))
        else
            echo "FAIL: $filename (expected $expected, got $actual)"
            failed=$((failed + 1))
        fi
    else
        # custom wrapper: pass expected as argument, check via exit code + stderr
        if ! $GCC -static -o "$bin_file" "$wrapper" "$obj_file" 2>&1; then
            echo "ERROR: $filename — linking failed"
            errors=$((errors + 1))
            continue
        fi

        if $QEMU $QEMU_FLAGS "$bin_file" "$expected" 2>"$TMPDIR/stderr.txt"; then
            echo "PASS: $filename (expected $expected)"
            passed=$((passed + 1))
        else
            stderr_output=$(cat "$TMPDIR/stderr.txt")
            echo "FAIL: $filename — $stderr_output"
            failed=$((failed + 1))
        fi
    fi
done

echo ""
echo "Results: $passed passed, $failed failed, $errors errors"

if [ "$failed" -gt 0 ] || [ "$errors" -gt 0 ]; then
    exit 1
fi
