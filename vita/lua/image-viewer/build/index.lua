imageFile = nil
x = 0
y = 0

function StartGame()
    imageFile = Graphics.loadImage("app0:/image/1.jpg")
end

function UpdateGame()
    pad = Controls.read()
    if Controls.check(pad,SCE_CTRL_UP) then
        y = y - 1
    end
    if Controls.check(pad,SCE_CTRL_DOWN) then
        y = y + 1
    end
    if Controls.check(pad,SCE_CTRL_LEFT) then
        x = x - 1
    end
    if Controls.check(pad,SCE_CTRL_RIGHT) then
        x = x + 1
    end
end

function DrawGame()
    Graphics.initBlend()
    Screen.clear()
    Graphics.drawImage(x,y,imageFile)
    Graphics.termBlend()
    Screen.flip()
end

StartGame()

while (true) do
    UpdateGame()
    DrawGame()
end
