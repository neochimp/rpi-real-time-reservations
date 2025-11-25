sudo mount /dev/sdb2 mnt
make proj3/modules/res/testMod.c
cp -r proj3 mnt/home/pi/Documents/
sudo umount mnt
