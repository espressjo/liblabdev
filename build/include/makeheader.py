from os import listdir
header = "#ifndef LIBLABDEV_H\n#define LIBLABDEV_H\n"
h = ['#include "labdev/%s"'%(_h) for _h in listdir("./labdev") if ".h" in _h]
H = "\n".join(h)

header+=H

header+="\n#endif"

print(header)

with open("labdev.h","w") as f:
    f.write(header)
