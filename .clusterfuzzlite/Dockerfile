FROM ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest AS LITE_BUILDER

# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1

# Install additional package dependencies.
RUN apt update && apt install -y ninja-build zip

# Copy the project's source code.
COPY . $SRC/app-xrp
COPY --from=LITE_BUILDER /opt/ledger-secure-sdk $SRC/app-xrp/BOLOS_SDK

# Working directory for build.sh
WORKDIR $SRC/app-xrp

# Copy build.sh into $SRC dir.
COPY ./.clusterfuzzlite/build.sh $SRC/
