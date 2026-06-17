python _bin_mkromfs.py --binary --ictype 87x3D root root.bin
..\..\prepend_header.exe /user_data1   root.bin   /ic_type 87x3D
..\..\prepend_header.exe /user_data1   root.bin  /mp_ini  mp_data1.ini  /ic_type 87x3D
..\..\md5.exe   root_MP.bin
copy "resource.h" "..\..\..\..\src\sample\bt_audio_trx\panel\user_data\" /y
pause

