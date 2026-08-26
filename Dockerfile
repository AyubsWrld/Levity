FROM ubuntu:24.04

# Prevent interactive prompts during build
ENV DEBIAN_FRONTEND=noninteractive

# 1. Install C++ Toolchain, IPC Dependencies, and System Utilities
RUN apt-get update && apt-get install -y \
        build-essential \
        cmake \
        gdb \
        ninja-build \
        clang-tidy \
        clang-format \
        pkg-config \
        git \
        curl \
        wget \
        tar \
        xz-utils \
        libzmq3-dev \
        libprotobuf-dev \
        protobuf-compiler \
        python3 \
        python3-pip \
        nodejs \
        npm \
        ccache \
        openssh-server \
    && rm -rf /var/lib/apt/lists/*

# 2. Install GitHub CLI (For Copilot) and AI Tools
# NodeJS Installation (Always installed for CLI agents like Copilot)
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && \
    apt-get install -y --no-install-recommends nodejs && \
    rm -rf /var/lib/apt/lists/*

# Install Gemini CLI (community Node package) and legacy Copilot NPM if needed
RUN npm install -g @google/gemini-cli @github/copilot

# Conan 2 is the only third-party dependency manager for this repository.
RUN python3 -m pip install --no-cache-dir --break-system-packages --ignore-installed distro "conan==2.31.2"
WORKDIR /deps
COPY conanfile.py ./
RUN conan profile detect --force \
    && conan install . --build=missing -s build_type=Release -s compiler.cppstd=23

# 3. Setup ZeroMQ IPC Directory
RUN mkdir -p /tmp/edge-ipc && chmod 777 /tmp/edge-ipc

# 4. Keep the container alive so the IDE can connect
CMD ["sleep", "infinity"]