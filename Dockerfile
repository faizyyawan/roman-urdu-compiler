FROM archlinux:latest

RUN pacman -Syu --noconfirm --needed \
    base-devel flex bison clang ca-certificates \
    && pacman -Scc --noconfirm

WORKDIR /project
COPY . .
RUN make clean && make test

CMD ["make", "demo"]

