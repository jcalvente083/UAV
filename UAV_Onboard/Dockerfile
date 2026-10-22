# ---------- FASE 1: compilación ----------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

ARG ONNXRUNTIME_VERSION=1.17.1
ARG TARGETARCH

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake wget ca-certificates libopencv-dev \
    && rm -rf /var/lib/apt/lists/*

RUN if [ "$TARGETARCH" = "amd64" ]; then ORT_ARCH=x64; \
    elif [ "$TARGETARCH" = "arm64" ]; then ORT_ARCH=aarch64; \
    else echo "Arquitectura no soportada: $TARGETARCH" && exit 1; fi && \
    wget -q "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION}.tgz" && \
    tar -xzf "onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION}.tgz" && \
    mv "onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION}" /opt/onnxruntime && \
    rm "onnxruntime-linux-${ORT_ARCH}-${ONNXRUNTIME_VERSION}.tgz"

WORKDIR /src
COPY . .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -DORT_DIR=/opt/onnxruntime && \
    cmake --build build -j"$(nproc)"

# ---------- FASE 2: ejecución ----------
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Etc/UTC

RUN apt-get update && apt-get install -y --no-install-recommends \
    libopencv-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/UAV_Onboard .
COPY --from=builder /opt/onnxruntime/lib/*.so* /usr/local/lib/
COPY --from=builder /src/Models ./Models
COPY --from=builder /src/config.json .

RUN ldconfig

CMD ["./UAV_Onboard"]