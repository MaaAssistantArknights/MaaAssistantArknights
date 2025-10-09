#!/bin/bash
WORKSPACE=$(pwd)

# conda activate maa

echo "===================="
cd "$WORKSPACE"
echo "Setting up git safe.directory for $WORKSPACE and its submodules..."
git config --global --add safe.directory "$WORKSPACE"
git submodule foreach --recursive 'git config --global --add safe.directory "$toplevel/$path"'

echo "===================="
cd "$WORKSPACE"/docs
echo "Installing node modules..."
npm install -g pnpm
pnpm install --frozen-lockfile
