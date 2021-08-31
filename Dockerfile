FROM riftember/llvm-for-spfie:11.0.0

RUN apt-get -y install \
    git \
    libfl-dev \
    texinfo \
    libtool

ENV TERM=xterm-256color

WORKDIR /

RUN git clone https://github.com/BoiseState-AdaptLab/spf-ie

RUN mkdir /spf-ie/build && \
    cd /spf-ie/build && \
    cmake -DLLVM_SRC=/llvm-project .. && \
    cmake --build .

WORKDIR /spf-ie
