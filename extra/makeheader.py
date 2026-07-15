from os import listdir
header = "#ifndef LIBLABDEV_H\n#define LIBLABDEV_H\n"
h = ['#include "labdev/%s"'%(_h) for _h in listdir("../build/include/labdev") if ".h" in _h]
H = "\n".join(h)

header+=H

header+="\n#endif"

print(header)

with open("../build/include/labdev.h","w") as f:
    f.write(header)
