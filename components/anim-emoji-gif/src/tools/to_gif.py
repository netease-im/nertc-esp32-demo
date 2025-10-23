# python3 to_gif.py anger.c anger.gif
import re, sys, pathlib
txt = pathlib.Path(sys.argv[1]).read_text()
hex_str = re.findall(r'0x[0-9a-fA-F]{2}', txt)
data = bytes(int(x,16) for x in hex_str)
pathlib.Path(sys.argv[2]).write_bytes(data)