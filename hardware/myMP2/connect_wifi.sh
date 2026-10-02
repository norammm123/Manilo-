#!/bin/sh
SSID="gkywifi"
PSK="12345678"
INTERFACE="wlan0"
echo "[1] Set reg domain"
iw reg set CN 2>/dev/null
echo "[2] Up $INTERFACE"
ip link set "$INTERFACE" up
sleep 1
echo "[3] Kill old"
killall wpa_supplicant 2>/dev/null
killall udhcpc 2>/dev/null
sleep 1
echo "[4] Connect..."
wpa_supplicant -B -D nl80211 -i "$INTERFACE" -c /etc/wpa_supplicant.conf
sleep 3
echo "[5] Link status"
iw dev "$INTERFACE" link
echo "[6] DHCP"
udhcpc -i "$INTERFACE" -t 10 -n 2>/dev/null || dhclient "$INTERFACE" 2>/dev/null
echo "[7] IP"
ip addr show "$INTERFACE" | grep inet
IP=$(ip addr show "$INTERFACE" | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
if [ -n "$IP" ]; then
    echo "Done!"
else
    echo "WiFi failed"
fi
