# mutisocket


1. 程式編譯 gcc mutclient.c -o mutclient gcc mutserver.c -o mutserver 
2. 程式執行 
> ./mutserver [group ip] [local ip] [port number] [file name] 

> ./mutclient [group number] [local ip] [port number] 

3. 遺失率計算: (理應接受到的封包數目-接受到封包數目) / 理應接受到的封包數目 

4.  hamming code，首先利用 fwrite 把資料讀進來，再使用 bool array 儲 存成 bit pattern ，之後再對這個 bit pattern 做 hamming code ，因為一個 char 為 8bit ，所以採用 8+4 bit(2^n > 8  &&  k = n+1 , so k = 4)的 hamming code，根據定義我每 12 個 bit 的 1、2、 4、8 位置為加密的位數。並利用 even parity 去做運算。 
> P1 = 1 3 5 7 9 11

> P2 = 2 3 6 7 10 11 

> P3 = 4 5 6 7 12 

> P4 = 8 9 10 11 12 

