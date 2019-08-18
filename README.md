# mutisocket


1. 程式編譯 gcc mutclient.c -o mutclient gcc mutserver.c -o mutserver 
2. 程式執行 
> ./mutserver [group ip] [local ip] [port number] [file name] 

> ./mutclient [group number] [local ip] [port number] 

3. 遺失率計算: (理應接受到的封包數目-接受到封包數目) / 理應接受到的封包數目 
