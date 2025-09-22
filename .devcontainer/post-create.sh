#!/bin/bash
WORKSPACE=$(pwd)

# conda activate maa

echo "===================="
echo "Setting up git safe.directory for $WORKSPACE and its submodules..."
cd "$WORKSPACE"
git config --global --add safe.directory "$WORKSPACE"
git submodule foreach --recursive 'git config --global --add safe.directory "$toplevel/$path"'

echo "===================="
cd "$WORKSPACE"
echo "Installing dependencies for python..."
# pip install -r tools/.../requirements.txt
# pip install -r tools/.../requirements-dev.txt

echo "===================="
echo "Installing dependencies for nodejs..."
cd "$WORKSPACE"/docs
pnpm install --frozen-lockfile
