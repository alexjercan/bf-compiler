#!/bin/bash

PROJECT=$1

# Set the paths to the folders
SRC_FOLDER="examples/src"
IN_FOLDER="examples/in"
OUT_FOLDER="examples/out"
ERR_FOLDER="examples/err"

# Build the compiler
make -s -C $PROJECT clean && make -s -C $PROJECT

# Set the paths to your compiler
COMPILER="./$PROJECT/main"

# Function to compare two files
compare_files() {
    diff -q "$1" "$2" >/dev/null
    return $?
}

# Function to execute a test
execute_test() {
    local test_name="$1"
    local src_file="$SRC_FOLDER/$test_name.bf"
    local in_file="$IN_FOLDER/$test_name.txt"
    local out_file="$OUT_FOLDER/$test_name.txt"
    local err_file="$ERR_FOLDER/$test_name.txt"

    # Compile the source file and redirect stdout/stderr to temp files
    temp_asm=$(mktemp)
    temp_err=$(mktemp)
    $COMPILER -f "$src_file" -o "$temp_asm" 2> "$temp_err"

    # Check error file
    if [ -s "$err_file" ]; then
        expected_err=$(<"$err_file")
        compare_files "$temp_err" <(echo "$expected_err")
        if [ $? -ne 0 ]; then
            echo "$test_name.bf ... fail (incorrect error message)"
            rm -f "$temp_asm" "$temp_err"
            return 1
        fi

        # Check if the assembly file is not generated and pass because the
        # error message is correct
        if [ ! -s "$temp_asm" ]; then
            echo "$test_name.bf ... pass"
            rm -f "$temp_asm" "$temp_err"
            return 0
        fi
    fi

    temp_obj=$(mktemp)
    nasm -felf64 -g "$temp_asm" -o "$temp_obj"

    # Check assembly file
    if [ $? -ne 0 ]; then
        echo "$test_name.bf ... fail (assembly error)"
        rm -f "$temp_asm" "$temp_err" "$temp_obj"
        return 1
    fi

    temp_elf=$(mktemp)
    ld "$temp_obj" -o "$temp_elf"

    # Check linker file
    if [ $? -ne 0 ]; then
        echo "$test_name.bf ... fail (linker error)"
        rm -f "$temp_asm" "$temp_err" "$temp_obj" "$temp_elf"
        return 1
    fi

    # Run the compiled executable with input file
    temp_out=$(mktemp)
    "$temp_elf" <"$in_file" >"$temp_out"

    # Check return code
    if [ $? -ne 0 ]; then
        echo "$test_name.bf ... fail (runtime error)"
        rm -f "$temp_asm" "$temp_err" "$temp_obj" "$temp_elf" "$temp_out"
        return 1
    fi

    # Check output file
    if [ -s "$out_file" ]; then
        compare_files "$temp_out" "$out_file"
        if [ $? -ne 0 ]; then
            echo "$test_name.bf ... fail (incorrect output)"
            rm -f "$temp_asm" "$temp_err" "$temp_obj" "$temp_elf" "$temp_out"
            return 1
        fi
    fi

    echo "$test_name.bf ... pass"
    rm -f "$temp_asm" "$temp_err" "$temp_obj" "$temp_elf" "$temp_out"
    return 0
}


# Initialize counters
total_tests=0
pass_count=0

# Display total score
echo -e "=========================== Brainf**k Compiler ===========================\n"

# Iterate over each test file in src folder
for src_file in "$SRC_FOLDER"/*.bf; do
    test_name=$(basename "$src_file" .bf)
    execute_test "$test_name"
    if [ $? -eq 0 ]; then
        pass_count=$((pass_count + 1))
    fi
    total_tests=$((total_tests + 1))
done

echo -e "Total: $pass_count/$total_tests"

make -s -C $PROJECT clean

if [ $pass_count -eq $total_tests ]; then
    exit 0
else
    exit 1
fi
