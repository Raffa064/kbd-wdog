#!/bin/sh
set -e

echo "=== Compilando ==="
make

echo "=== Instalando binário em /usr/local/bin/ ==="
sudo cp kbd-wdog /usr/local/bin/

echo "=== Instalando script fix em /usr/local/lib/kbd-wdog/ ==="
sudo mkdir -p /usr/local/lib/kbd-wdog
sudo cp fix /usr/local/lib/kbd-wdog/
sudo chmod +x /usr/local/lib/kbd-wdog/fix

echo "=== Instalando unit systemd ==="
sudo cp kbd-wdog.service /etc/systemd/system/

echo "=== Recarregando systemd ==="
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

echo "=== Ativando serviço ==="
sudo systemctl enable --now kbd-wdog

echo "=== Pronto! ==="
sudo systemctl status kbd-wdog --no-pager
