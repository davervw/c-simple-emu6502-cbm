10 PRINT "Think of a number between 1 and 100"
15 L=1:H=100:G=0
20 FOR I=1 TO 100000:NEXT
30 X=INT(RND(1)*(H+1-L)+L)
40 PRINT "Is your number "X" ?"
45 G=G+1
50 PRINT "(Y)es (H)igher (L)ower ";
60 A$="":INPUT A$
70 IF A$="Y" THEN PRINT "I'm so smart!!!! I took "G" guesses" : GOTO 10
80 IF A$="H" THEN L=X+1: GOTO 20
90 IF A$="L" THEN H=X-1: GOTO 20
95 IF A$="Q" THEN END
100 PRINT "MUST ANSWER Y, H, OR L"
110 GOTO 60

