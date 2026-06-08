#!/bin/sh
set -e

echo "=== Parando e desabilitando serviço ==="
sudo systemctl disable --now kbd-wdog 2>/dev/null || true

echo "=== Removendo binário ==="
sudo rm -f /usr/local/bin/kbd-wdog

echo "=== Removendo unit systemd ==="
sudo rm -f /etc/systemd/system/kbd-wdog.service

echo "=== Removendo script fix ==="
sudo rm -rf /usr/local/lib/kbd-wdog

echo "=== Recarregando systemd ==="
sudo systemctl daemon-reload

echo "=== Desinstalação concluída ==="
