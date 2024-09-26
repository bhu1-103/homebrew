# homebrew
list of homebrew programs made by me

## setting up ftp
- `pcmanfm ftp://192.168.31.166:5000` #ip address of 3ds. <br>
But doing this all the time manually is tiring. So, I follow a different method. <br>
- mount ftp server as a file system
`sudo pacman -S curlftpfs`
`mkdir -p ~/ftp_mount`
`curlftpfs ftp://bhu1:password@192.168.31.166:5000 ~/ftp_mount` enter your password there


## nds [devkitpro]
[x] console
[x] basic 2d and 3d
[x] synthwave library clone
[ ] doom-like

## psp [pspdsk]
[x] console (partial)

## psp [luaplayer++]
[x] basic 2d (partial)

## 3ds [maybe devkitpro]
[ ] console
[ ] basic 2d and 3d

will upload source files soon...
