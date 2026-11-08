FROM ubuntu:24.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    --no-install-recommends build-essential flex bison clang ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /project
COPY . .
RUN make clean && make test

CMD ["make", "demo"]

