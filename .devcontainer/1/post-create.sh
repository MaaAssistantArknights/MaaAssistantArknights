#!/bin/bash
WORKSPACE=$(pwd)

# conda activate maa

echo "===================="
cd "$WORKSPACE"
echo "Setting up git safe.directory..."
git config --global --add safe.directory "$WORKSPACE"
git submodule foreach --recursive 'git config --global --add safe.directory "$toplevel/$path"'

echo "===================="
cd "$WORKSPACE"
echo "Updating submodules..."
git submodule update --init --recursive

echo "===================="
cd "$WORKSPACE"/docs
echo "Installing node modules..."
npm install -g pnpm
pnpm install --frozen-lockfile

echo "===================="
cd "$WORKSPACE"
echo "Installing Python dependencies..."
# Install Python dependencies from all tools
for req_file in tools/*/requirements.txt; do
    if [ -f "$req_file" ]; then
        echo "Installing from $req_file"
        pip install -r "$req_file"
    fi
done

for req_file in tools/*/requirements-dev.txt; do
    if [ -f "$req_file" ]; then
        echo "Installing from $req_file"
        pip install -r "$req_file"
    fi
done

echo "===================="
cd "$WORKSPACE"
echo "Installing MaaDeps..."
python tools/maadeps-download.py
# Link clang-format & clangd to /usr/local/bin for easy access
sudo ln -s $WORKSPACE/src/MaaUtils/MaaDeps/x-tools/llvm/bin/clang-format /usr/local/bin/clang-format
sudo ln -s $WORKSPACE/src/MaaUtils/MaaDeps/x-tools/llvm/bin/clangd /usr/local/bin/clangd
