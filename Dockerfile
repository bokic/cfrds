# syntax=docker/dockerfile:1

# ---------------------------------------------------------------------------
# Stage 1: build the cfrds CLI and the shared libraries.
# ---------------------------------------------------------------------------
FROM alpine:3.21 AS build

ARG CFRDS_VERSION=""

RUN apk add --no-cache \
        bash \
        build-base \
        cmake \
        git \
        json-c-dev \
        libxml2-dev \
        ninja \
        pkgconf \
        python3

WORKDIR /src

COPY . .

# Pass an explicit CFRDS_VERSION (e.g. from a git tag) when available.
# Otherwise the build falls back to `git describe --tags --abbrev=0`.
RUN if [ -n "$CFRDS_VERSION" ]; then export CFRDS_VERSION="$CFRDS_VERSION"; fi \
    && BUILD_TESTS=OFF ./build.sh

# ---------------------------------------------------------------------------
# Stage 2: minimal runtime image with just the executables, shared libraries
# and their runtime dependencies.
# ---------------------------------------------------------------------------
FROM alpine:3.21 AS runtime

ARG CFRDS_VERSION=""

LABEL org.opencontainers.image.title="cfrds" \
      org.opencontainers.image.description="ColdFusion RDS protocol library and CLI application" \
      org.opencontainers.image.source="https://github.com/bokic/cfrds" \
      org.opencontainers.image.licenses="MIT"

RUN apk add --no-cache \
        json-c \
        libxml2

COPY --from=build /src/bin/cfrds /usr/local/bin/cfrds
COPY --from=build /src/bin/libcfrds.so* /usr/local/lib/

# The executables are built with a build-tree RPATH pointing to the build
# output directory; LD_LIBRARY_PATH lets the loader find the cfrds
# shared libraries at runtime.
ENV LD_LIBRARY_PATH=/usr/local/lib

ENTRYPOINT ["/usr/local/bin/cfrds"]
