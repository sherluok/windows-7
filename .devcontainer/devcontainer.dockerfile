# https://docs.docker.com/reference/dockerfile/
# https://docs.docker.com/reference/cli/docker/buildx/build/

# https://github.com/devcontainers/images/tree/main/src/base-debian
FROM mcr.microsoft.com/devcontainers/base:trixie

ARG NODE_VERSION=24.14.1
ARG NODE_ARCH=x64
ARG NOVNC_VERSION=1.7.0
ARG WEBSOCKIFY_VERSION=0.13.0

RUN <<EOF
apt-get update
apt-get install -y --no-install-recommends \
	ca-certificates curl xz-utils \
	x11-xserver-utils x11-utils x11-apps dbus-x11 xterm xauth \
	tigervnc-standalone-server tigervnc-common \
	fluxbox xdg-utils fbautostart \
	xfonts-base fonts-dejavu fonts-noto fonts-noto-cjk fonts-wqy-microhei \
	chromium chromium-sandbox libnss3 libxss1 libasound2 \
	python3-minimal python3-numpy
rm -rf /var/lib/apt/lists/*
EOF

# 安装 Node.js
RUN <<EOF
curl -fsSL "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-${NODE_ARCH}.tar.xz" | tar -xJ -C /usr/local --strip-components=1 --no-same-owner
node --version
corepack enable
EOF

# 安装 noVNC
RUN <<EOF
mkdir -p /usr/local/novnc

curl -fsSL "https://github.com/novnc/noVNC/archive/v${NOVNC_VERSION}.zip" -o /tmp/novnc.zip
unzip /tmp/novnc.zip -d /usr/local/novnc
cp "/usr/local/novnc/noVNC-${NOVNC_VERSION}/vnc_lite.html" "/usr/local/novnc/noVNC-${NOVNC_VERSION}/index.html"
rm -f /tmp/novnc.zip

curl -fsSL "https://github.com/novnc/websockify/archive/v${WEBSOCKIFY_VERSION}.zip" -o /tmp/websockify.zip
unzip /tmp/websockify.zip -d /usr/local/novnc
ln -s "/usr/local/novnc/websockify-${WEBSOCKIFY_VERSION}" "/usr/local/novnc/noVNC-${NOVNC_VERSION}/utils/websockify"
rm -f /tmp/websockify.zip

ln -s "/usr/local/novnc/noVNC-${NOVNC_VERSION}/utils/novnc_proxy" "/usr/local/bin"
EOF

USER vscode

# 配置 TigerVNC
RUN <<EOF
touch /home/vscode/.Xauthority
chmod 600 /home/vscode/.Xauthority

mkdir -p /home/vscode/.config/tigervnc
cat > /home/vscode/.config/tigervnc/xstartup <<-'EOF'
	#!/bin/sh
	unset SESSION_MANAGER
	unset DBUS_SESSION_BUS_ADDRESS
	exec fluxbox
	EOF
chmod +x /home/vscode/.config/tigervnc/xstartup
EOF

# 配置 fluxbox
RUN mkdir -p /home/vscode/.fluxbox
COPY fluxbox-apps.txt /home/vscode/.fluxbox/apps

CMD ["sleep", "infinity"]
