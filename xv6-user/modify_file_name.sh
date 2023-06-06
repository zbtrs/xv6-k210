#!/bin/bash

for file in test_*; do
    if [[ -f "$file" ]]; then
        mv "$file" "_$file"
    fi
done

