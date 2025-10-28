FROM mcr.microsoft.com/devcontainers/base:ubuntu

USER vscode

ENV CONDA_DIR=/home/vscode/miniconda
ENV PATH="$CONDA_DIR/bin:$PATH"

ARG PYTHON_VERSION=3.12
ARG NODEJS_VERSION=24

# Install Miniconda
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O ~/miniconda.sh \
    && bash ~/miniconda.sh -b -p $CONDA_DIR \
    && rm ~/miniconda.sh \
    && conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/main \
    && conda tos accept --override-channels --channel https://repo.anaconda.com/pkgs/r

# Configure conda environment
RUN eval "$(conda shell.bash hook)" \
    && conda create -n maa \
    && conda activate maa \
    && conda install -y \
    python=$PYTHON_VERSION \
    conda-forge::nodejs=$NODEJS_VERSION \
    && pip install pre-commit

# Finalize conda setup
RUN conda init \
    && conda config --set auto_activate false \
    && echo "conda activate maa" >> ~/.bashrc
