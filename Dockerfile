FROM ubuntu:24.04

RUN apt update && apt upgrade -y
RUN apt install -y libssl-dev gcc-13 g++-13 libspdlog-dev libfmt-dev llvm-14-dev libedit-dev libcxxopts-dev libpfm4-dev ninja-build libpcap-dev libopenblas-pthread-dev libibverbs-dev librdmacm-dev curl

WORKDIR /opt
RUN curl -fsSL http://108.181.27.85/wasi/wasi-sdk-21.0-linux.tar.gz | tar Cxz /opt
RUN mv wasi-sdk-21 wasi-sdk

RUN apt install -y build-essential
RUN apt install -y cmake
RUN apt install -y python3-pip python3-numpy
RUN apt install -y git
RUN apt install -y python3-matplotlib
RUN apt install -y wabt
RUN apt install -y vim netcat-openbsd
RUN apt install -y tcpdump iproute2

RUN mkdir /app
COPY . /app

WORKDIR /app/lib/wasm-micro-runtime/samples/socket-api/
RUN cmake . -DCMAKE_C_COMPILER_WORK=1 -DCMAKE_CXX_COMPILER_WORK=1 && make -j

WORKDIR /app/lib/wasm-micro-runtime/wamr-compiler
RUN mkdir build && cd build && \
	cmake .. -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_AOT_STACK_FRAME=1 -DWAMR_BUILD_DEBUG_AOT=0 -DWAMR_BUILD_DUMP_CALL_STACK=1 -DWAMR_BUILD_CUSTOM_NAME_SECTION=1 -DWAMR_BUILD_SHARED_MEMORY=1 -DWAMR_BUILD_WITH_CUSTOM_LLVM=1 -DWAMR_BUILD_LOAD_CUSTOM_SECTION=1 -DLLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm/ && make -j

WORKDIR /app
RUN touch /usr/lib/llvm-14/lib/libMLIRSupportIndentedOstream.a && CC=gcc-13 CXX=g++-13 cmake -S /app -B /app/build -DCMAKE_BUILD_TYPE=Debug -DMVVM_BUILD_TEST=1 -DLLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm/

WORKDIR /app/build
RUN cmake --build . --config Debug -- -j


