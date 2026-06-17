python _bin_mkromfs_0x4400000.py --binary --addr 0x256E400 root root.bin
..\..\..\..\..\tool\Gadgets\prepend_header.exe /user_data1   root.bin
..\..\..\..\..\tool\Gadgets\prepend_header.exe /user_data1   root.bin  /mp_ini  mp_data1.ini
..\..\..\..\..\tool\Gadgets\md5.exe   root_MP.bin

