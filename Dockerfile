FROM registry.hub.docker.com/riftember/llvm-for-spfie:11.0.0

RUN apt-get -y install \
    git \
    libfl-dev \
    texinfo \
    libtool

WORKDIR /

COPY . /spf-ie

RUN mkdir /spf-ie/build && \
    cd /spf-ie/build && \
    cmake -DLLVM_SRC=/llvm-project .. && \
    cmake --build .

WORKDIR /spf-ie
