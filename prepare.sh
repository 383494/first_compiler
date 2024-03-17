#!/bin/bash

rm build -r
cmake -DCMAKE_BUILD_TYPE=Debug -B build
