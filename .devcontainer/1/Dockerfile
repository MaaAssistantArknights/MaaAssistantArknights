FROM mcr.microsoft.com/devcontainers/base:ubuntu

USER vscode

ENV PATH="/home/vscode/.local/bin:${PATH}"

ARG CLANGD_VERSION=20
ARG PYTHON_VERSION=3.12.11
ARG NODEJS_VERSION=24

# Add kitware's apt repository (for cmake)
RUN curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc | sudo gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg
RUN cat <<EOF | sudo tee /etc/apt/sources.list.d/kitware.sources
Types: deb
URIs: https://apt.kitware.com/ubuntu
Suites: $(lsb_release -cs)
Components: main
Architectures: $(dpkg --print-architecture)
Signed-By: /usr/share/keyrings/kitware-archive-keyring.gpg
EOF

# Install system dependencies
RUN sudo apt update \
    && sudo apt upgrade -y \
    && sudo apt install -y \
    cmake \
    fish \
    && sudo rm -rf /var/lib/apt/lists/*

# Initialize fish
RUN fish -c exit

# Install and setup mise and uv
RUN curl https://mise.run | sh
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Activate mise
RUN echo 'eval "$(mise activate bash)"' >> ~/.bashrc
RUN echo 'mise activate fish | source' >> ~/.config/fish/config.fish

# Setup Python and Node.js
RUN mise use -g node@$NODEJS_VERSION
RUN uv python install $PYTHON_VERSION

# Create and activate venv
RUN uv venv --clear ~/.venv/maa
RUN echo "source ~/.venv/maa/bin/activate" >> ~/.bashrc
RUN echo "source ~/.venv/maa/bin/activate.fish" >> ~/.config/fish/config.fish

# Install tools
RUN uv tool install prek
RUN uv tool install ruff
