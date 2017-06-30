#!/bin/sh
install -Dm644 ./NeoUdp.service /usr/lib/systemd/system/NeoUdp.service
mkdir -p /etc/systemd/system/NeoUdp.service.d/
systemctl daemon-reload
systemctl enable NeoUdp.service
