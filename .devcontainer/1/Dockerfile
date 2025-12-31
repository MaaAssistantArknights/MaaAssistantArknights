FROM mcr.microsoft.com/devcontainers/base:ubuntu

USER vscode

ENV PATH="/home/vscode/.local/bin:${PATH}"

ARG CLANGD_VERSION=20
ARG PYTHON_VERSION=3.12.11
ARG NODEJS_VERSION=24

# Install system dependencies
RUN sudo apt update \
    && sudo apt upgrade -y \
    && sudo apt install -y \
    cmake fish

# Install and setup Mise and uv
RUN curl https://mise.run | sh
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Activate Mise
RUN echo 'eval "$(mise activate bash)"' >> ~/.bashrc
RUN echo 'mise activate fish | source' >> ~/.config/fish/config.fish

# Setup Python and NodeJS
RUN mise use -g node@$NODEJS_VERSION
RUN uv python install $PYTHON_VERSION

# Install tools
RUN uv tool install prek ruff
