#!/bin/bash

set -ex

sshpass -p admin ssh 192.168.33.38 mkdir -p /boot/opencloudos/
sshpass -p admin rsync -avz release/Image-6.6.119-kdev 192.168.33.38:/boot/opencloudos/
sshpass -p admin rsync -avz release/rk3588-bdy-g98.dtb 192.168.33.38:/boot/opencloudos/
sshpass -p admin rsync -avz -P --delete kos/lib/modules/6.6.119-kdev 192.168.33.38:/lib/modules/





