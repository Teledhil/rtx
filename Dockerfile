FROM ubuntu:jammy

# Use local mirror to get packages faster
ARG APT_PROXY="None"
RUN if [ "${APT_PROXY}" != "None" ]; then \
  echo "Acquire::http { Proxy \"http://${APT_PROXY}:3142\"; };" >> /etc/apt/apt.conf.d/02-apt-proxy; \
fi

ENV DEBIAN_FRONTEND noninteractive
RUN sed -i '/deb-src/s/^# //' /etc/apt/sources.list \
 && apt update \
 && apt install -y \
     clang \
     clang-tools \
     cmake \
     curl \
     git \
     gnupg2 \
     llvm \
     libgl-dev \
     libx11-dev \
     libxcursor-dev \
     libxi-dev \
     libxinerama-dev \
     libxrandr-dev \
     ninja-build \
 && rm -rf /var/lib/apt/lists/*

ENV VULKAN_SDK_VERSION 1.3.243
RUN curl https://packages.lunarg.com/lunarg-signing-key-pub.asc \
     -o /etc/apt/trusted.gpg.d/lunarg-signing-key-pub.asc \
 && curl https://packages.lunarg.com/vulkan/${VULKAN_SDK_VERSION}/lunarg-vulkan-${VULKAN_SDK_VERSION}-jammy.list \
     -o /etc/apt/sources.list.d/lunarg-vulkan-${VULKAN_SDK_VERSION}-jammy.list \
 && apt update \
 && apt install -y vulkan-sdk \
 && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 100
RUN update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 100

# Fix git submodule update failing
RUN git config --global --add safe.directory /src
