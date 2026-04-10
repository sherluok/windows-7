# https://docs.docker.com/reference/dockerfile/
# https://docs.docker.com/reference/cli/docker/buildx/build/

# https://github.com/devcontainers/images/tree/main/src/base-debian
FROM mcr.microsoft.com/devcontainers/base:trixie

ARG NODE_VERSION=24.14.1
ARG NODE_ARCH=x64

RUN <<EOF
apt-get update
apt-get install -y --no-install-recommends ca-certificates curl xz-utils
rm -rf /var/lib/apt/lists/*
EOF

# Install Node.js.
RUN <<EOF
curl -fsSL "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-${NODE_ARCH}.tar.xz" | tar -xJ -C /usr/local --strip-components=1 --no-same-owner
node --version
EOF

# Enable corepack.
RUN <<EOF
corepack enable
EOF

CMD ["sleep", "infinity"]
