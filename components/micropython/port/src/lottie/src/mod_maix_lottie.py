import lcd, time
from Maix import lottie
import KPU

import os
import sensor, image, time, lcd, json
import gc, sys

#lcd.init(freq=15000000, type=2, invert=True, offset_w0=0, offset_h0=0, offset_w1=0, offset_h1=0, width=240, height=240, rst=37, dcx=38, ss=36, clk=39)
#lcd.rotation(2)

#lcd.init(freq=15000000)
#lcd.rotation(2)

#base_path = '' # flash
base_path = '/sd/test'
names = os.listdir(base_path)
print(names)
json_files = [name for name in names if name.lower().endswith('.json')]
print(json_files)
lottie.init()
while len(json_files):
    for p in json_files:
        with open(base_path + '/' + p) as f:
            tmp = f.read()
            lottie.load(tmp)
        print(p)
        total = lottie.total()
        KPU.memtest()
        for i in range(0, 2):
            for f in range(total):
                tmp = time.ticks_ms()
                lottie.view(f)
                print(time.ticks_ms() - tmp)
                #time.sleep_ms(1000//fps)
