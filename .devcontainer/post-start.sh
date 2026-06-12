echo "whoami: $(whoami)"
echo "pwd: $(pwd)"

# echo "Starting vnc server (TigerVNC)..."
# tigervncserver :1 -geometry 1280x800 -depth 24 -localhost -SecurityTypes None --I-KNOW-THIS-IS-INSECURE
# sleep 1
# sudo ss -tulpn | grep :5901

# echo "Starting vnc web proxy (noVNC)..."
# command -v novnc_proxy
# ls -l $(command -v novnc_proxy)
# nohup novnc_proxy --listen "localhost:6080" --vnc "localhost:5901" >> "$HOME/novnc_proxy.log" 2>&1 < /dev/null &
# sleep 1
# sudo ss -tulpn | grep :6080

# pnpm run start
# DISPLAY=:1 chromium --no-sandbox --disable-dev-shm-usage --disable-gpu --app=http://localhost:3000
