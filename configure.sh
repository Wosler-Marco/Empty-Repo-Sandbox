#!/usr/bin/env bash

ENABLE_DEBUG=OFF
BUILD_TESTS=OFF
BUILD_UNIT_TESTS=OFF
REMOVE_BUILD=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -d|--debug)
            ENABLE_DEBUG=ON
            shift
            ;;
        -t|--test)
            BUILD_TESTS=ON
            shift
            ;;
        -u|--unittest)
            BUILD_UNIT_TESTS=ON
            shift
            ;;
        -r|--remove)
            REMOVE_BUILD=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ "$REMOVE_BUILD" == true ]; then
    rm -f -r ./build
fi
cmake -S . -B ./build -DENABLE_DEBUG=$ENABLE_DEBUG -DBUILD_TESTS=$BUILD_TESTS -DBUILD_UNIT_TESTS=$BUILD_UNIT_TESTS
