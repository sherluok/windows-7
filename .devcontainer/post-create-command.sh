whoami
pwd

git config --global --add safe.directory /workspace

# sudo chown vscode /home/vscode/.local/share/pnpm/store
sudo chown vscode /workspace # https://code.visualstudio.com/docs/sourcecontrol/faq#_why-is-vs-code-warning-me-that-the-git-repository-is-potentially-unsafe
sudo chown vscode /workspace/.pnpm-store
sudo chown vscode /workspace/node_modules
corepack install
# pnpm config set store-dir /home/vscode/.local/share/pnpm/store
pnpm install
