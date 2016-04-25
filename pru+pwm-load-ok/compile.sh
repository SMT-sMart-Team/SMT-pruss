# !/bin/bash

date
echo "==================================================================================="
echo "start execute $0"
make -f Makefile.gnu
# scp out/pwm-pru1.elf root@192.168.7.2:/root/1_HW.elf

echo "end execute $0"
echo "==================================================================================="
