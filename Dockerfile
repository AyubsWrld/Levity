FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    gdb \
    openssh-server \
    rsync \
    git \
    python3 \
    python3-pip \
    python3-venv \
    pkg-config \
    patchelf \
    binutils \
    elfutils \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages "conan>=2.0"

RUN mkdir /var/run/sshd && \
    echo 'root:root' | chpasswd && \
    sed -i 's/#*PermitRootLogin .*/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' /etc/pam.d/sshd

EXPOSE 22

WORKDIR /work

CMD ["/usr/sbin/sshd", "-D"]
