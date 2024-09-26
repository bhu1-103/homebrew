#!/usr/bin/zsh

mkdir -p ~/ftp_mount
curlftpfs ftp://username:password@192.168.31.166:5000 ~/ftp_mount
cd ~/ftp_mount
ls
fusermount -u ~/ftp_mount
