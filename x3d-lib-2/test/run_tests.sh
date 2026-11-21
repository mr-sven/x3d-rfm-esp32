#!/usr/bin/env bash

print_red() { printf "\033[31m$1\033[0m"; }
print_grn() { printf "\033[32m$1\033[0m"; }

for test in $@
do
  printf "%-20s " $(basename "$test")

  if output=$($test 2>&1)
  then
    print_grn "-> PASS\n"
  else
    print_red "-> FAIL\n"
    echo "$output"
    exit 0
  fi
done

print_grn "\nALL GOOD\n"
