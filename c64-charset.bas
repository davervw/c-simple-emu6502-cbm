9 POKE 56333,127
10 POKE 1,PEEK(1) AND (255-4)
11 A=4
12 PRINT CHR$(147);
15 C=0
20 FOR I=0 TO 7
30 FOR J=0 TO A-1
40 V=0:IF C+J < 512 THEN V=PEEK(13*4096+(C+J)*8+I)
50 M=128
60 IF (V AND M) = 0 THEN PRINT " ";
70 IF (V AND M) <> 0 THEN PRINT "*";
75 M=M/2:IF M>=1 THEN 60
80 NEXT J
85 PRINT
90 NEXT I
95 C=C+A
96 IF C<512 THEN 20
100 POKE 1,PEEK(1) OR 4
110 POKE 56333,129
