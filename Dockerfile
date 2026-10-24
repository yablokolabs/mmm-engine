# Portable mmm CLI image: docker run --rm -v "$PWD:/work" ghcr.io/yablokolabs/mmm-engine fit --data history.csv --output model.json
FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ cmake make git ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DMMM_NATIVE=OFF \
        -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc" \
    && cmake --build build -- -j"$(nproc)" \
    && ./build/mmm_tests

FROM ubuntu:24.04
LABEL org.opencontainers.image.source="https://github.com/yablokolabs/mmm-engine" \
      org.opencontainers.image.description="Deterministic marketing mix modelling CLI: fit, optimize, forecast" \
      org.opencontainers.image.licenses="MIT"
COPY --from=build /src/build/mmm /usr/local/bin/mmm
WORKDIR /work
ENTRYPOINT ["mmm"]
CMD ["help"]
