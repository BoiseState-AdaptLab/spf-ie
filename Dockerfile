FROM riftember/llvm-for-spfie:11.0.0

RUN apt-get -y install \
    git \
    texinfo \
    libtool \
    libfl-dev \
    byacc \
    bison

WORKDIR /

COPY . /spf-ie

RUN mkdir /spf-ie/build && \
    cd /spf-ie/build && \
    cmake -DLLVM_SRC=/llvm-project .. && \
    cmake --build .

WORKDIR /spf-ie
