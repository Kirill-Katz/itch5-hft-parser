

sudo mountpoint -q /dev/hugepages || mount -t hugetlbfs nodev /dev/hugepages
echo 64 | sudo tee /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
sudo cpupower frequency-set -g performance
sudo systemctl stop cron
sudo systemctl stop bluetooth
sudo systemctl stop cups
sudo systemctl stop irqbalance

