# homebrew
## list of homebrew programs made by me

## INSTALLATION
- 3DS [devkit pro](https://devkitpro.org/)
- - add repo `https://pkg.devkitpro.org/packages` and `https://pkg.devkitpro.org/packages/linux/$arch/`
- - `sudo pacman -S 3ds-dev`

## setting up ftp
- `pcmanfm ftp://192.168.31.166:5000` #ip address of 3ds. <br>
But doing this all the time manually is tiring. So, I follow a different method. <br>
- mount ftp server as a file system
`sudo pacman -S curlftpfs` <br>
`mkdir -p ~/ftp_mount` <br>
`curlftpfs ftp://bhu1:password@192.168.31.166:5000 ~/ftp_mount` enter your password there <br>

## nds [devkitpro]
- [x] console
- [x] basic 2d and 3d
- [x] synthwave library clone
- [ ] doom-like

## psp [pspdsk]
- [x] console print (partial)

## psp [luaplayer++]
- [x] basic 2d (partial)

## 3ds [[devkitpro]](https://github.com/bhu1-103/homebrew/tree/main/3ds)
- [x] [hello world](https://github.com/bhu1-103/homebrew/tree/main/3ds/hello_world)
- [ ] basic 2d and 3d
      
# vita 

## vitasdk

### [camera](https://github.com/bhu1-103/homebrew/tree/main/vita/camera)

### godot


welcome the new member of the family, switch lite
i guess not
