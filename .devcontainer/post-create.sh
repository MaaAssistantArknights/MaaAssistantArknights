#!/bin/bash
WORKSPACE=$(pwd)

echo "===================="
cd "$WORKSPACE"
echo "Setting up git safe.directory for $WORKSPACE and its submodules..."
git config --global --add safe.directory "$WORKSPACE"
git submodule foreach --recursive 'git config --global --add safe.directory "$toplevel/$path"'
