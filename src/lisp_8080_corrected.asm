PG6 ASSM 0 8000

0000                0001 OUTC    EQU 0F009H
0000                0002 MEM     EQU 700H    START ADDRESS OF FREE MEMORY
0000                0003 FREEBLK EQU 64      #256 BYTE BLOCKS
0000                0004 INCH    EQU 0F006H
0000                0005 CRLF    EQU 0F021H
0000                0006 ABEND   EQU 0F000H  MONITOR REENTRY
0000                0007
0000                0008 *
0000                0009 *
0000 C3 B6 04       0010        JMP  START
0003 CD 10 00       0011 CADDR  CALL CDR
0006 CD 10 00       0012 CADR   CALL CDR
0009 F5             0013 CAR    PUSH PSW     SAVE A,F
000A 7E             0014 CAR2   MOV  A,M     GET CAR/CDR PTR
000B 23             0015        INX  H
000C 66             0016        MOV  H,M     HL:=CAR(HL)
000D 6F             0017        MOV  L,A
000E F1             0018        POP  PSW
000F C9             0019        RET
0010 F5             0020 CDR    PUSH PSW
0011 23             0021        INX  H        SKIP CAR PTR
0012 23             0022        INX  H        FOR HL:=CDR(HL)
0013 C3 0A 00       0023        JMP  CAR2
0016 D5             0024 CONS   PUSH PSW     SAVE NEW CDR VALUE
0017 E5             0025        PUSH H       SAVE NEW CAR VALUE
0018 11 04 00       0026        LXI  D,4      NEED 4 BYTES
001B CD AD 05       0027        CALL GMEM    GET MEMORY
001E D1             0028        POP  D       CAR VALUE
001F 73             0029        MOV  M,E      INTO LINK
0020 23             0030        INX  H
0021 72             0031        MOV  M,D
0022 23             0032        INX  H
0023 D1             0033        POP  D       CDR VALUE
0024 73             0034        MOV  M,E


0025 23             0035        INX  H
0026 72             0036        MOV  M,D
0027 2B             0037        DCX  H        HL:=CONS(HL,DE)
0028 2B             0038        DCX  H
0029 2B             0039        DCX  H
002A C9             0040        RET
002B 7C             0041 ATOM   MOV  A,H      ATOM(HL)
002C E6 80          0042        ANI  80H      TEST HIGH ORDER BIT
002E FE 80          0043        CPI  80H      ATOM IF BIT SET
0030 C9             0044        RET           Z-FLAG IS TRUE
0031 7C             0045 EQ     MOV  A,H      EQ(HL,DE)
0032 BA             0046        CMP  D        EQUAL IF SAME POINTER
0033 C0             0047        RNZ
0034 7D             0048        MOV  A,L
0035 BB             0049        CMP  E
0036 C9             0050        RET           Z-FLAG SET IF TRUE
0037 E5             0051 ISLIST PUSH H        SAVE. ISLIST(HL)
0038 CD 2B 00       0052 TEST   CALL ATOM     LOOK FOR ATOMIC
003B CA 44 00       0053        JZ   EOL      END OF LIST?
003E CD 10 00       0054        CALL CDR      TRUE IF C(D**N)=NIL
0041 C3 38 00       0055        JMP  TEST
0044 CD 49 00       0056 EOL    CALL NULL     NIL IS ATOM
0047 E1             0057        POP  H
0048 C9             0058        RET           Z-FLAG SET IF TRUE
0049 3E 80          0059 NULL   MVI  A,80H    NULL(HL) IF PTR IS 8000H
004B BC             0060        CMP  H
004C C0             0061        RNZ           NZ=FALSE
004D AF             0062        XRA  A        A=0
004E BD             0063        CMP  L
004F C9             0064        RET
0050 E5             0065 OUTPUT PUSH H
0051 CD 49 00       0066        CALL NULL
0054 C2 5D 00       0067        JNZ  NNL
0057 21 69 04       0068        LXI  H,NILAT  PRINT 'NIL'
005A C3 63 00       0069        JMP  PAT      GO PRINT ATOM
005D CD 2B 00       0070 NNL    CALL ATOM     OUTPUT AN ATOM?
0060 C2 75 00       0071        JNZ  NAT
0063 C5             0072 PAT    PUSH B        SAVE
0064 7C             0073        MOV  A,H
0065 E6 7F          0074        ANI  7FH      CLEAR ATOM FLAG
0067 67             0075        MOV  H,A
0068 46             0076        MOV  B,M      ATOM STRING LENGTH
0069 23             0077 ALP    INX  H
006A 7E             0078        MOV  A,M      GET CHAR
006B CD 09 F0       0079        CALL OUTC     PRINT CHAR
006E 05             0080        DCR  B
006F C2 69 00       0081        JNZ  ALP
0072 C1             0082        POP  B
0073 E1             0083        POP  H
0074 C9             0084        RET
0075 3E 28          0085 NAT    MVI  A,'('    BEGIN LIST SEP
0077 CD 09 F0       0086        CALL OUTC
007A CD 37 00       0087        CALL ISLIST   LIST OR SEXP?
007D C2 99 00       0088        JNZ  SEXP
0080 E5             0089 NIS    PUSH H        SAVE CURRENT PTR
0081 CD 09 00       0090        CALL CAR
0084 CD 50 00       0091        CALL OUTPUT   RECURSIVE CALL FOR LEFT SUBT
0087 E1             0092        POP  H        CURRENT
0088 CD 10 00       0093        CALL CDR      NEXT LIST ELEMENT
008B CD 49 00       0094        CALL NULL     END OF LIST?
008E CA AC 00       0095        JZ   CLOSE    YES, FINISH
0091 3E 20          0096        MVI  A,' '    NO, SEPARATOR
0093 CD 09 F0       0097        CALL OUTC
0096 C3 80 00       0098        JMP  NIS
0099 CD 09 00       0099 SEXP   CALL CAR
009C CD 50 00       0100        CALL OUTPUT   RECURSIVE
009F 3E 2E          0101        MVI  A,'.'    SEXP SEPARATOR
00A1 CD 09 F0       0102        CALL OUTC
00A4 E1             0103        POP  H        REVIVE PARAM
00A5 E5             0104        PUSH H        AND SAVE
00A6 CD 10 00       0105        CALL CDR      RIGHT HALF
00A9 CD 50 00       0106        CALL OUTPUT   RECURSIVE
00AC 3E 29          0107 CLOSE  MVI  A,')'
00AE CD 09 F0       0108        CALL OUTC
00B1 E1             0109        POP  H

00B2 C9             0110        RET
00B3 CD 2B 00       0111 EQUAL  CALL ATOM     EQUAL(HL;DE)
00B6 EB             0112        XCHG          Z-FLAG SET IF EQUAL
00B7 C2 C3 00       0113        JNZ  XNA
00BA CD 2B 00       0114        CALL ATOM
00BD EB             0115        XCHG
00BE C0             0116        RNZ           NOT BOTH ATOMS, NONEQ
00BF CD 31 00       0117        CALL EQ       BOTH ATOMS, USE EQ
00C2 C9             0118        RET
00C3 CD 2B 00       0119 XNA    CALL ATOM
00C6 EB             0120        XCHG
00C7 CA E8 00       0121        JZ   SETNZ    NOT BOTH ATOMS
00CA E5             0122        PUSH H        SAVE ROOTS
00CB D5             0123        PUSH D
00CC CD 09 00       0124        CALL CAR
00CF EB             0125        XCHG
00D0 CD 09 00       0126        CALL CAR
00D3 CD B3 00       0127        CALL EQUAL    RECURSIVE ON SUBTREES
00D6 D1             0128        POP  D
00D7 E1             0129        POP  H
00D8 C0             0130        RNZ           BOTH SUBTREES MUST MATCH
00D9 E5             0131        PUSH H
00DA D5             0132        PUSH D
00DB CD 10 00       0133        CALL CDR
00DE EB             0134        XCHG
00DF CD 10 00       0135        CALL CDR
00E2 CD B3 00       0136        CALL EQUAL    RECURSIVE
00E5 D1             0137        POP  D
00E6 E1             0138        POP  H
00E7 C9             0139        RET           WITH RESULTS OF LAST CALL
00E8 F6 01          0140 SETNZ  ORI  1        SET NZ FLAG
00EA C9             0141        RET
00EB CD 49 00       0142 PAIRLIS CALL NULL    PAIRLIS(HL;DE;BC)
00EE C2 F4 00       0143        JNZ  FULC     PAIRLIS(X,Y,A)
00F1 60             0144        MOV  H,B
00F2 69             0145        MOV  L,C
00F3 C9             0146        RET           X=NULL SO RETURN A
00F4 D5             0147 FULC   PUSH D        SAVE Y
00F5 E5             0148        PUSH H
00F6 D5             0149        PUSH D
00F7 CD 09 00       0150        CALL CAR      (X)
00FA EB             0151        XCHG
00FB CD 09 00       0152        CALL CAR      (Y)
00FE EB             0153        XCHG
00FF CD 16 00       0154        CALL CONS     (CAR(X),CAR(Y))
0102 D1             0155        POP  D        RESTORE Y
0103 E3             0156        XTHL          SAVE CONS, GET X
0104 CD 10 00       0157        CALL CDR      (X)
0107 EB             0158        XCHG
0108 CD 10 00       0159        CALL CDR      (Y)
010B EB             0160        XCHG
010C CD EB 00       0161        CALL PAIRLIS  (CDR(X),CDR(Y),A) RECURSIVE
010F D1             0162        POP  D        FROM CONS
0110 EB             0163        XCHG
0111 CD 16 00       0164        CALL CONS     (CONS(...),PAIRLIS(...))
0114 D1             0165        POP  D        RESTORE Y
0115 C9             0166        RET
0116 D5             0167 ASSOC  PUSH D        (HL,DE) OR (X,A)
0117 D5             0168 ASSO2  PUSH D        SAVE A
0118 EB             0169        XCHG
0119 CD 49 00       0170        CALL NULL
011C C2 22 01       0171        JNZ  ASOGO
011F E1             0172        POP  H        NULL ALIST, QUIT
0120 D1             0173        POP  D
0121 C9             0174        RET           RESULT=NIL
0122 CD 09 00       0175 ASOGO  CALL CAR      (A)
0125 CD 09 00       0176        CALL CAR      (CAR(A))
0128 CD B3 00       0177        CALL EQUAL    (CAR(...),X)
012B C2 34 01       0178        JNZ  NOTYET
012E E1             0179        POP  H        A FROM STACK
012F D1             0180        POP  D
0130 CD 09 00       0181        CALL CAR      (A)
0133 C9             0182        RET           VALUE ASSOC WITH A
0134 E1             0183 NOTYET POP  H
0135 CD 10 00       0184        CALL CDR      DOWN ASSOC LIST X

0136 EB             0185        XCHG
0137 C3 17 01       0186        JMP  ASSO2    =CALL,RET
013C CD 2B 00       0187 EVAL   CALL ATOM     (E) (HL,DE) IS (E,A)
013F C2 53 01       0188        JNZ  NOTAT
0142 CD 49 00       0189        CALL NULL
0145 C8             0190        RZ            VALUE(NIL)=NIL
0146 CD 16 01       0191        CALL ASSOC    (E,A)
0149 CD 49 00       0192        CALL NULL
014C CA 00 F0       0193        JZ   ABEND    ERROR, NOT DEFINED
014F CD 10 00       0194        CALL CDR      (ASSOC(...))
0152 C9             0195        RET
0153 E5             0196 NOTAT  PUSH H        SAVE E
0154 CD 09 00       0197        CALL CAR      (E)
0157 CD 2B 00       0198        CALL ATOM     (CAR(E))
015A C2 00 01       0199        JNZ  NOTAA1
015D D5             0200        PUSH D        SAVE A
015E 11 61 84       0201        LXI  D,QUOTEA1+ATOMIC
0161 CD 31 00       0202        CALL EQ       (CAR(E),'QUOTE')
0164 C2 6D 01       0203        JNZ  NOTQ
0167 D1             0204        POP  D        A PROCESS 'QUOTE'
0168 E1             0205        POP  H        E
0169 CD 06 00       0206        CALL CADR     (E)
016C C9             0207        RET
016D 11 3E 84       0208 NOTQ   LXI  D,CONDA1+ATOMIC
0170 CD 31 00       0209        CALL EQ       (CAR(E),'COND')
0173 C2 7F 01       0210        JNZ  NOTCD
0176 D1             0211        POP  D        A-PROCESS 'COND'
0177 E1             0212        POP  H        E
0178 CD 10 00       0213        CALL CDR      (E)
017B CD 49 02       0214        CALL EVCON    (CDR(E),A)
017E C9             0215        RET
017F D1             0216 NOTCD  POP  D        A
0180 E3             0217 NOTAA1 XTHL          SAVE CAR(E)
0181 CD 10 00       0218        CALL CDR      (E)
0184 CD 75 02       0219        CALL EVLIS    (CDR(E),A)
0187 EB             0220        XCHG
0188 78             0221        MOV  A,B      SWAP BC,HL
0189 44             0222        MOV  B,H
018A 67             0223        MOV  H,A
018B 79             0224        MOV  A,C
018C 4D             0225        MOV  C,L
018D 6F             0226        MOV  L,A
018E E3             0227        XTHL          SAVE BC
018F CD 96 01       0228        CALL APPLY    (CAR(E),EVLIS(...),A)
0192 50             0229        MOV  D,B      RESTORE A
0193 59             0230        MOV  E,C
0194 C1             0231        POP  B
0195 C9             0232        RET
0196 CD 2B 00       0233 APPLY  CALL ATOM     (FN) (FN;X;A) IS (HL;DE;BC)
0199 C2 F9 01       0234        JNZ  APNA
019C D5             0235        PUSH D        SAVE X
019D E5             0236        PUSH H        SAVE FN
019E 11 37 06       0237        LXI  D,ASMFN  ASSOC LIST OF ASSEMBLED
01A1 CD 16 01       0238        CALL ASSOC    FUNCTIONS
01A4 CD 49 00       0239        CALL NULL     ON LIST?
01A7 CA EF 01       0240        JZ   NOTDEF   NO, GO USE EVAL
01AA F1             0241        POP  PSW      DISCARD FN
01AB CD 10 00       0242        CALL CDR      FUNCTION DATA
01AE 54             0243        MOV  D,H      DE=HL
01AF 5D             0244        MOV  E,L
01B0 CD 10 00       0245        CALL CDR      GET FN FLAGS
01B3 EB             0246        XCHG
01B4 CD 09 00       0247        CALL CAR      GET FN ADDR
01B7 7B             0248        MOV  A,E      FN FLAGS
01B8 D1             0249        POP  D        GET X
01B9 C5             0250        PUSH B        SAVE APPLY ARGS
01BA D5             0251        PUSH D
01BB 01 EC 01       0252        LXI  B,APRET  RETURN POINT
01BE A7             0253        ANA  A        TEST T/F VS LIST FN RETURN
01BF F2 C5 01       0254        JP   WBLIST   RETURNS LISTS
01C2 01 E3 01       0255        LXI  B,APTFR  RETURNS Z/NZ FOR T/F
01C5 C5             0256 WBLIST PUSH B        SET FN RETURN POINT
01C6 E5             0257        PUSH H        SET FN CALL POINT
01C7 62             0258        MOV  H,D      HL=X
01C8 6B             0259        MOV  L,E
01C9 E6 07          0260        ANI  A,3      ISOLATE #ARGS

01CB C8             0261        RZ            NO ARGS, CALL FN WITH HL=X
01CC FE 02          0262        CPI  2        HOW MANY ARGS
01CE CA DB 01       0263        JZ   ARG2
01D1 DA DF 01       0264        JC   ARG1     ONLY ONE
01D4 CD 03 00       0265        CALL CADDR    THREE ARGS, BC=3RD
01D7 44             0266        MOV  B,H
01D8 4D             0267        MOV  C,L
01D9 62             0268        MOV  H,D      HL=X
01DA 6B             0269        MOV  L,E
01DB CD 06 00       0270 ARG2   CALL CADR
01DE EB             0271        XCHG          DE=2ND ARG
01DF CD 09 00       0272 ARG1   CALL CAR      HL=1ST ARG
01E2 C9             0273        RET           CALL FUNCTION
01E3 21 67 84       0274 APTFR  LXI  H,TAT+ATOMIC ASSUME TRUE
01E6 CA EC 01       0275        JZ   APRET    RIGHT
01E9 21 52 84       0276        LXI  H,FAT+ATOMIC NO, FALSE
01EC D1             0277 APRET  POP  D        RESTORE REGS
01ED C1             0278        POP  B
01EE C9             0279        RET
01EF E1             0280 NOTDEF POP  H        GET FN
01F0 50             0281        MOV  D,B      DE:=A
01F1 59             0282        MOV  E,C
01F2 CD 3C 01       0283        CALL EVAL     (FN,A)
01F5 D1             0284        POP  D        RESTORE X
01F6 C3 96 01       0285        JMP  APPLY    (EVAL(...),X,A)
01F9 E5             0287 APNA   PUSH H        SAVE FN
01FA CD 09 00       0288        CALL CAR      (FN)
01FD D5             0289        PUSH D        SAVE X
01FE 11 5A 84       0290        LXI  D,LAMBDAA1+ATOMIC
0201 CD 31 00       0291        CALL EQ       (CAR(FN),'LAMBDA')
0204 C2 1A 02       0292        JNZ  NOTLAM
0207 D1             0293        POP  D        X
0208 E1             0294        POP  H        FN
0209 E5             0295        PUSH H
020A CD 06 00       0296        CALL CADR     (FN)
020D CD EB 00       0297        CALL PAIRLIS  (CADR(FN),X,A)
0210 EB             0298        XCHG
0211 E3             0299        XTHL          SAVE X, GET FN
0212 CD 03 00       0300        CALL CADDR    (FN)
0215 CD 3C 01       0301        CALL EVAL     (CADDR(FN)),PAIRLIS(...)
0218 D1             0302        POP  D
0219 C9             0303        RET
021A 11 54 84       0304 NOTLAM LXI  D,LABELAT+ATOMIC
021D CD 31 00       0305        CALL EQ       (CAR(FN),'LABEL')
0220 C2 44 02       0306        JNZ  NOTLB
0223 D1             0307        POP  D        X
0224 E1             0308        POP  H        FN
0225 E5             0309        PUSH H
0226 CD 03 00       0310        CALL CADDR    (FN)
0229 E3             0311        XTHL          GET FN, SAVE CADDR(FN)
022A CD 06 00       0312        CALL CADR     (FN)
022D EB             0313        XCHG
022E E3             0314        XTHL          X ONTO STACK
022F EB             0315        XCHG
0230 CD 16 00       0316        CALL CONS     (CADR(FN),CADDR(FN))
0233 D5             0317        PUSH D        SAVE CADDR(FN)
0234 50             0318        MOV  D,B      DE:=A
0235 59             0319        MOV  E,C
0236 CD 16 00       0320        CALL CONS     (CONS(...),A)
0239 44             0321        MOV  B,H      BC:=CONS(...)
023A 4D             0322        MOV  C,L
023B E1             0323        POP  H        CADDR(FN)
023C EB             0324        XCHG
023D E3             0325        XTHL          SAVE A, GET X
023E EB             0326        XCHG
023F CD 96 01       0327        CALL APPLY    (CADDR(FN),X,CONS(...))
0242 C1             0328        POP  B        A RESTORED
0243 C9             0329        RET
0244 3E FF          0330 NOTLB  MVI  A,0FFH
0246 CD 00 F0       0331        CALL ABEND    ERROR IN LISP PROGRAM
0249 CD 49 00       0332 EVCON  CALL NULL     (C,A) IS (HL,DE)
024C CA 1A 02       0333        JZ   NOTLAB   ERROR IF C=NULL
024F E5             0334        PUSH H        SAVE C
0250 CD 09 00       0335        CALL CAR      (C)
0253 CD 09 00       0336        CALL CAR      (CAR(C))

0256 CD 3C 01       0337        CALL EVAL     (CAAR(C),A)
0259 D5             0338        PUSH D        SAVE A
025A 11 67 84       0339        LXI  D,TAT+ATOMIC
025D CD B3 00       0340        CALL EQUAL    (EVAL(...),'T')
0260 D1             0341        POP  D        A
0261 E1             0342        POP  H        C
0262 C2 6F 02       0343        JNZ  CALEC
0265 CD 09 00       0344        CALL CAR      (C)
0268 CD 06 00       0345        CALL CADR     (CAR(C))
026B CD 3C 01       0346        CALL EVAL     (CADAR(C),A)
026E C9             0347        RET
026F CD 10 00       0348 CALEC  CALL CDR      (C)
0272 C3 49 02       0349        JMP  EVCON    (CDR(C),A) -(CALL;RET)
0275 CD 49 00       0350 EVLIS  CALL NULL     (M,A) IS (HL,DE)
0278 C8             0351        RZ            EVLIS(NULL;A)=NULL
0279 E5             0352        PUSH H        SAVE M
027A CD 09 00       0353        CALL CAR      (M)
027D CD 3C 01       0354        CALL EVAL     (CAR(M))
0280 E3             0355        XTHL          SAVE EVAL, GET M
0281 CD 10 00       0356        CALL CDR      (M)
0284 CD 75 02       0357        CALL EVLIS    (CDR(M)) RECURSIVE
0287 EB             0358        XCHG
0288 E3             0359        XTHL          SAVE A, GET EVAL
0289 CD 16 00       0360        CALL CONS     (EVAL(CAR(M)),EVLIS(CDR(M)))
028C D1             0361        POP  D
028D C9             0362        RET
028E E5             0363 EVALQUOTE PUSH H
028F 2A B4 04       0364        LHLD EVQUAL
0292 44             0365        MOV  B,H
0293 4D             0366        MOV  C,L
0294 E1             0367        POP  H
0295 CD 96 01       0368        CALL APPLY    (FN=HL,X=DE,NULL)
0298 CD 21 F0       0369        CALL CRLF
029B 3E             0370        MVI  A,'>'
029D CD 09 F0       0371        CALL OUTC
02A0 CD 09 F0       0372        CALL OUTC
02A3 CD 50 00       0373        CALL OUTPUT
02A6 C9             0374        RET
02A7 D5             0375 MAKEATOM PUSH D
02A8 C5             0376        PUSH B
02A9 F5             0377        PUSH PSW
02AA EB             0378        XCHG          D->INPUT STRING
02AB 21 69 04       0379        LXI  H,NILAT  'NIL'
02AE CD 25 03       0380        CALL ACOMP
02B1 C2 BB 02       0381        JNZ  TREES    STRING NOT 'NIL'
02B4 21 00 80       0382        LXI  H,8000H  NIL POINTER
02B7 F1             0383 MAKEOUT POP  PSW
02B8 C1             0384        POP  B
02B9 D1             0385        POP  D
02BA C9             0386        RET
02BB 2A 2F 04       0387 TREES  LHLD NAMETREE
02BE CD C4 02       0388        CALL AN       SEARCH KNOWN ATOM TREE
02C1 C3 B7 02       0389        JMP  MAKEOUT
02C4 44             0390 AN     MOV  B,H      BC->TREE (SAVE)
02C5 4D             0391        MOV  C,L
02C6 CD 09 00       0392        CALL CAR      TO ATOM
02C9 CD 25 03       0393        CALL ACOMP
02CC C8             0394        RZ            FOUND IDENTICAL ATOM
02CD 60             0395        MOV  H,B      RESTORE HL
02CE 69             0396        MOV  L,C
02CF CD 10 00       0397        CALL CDR
02D2 F5             0398        PUSH PSW      SAVE FLAGS
02D3 CD 49 00       0399        CALL NULL     END OF TREE?
02D6 C2 E7 02       0400        JNZ  ISLR     JUMP IF MORE
02D9 D5             0401        PUSH D        SAVE
02DA 54             0402        MOV  D,H      DE:=NIL
02DB 5D             0403        MOV  E,L
02DC CD 16 00       0404        CALL CONS     (NIL;NIL) ADDING TO TREE
02DF D1             0405        POP  D
02E0 03             0406        INX  B        INSERT INTO EXISTING TREE
02E1 03             0407        INX  B
02E2 7D             0408        MOV  A,L      CHANGE LINK
02E3 02             0409        STAX B
02E4 03             0410        INX  B
02E5 7C             0411        MOV  A,H

02E6 02             0412        STAX B
02E7 F1             0413 ISLR   POP  PSW      FLAGS
02E8 DA ED 02       0414        JC   ISLEFT   JUMP IF NEW LESS
02EB 23             0415        INX  H        GO RIGHT, INCR TO
02EC 23             0416        INX  H        OTHER PTR
02ED 44             0417 ISLEFT MOV  B,H      SAVE NEW TREE ROOT
02EE 4D             0418        MOV  C,L
02EF CD 09 00       0419        CALL CAR      FOLLOW HL
02F2 CD 49 00       0420        CALL NULL
02F5 C2 C4 02       0421        JNZ  AN       IF NOT NULL(=CNZ,RNZ)
02F8 D5             0422        PUSH D
02F9 1A             0423        LDAX D        GET NAME LENGTH
02FA C6 04          0424        ADI  4        1 FOR LENGTH FIELD,3 TO CEIL
02FC E6 FC          0425        ANI  0FCH     DOWN TO 4*N
02FE 5F             0426        MOV  E,A
02FF 16 00          0427        MVI  D,0      DE=BYTES NEEDED
0301 CD AD 05       0428        CALL GMEM
0304 11 00 80       0429        LXI  D,8000H  NIL
0307 E5             0430        PUSH H        SAVE NEW ATOM
0308 19             0431        DAD  D        SET ATOMIC
0309 CD 16 00       0432        CALL CONS     (NEW ATOM,NIL)
030C 7D             0433        MOV  A,L
030D 02             0434        STAX B        CONNECT TO CURRENT TREE
030E 03             0435        INX  B
030F 7C             0436        MOV  A,H
0310 02             0437        STAX B
0311 E1             0438        POP  H
0312 D1             0439        POP  D
0313 E5             0440        PUSH H        SAVE FOR RETURN
0314 1A             0441        LDAX D
0315 47             0442        MOV  B,A      LENGTH COUNT
0316 77             0443        MOV  M,A
0317 23             0444 COPYAT INX  H        COPY ATOM TEXT TO ALLOC AREA
0318 13             0445        INX  D
0319 1A             0446        LDAX D
031A 77             0447        MOV  M,A
031B 05             0448        DCR  B
031C C2 17 03       0449        JNZ  COPYAT
031F E1             0450        POP  H
0320 7C             0451        MOV  A,H
0321 F6 80          0452        ORI  80H      SET ATOMIC FLAG
0323 67             0453        MOV  H,A
0324 C9             0454        RET
0325 E5             0455 ACOMP  PUSH H        SAVE REGS
0326 D5             0456        PUSH D
0327 C5             0457        PUSH B
0328 7C             0458        MOV  A,H
0329 E6 7F          0459        ANI  7FH      NO ATOM FLAG
032B 67             0460        MOV  H,A
032C 1A             0461        LDAX D        COUNT 2ND
032D 4F             0462        MOV  C,A
032E 46             0463        MOV  B,M      COUNT OF 1ST
032F 23             0464 COMPLP INX  H
0330 13             0465        INX  D
0331 1A             0466        LDAX D
0332 96             0467        SUB  M        COMPARE CHARS
0333 C2 40 03       0468        JNZ  NEQL
0336 05             0469        DCR  B
0337 CA 44 03       0470        JZ   DONE1
033A 0D             0471        DCR  C
033B C2 2F 03       0472        JNZ  COMPLP
033E B0             0473        ORA  B        SET NZ
033F 37             0474        STC           HL LONGER, SET '>'
0340 C1             0475 NEQL   POP  B
0341 D1             0476        POP  D
0342 E1             0477        POP  H        RESTORE
0343 C9             0478        RET
0344 0D             0479 DONE1  DCR  C
0345 C3 40 03       0480        JMP  NEQL     SET FLAG BY RESIDUAL
0348 CD 06 F0       0481 NEXTCHAR CALL INCH
034B FE 20          0482        CPI  ' '
034D DA 48 03       0483        JC   NEXTCHAR IGNORE CONTROL CHAR
0350 32 9C 04       0484        STA  CURCH
0353 C9             0485        RET
0354 CD C4 03       0486 INPUT  CALL TAKELP   Z=TRUE GET '(' IF AVAIL

0357 C2 9C 03       0487        JNZ  S139     JUMP IF NOT THERE
035A CD D7 03       0488        CALL TAKERP   GET ')' IF THERE
035D C2 64 03       0489        JNZ  S127     JUMP IF NOT THERE
0360 21 00 80       0490        LXI  H,8000H  NIL IS ()
0363 C9             0491        RET
0364 D5             0492 S127   PUSH D
0365 CD 54 03       0493        CALL INPUT    RECURSIVE CALL
0368 EB             0494        XCHG          SAVE 'INPUT'
0369 CD CF 03       0495        CALL TAKEDOT  TRY FOR '.' (SEXP)
036C C2 75 03       0496        JNZ  S130     JUMP IF LIST NOTATION
036F CD 54 03       0497        CALL INPUT    GET CDR SIDE
0372 C3 90 03       0498        JMP  S135     GO GET ')'
0375 CD ED 03       0499 S130   CALL TAKEBL   LIST:GET SEP ' ' OR ','
0378 C2 81 03       0500        JNZ  S132     NO SEPARATOR
037B CD A9 03       0501        CALL INPTL    GET REST OF LIST
037E C3 90 03       0502        JMP  S135     THEN ')'
0381 CD DF 03       0503 S132   CALL CTRP     SEE IF ')', BUT DON'T TAKE
0384 C2 8D 03       0504        JNZ  S134
0387 21 00 80       0505        LXI  H,8000H  NIL
038A C3 90 03       0506        JMP  S135
038D CD A9 03       0507 S134   CALL INPTL
0390 CD D7 03       0508 S135   CALL TAKERP   TAKE ')'
0393 C2 44 02       0509 BADIN  EQU  NOTLB
0393 C2 44 02       0510        JNZ  BADIN    ILLEGAL STRING
0396 EB             0511        XCHG
0397 CD 16 00       0512        CALL CONS
039A D1             0513        POP  D
039B C9             0514        RET
039C CD E5 03       0515 S139   CALL CTATOM
039F C2 44 02       0516        JNZ  BADIN
03A2 CD 11 04       0517        CALL TAKEATOM PUT ATOM TEXT IN ATTXT
03A5 CD A7 02       0518        CALL MAKEATOM GET POINTER FOR IT
03A8 C9             0519        RET
03A9 CD DF 03       0520 INPTL  CALL CTRP
03AC C2 B3 03       0521        JNZ  S144
03AF 21 00 80       0522        LXI  H,8000H  NIL
03B2 C9             0523        RET
03B3 D5             0524 S144   PUSH D
03B4 CD 54 03       0525        CALL INPUT
03B7 EB             0526        XCHG
03B8 CD ED 03       0527        CALL TAKEBL
03BB CD A9 03       0528        CALL INPTL
03BE EB             0529        XCHG
03BF CD 16 00       0530        CALL CONS
03C2 D1             0531        POP  D
03C3 C9             0532        RET
03C4 3A 9C 04       0533 TAKELP LDA  CURCH
03C7 FE 28          0534        CPI  '('
03C9 C0             0535 TAKANY RNZ
03CA CD 48 03       0536        CALL NEXTCHAR
03CD AF             0537        XRA  A        SET Z-FLAG
03CE C9             0538        RET
03CF 3A 9C 04       0539 TAKEDOT LDA  CURCH
03D2 FE 2E          0540        CPI  '.'
03D4 C3 C9 03       0541        JMP  TAKANY
03D7 3A 9C 04       0542 TAKERP LDA  CURCH
03DA FE 29          0543        CPI  ')'
03DC C3 C9 03       0544        JMP  TAKANY
03DF 3A 9C 04       0545 CTRP   LDA  CURCH
03E2 FE 29          0546        CPI  ')'
03E4 C9             0547        RET
03E5 3A 9C 04       0548 CTATOM LDA  CURCH
03E8 FE 30          0549        CPI  '0'
03EA D8             0550        RC            (NZ SET FOR NOT ATOM)
03EB AF             0551        XRA  A        SET Z-FLAG
03EC C9             0552        RET
03ED 3A 9C 04       0553 TAKEBL LDA  CURCH
03F0 FE 20          0554        CPI  ' '
03F2 CA FC 03       0555        JZ   TAKING
03F5 DA FC 03       0556        JC   TAKING   CTRL OK
03F8 AF             0557        XRA  A        SET Z-FLAG
03F9 FE 2C          0558        CPI  ','
03FB C0             0559        RNZ
03FC CD 48 03       0560 TAKING CALL NEXTCHAR
03FF 3A 9C 04       0561        LDA  CURCH
0402 FE 20          0562        CPI  ' '

0404 CA FC 03       0563        JZ   TAKING
0407 DA FC 03       0564        JC   TAKING
040A FE 2C          0565        CPI  ','
040C CA FC 03       0566        JZ   TAKING
040F AF             0567        XRA  A        SET Z-FLAG (GOT BLANKS)
0410 C9             0568        RET
0411 C5             0569 TAKEATOM PUSH B     SAVE
0412 06 00          0570        MVI  B,0      INITIAL ATOM LENGTH
0414 21 9E 04       0571        LXI  H,ATTXT
0417 CD E5 03       0572 TATL   CALL CTATOM
041A C2 29 04       0573        JNZ  FINISHT
041D 3A 9C 04       0574        LDA  CURCH
0420 77             0575        MOV  M,A
0421 23             0576        INX  H
0422 04             0577        INR  B        CHAR COUNT
0423 CD 48 03       0578        CALL NEXTCHAR
0426 C3 17 04       0579        JMP  TATL
0429 21 9D 04       0580 FINISHT LXI  H,ATL
042C 70             0581        MOV  M,B      SET LENGTH
042D C1             0582        POP  B
042E C9             0583        RET
042F 00 00          0584 ATOMIC EQU 8000H     ATOM PTR FLAG
0431 04             0585 NAMETREE DW 0
0432 41 54          0586 ATOMAT DB 4
0434 4F 4D          0587        DW 'TA'       'ATOM'
0436 03             0588        DW 'MO'
0437 43 41          0589 CARAT  DB 3
0439 52             0590        DW 'AC'       'CAR'
043A 03             0591        DB 'R'
043B 43 44          0592 CDRAT  DB 3
043D 52             0593        DW 'DC'       'CDR'
043E 04             0594        DB 'R'
043F 43 4F          0595 CONDAT DB 4
0441 4E 44          0596        DW 'OC'       'COND'
0443 04             0597        DW 'DN'
0444 43 4F          0598 CONSAT DB 4
0446 4E 53          0599        DW 'OC'       'CONS'
0448 06             0600        DW 'SN'
0449 44 45          0601 DEFINAT DB 6
044B 46 49          0602        DW 'ED'       'DEFINE'
044D 4E 45          0603        DW 'IF'
044F 02             0604        DW 'EN'
0450 45 51          0605 EQAT   DB 2
0452 01             0606        DW 'QE'       'EQ'
0453 46             0607 FAT    DB 1
0454 05             0608        DB 'F'
0455 4C 41          0609 LABELAT DB 5
0457 42 45          0610        DW 'AL'       'LABEL'
0459 4C             0611        DW 'EB'
045A 06             0612        DB 'L'
045B 4C 41          0613 LAMBDAAT DB 6
045D 4D 42          0614        DW 'AL'       'LAMBDA'
045F 44 41          0615        DW 'BM'
0461 05             0616        DW 'AD'
0462 51 55          0617 QUOTEAT DB 5
0464 4F 54          0618        DW 'UO'       'QUOTE'
0466 45             0619        DW 'TO'
0467 01             0620        DB 'E'
0468 54             0621 TAT    DB 1
0469 03             0622        DB 'T'
046A 4E 49          0623 NILAT  DB 3
046C 4C             0624        DW 'IN'       'NIL'
046D 05             0625        DB 'L'
046E 43 41          0626 CADDAT DB 5
0470 44 44          0627        DW 'AC'       'CADDR'
0472 52             0628        DW 'DD'
0473 04             0629        DB 'R'
0474 43 41          0630 CADRAT DB 4
0476 44 52          0631        DW 'AC'       'CADR'
0478 04             0632        DW 'RD'
0479 4E 55          0633 NULLAT DB 4
047B 4C 4C          0634        DW 'UN'       'NULL'
047D 05             0635        DW 'LL'
047E 45 51          0636 EQUALAT DB 5
0480 55 41          0637        DW 'QE'       'EQUAL'
0482                0638        DW 'AU'

0482 4C             0639        DB    'L'
0483 07             0640 PAIRAT DB    7
0484 50 41          0641        DW    'AP'     'PAIRLIS'
0486 49 52          0642        DW    'RI'
0488 4C 49          0643        DW    'IL'
048A 53             0644        DB    'S'
048B 05             0645 ASOCAT DB    5
048C 41 53          0646        DW    'SA'     'ASSOC'
048E 53 4F          0647        DW    'OS'
0490 43             0648        DB    'C'
0491 04             0649 EVALAT DB    4
0492 45 56          0650        DW    'VE'     'EVAL'
0494 41 4C          0651        DW    'LA'
0496 05             0652 APLYAT DB    5
0497 41 50          0653        DW    'PA'     'APPLY'
0499 50 4C          0654        DW    'LP'
049B 59             0655        DB    'Y'
049C 00             0656 CURCH  DB    0
049D 00             0657 ATL    DB    0
049E                0658 ATTXT  DS    16       MAX ATOM LENGTH
04AE 00 00          0659 CVTFQ  DW    0
04B0 00 00          0660 SPSAV  DW    0
04B2 00 00          0661 PREVFQE DW   0
04B4 00 00          0662 EVQAL  DW    0
04B6 21 00 07       0663 START  LXI   H,MEM    START OF FREE MEMORY
04B9 22 AE 04       0664        SHLD  CVTFQ
04BC 36 FF          0665        MVI   M,0FFH   NO NEXT FREE BLOCK
04BE 23             0666        INX   H
04BF 36 FF          0667        MVI   M,0FFH
04C1 23             0668        INX   H
04C2 36 00          0669        MVI   M,0      LENGTH OF FREE AREA
04C4 23             0670        INX   H
04C5 36 40          0671        MVI   M,FREEBLK
04C7 21 00 80       0672        LXI   H,NIL
04CA 22 B4 04       0673        SHLD  EVQAL
04CD 21 67 84       0674        LXI   H,TAT+ATOMIC
04D0 11 00 80       0675        LXI   D,NIL
04D3 CD 16 00       0676        CALL  CONS     ('T',CONS(...))
04D6 E5             0677        PUSH  H
04D7 21 83 84       0678        LXI   H,PAIRAT+ATOMIC
04DA CD 16 00       0679        CALL  CONS
04DD EB             0680        XCHG
04DE CD 16 00       0681        CALL  CONS
04E1 EB             0682        XCHG
04E2 21 78 84       0683        LXI   H,NULLAT+ATOMIC
04E5 CD 16 00       0684        CALL  CONS
04E8 D1             0685        POP   D
04E9 CD 16 00       0686        CALL  CONS
04EC EB             0687        XCHG
04ED 21 61 84       0688        LXI   H,QUOTEAT+ATOMIC
04F0 CD 16 00       0689        CALL  CONS     ('QUOTE',NIL)
04F3 E5             0690        PUSH  H
04F4 21 54 84       0691        LXI   H,LABELAT+ATOMIC
04F7 11 00 80       0692        LXI   D,NIL
04FA CD 16 00       0693        CALL  CONS     THESE LINES
04FD E5             0694        PUSH  H
04FE 21 91 84       0695        LXI   H,EVALAT+ATOMIC
0501 CD 16 00       0696        CALL  CONS
0504 EB             0697        XCHG          START THE
0505 CD 16 00       0698        CALL  CONS
0508 EB             0699        XCHG
0509 21 7D 84       0700        LXI   H,EQULAT+ATOMIC
050C CD 16 00       0701        CALL  CONS
050F D1             0702        POP   D
0510 CD 16 00       0703        CALL  CONS     TREE OF
0513 EB             0704        XCHG          KNOWN ATOMS
0514 21 52 84       0705        LXI   H,FAT+ATOMIC
0517 CD 16 00       0706        CALL  CONS
051A D1             0707        POP   D
051B CD 16 00       0708        CALL  CONS
051E EB             0709        XCHG
051F 21 5A 84       0710        LXI   H,LAMBDAAT+ATOMIC
0522 CD 16 00       0711        CALL  CONS
0525 E5             0712        PUSH  H
0526 21 48 84       0713        LXI   H,DEFINAT+ATOMIC

0529 11 00 80       0714        LXI   D,NIL
052C CD 16 00       0715        CALL  CONS
052F E5             0716        PUSH  H
0530 21 3E 84       0717        LXI   H,CONDAT+ATOMIC
0533 CD 16 00       0718        CALL  CONS
0536 EB             0719        XCHG
0537 CD 16 00       0720        CALL  CONS
053A EB             0721        XCHG
053B 21 3A 84       0722        LXI   H,CDRAT+ATOMIC
053E CD 16 00       0723        CALL  CONS
0541 D1             0724        POP   D
0542 CD 16 00       0725        CALL  CONS
0545 EB             0726        XCHG
0546 21 43 84       0727        LXI   H,CONSAT+ATOMIC
0549 CD 16 00       0728        CALL  CONS
054C E5             0729        PUSH  H
054D 21 6D 84       0730        LXI   H,CADDAT+ATOMIC
0550 11 00 80       0731        LXI   D,NIL
0553 CD 16 00       0732        CALL  CONS
0556 CD 16 00       0733        CALL  CONS
0559 EB             0734        XCHG
055A 21 73 84       0735        LXI   H,CADRAT+ATOMIC
055D CD 16 00       0736        CALL  CONS
0560 E5             0737        PUSH  H
0561 21 8B 84       0738        LXI   H,ASOCAT+ATOMIC
0564 11 00 80       0739        LXI   D,NIL
0567 CD 16 00       0740        CALL  CONS
056A EB             0741        XCHG
056B CD 16 00       0742        CALL  CONS
056E EB             0743        XCHG
056F 21 96 84       0744        LXI   H,APLYAT+ATOMIC
0572 CD 16 00       0745        CALL  CONS
0575 D1             0746        POP   D
0576 CD 16 00       0747        CALL  CONS
0579 EB             0748        XCHG
057A 21 31 84       0749        LXI   H,ATOMAT+ATOMIC
057D CD 16 00       0750        CALL  CONS
0580 D1             0751        POP   D
0581 CD 16 00       0752        CALL  CONS
0584 EB             0753        XCHG
0585 21 36 84       0754        LXI   H,CARAT+ATOMIC
0588 CD 16 00       0755        CALL  CONS
058B D1             0756        POP   D
058C CD 16 00       0757        CALL  CONS
058F EB             0758        XCHG
0590 21 4F 84       0759        LXI   H,EQAT+ATOMIC
0593 CD 16 00       0760        CALL  CONS
0596 22 2F 04       0761        SHLD NAMETREE
0599 CD 48 03       0762 INLOOP CALL NEXTCHAR
059C CD 54 03       0763        CALL INPUT
059F CD ED 03       0764        CALL TAKEBL
05A2 EB             0765        XCHG
05A3 CD 54 03       0766        CALL INPUT
05A6 EB             0767        XCHG
05A7 CD 8E 02       0768        CALL EVALQUOTE
05AA C3 99 05       0769        JMP  INLOOP
05AD F5             0770 GTMEM  PUSH PSW      SAVE REGS
05AE C5             0771        PUSH B
05AF D5             0772        PUSH D        LENGTH REQUESTED
05B0 21 00 00       0773        LXI   H,0
05B3 39             0774        DAD  SP       SAVE PROGRAM SP
05B4 22 B0 04       0775        SHLD SPSAV
05B7 21 AE 04       0776        LXI   H,CVTFQ  TREE BASE LIKE 1ST FQE
05BA 22 B2 04       0777 LP     SHLD PREVFQE  SAVE FOR EXACT CASE
05BD F9             0778        SPHL
05BE E1             0779        POP  H        NEXT FQE
05BF 7C             0780        MOV  A,H      END OF CHAIN?
05C0 A5             0781        ANA  L        (ALL FFFFH PTR)
05C1 3C             0782        INR  A
05C2 CA FC 05       0783        JZ   NOSPAC   NO MEMORY LEFT
05C5 F9             0784        SPHL
05C6 C1             0785        POP  B        SKIP LINK
05C7 C1             0786        POP  B        FQE LENGTH
05C8 78             0787        MOV  A,B      FQE BIG ENOUGH?
05C9 92             0788        SUB  D        BC:=BC-DE

05CA CA D0 05       0789        JZ   HIEQ     JUMP IF LENGTHS CLOSE
05CD DA BA 05       0790        JC   LP       NO, TRY NEXT
05D0 47             0791 HIEQ   MOV  B,A
05D1 79             0792        MOV  A,C
05D2 C2 E9 05       0793        JNZ  BIGH     FQE MUCH BIGGER
05D5 93             0794        SUB  E
05D6 CA F1 05       0795        JZ   EXACT    FQE SIZE=REQ SIZE
05D9 DA BA 05       0796        JC   LP       WON'T FIT
05DC 4F             0797 RETPART MOV  C,A BC NOW EXCESS
05DD C5             0798        PUSH B        CHANGE FQE LENGTH
05DE 09             0799        DAD  B        CALCULATE START OF RETURN AR
05DF EB             0800 EXIT   XCHG          SAVE HL
05E0 2A B0 04       0801        LHLD SPSAV    RESTORE ORIGINAL SP
05E3 F9             0802        SPHL
05E4 E1             0803        POP  H        'DE' TO RESTORE
05E5 C1             0804        POP  B
05E6 F1             0805        POP  PSW
05E7 EB             0806        XCHG          SWAP TO RIGHT REGS
05E8 C9             0807        RET
05E9 93             0808 BIGH   SUB  E        BC:=BC-DE
05EA D2 DC 05       0809        JNC  RETPART  BC SET IF NO BORROW
05ED 05             0810        DCR  B        BORROW
05EE C3 DC 05       0811        JMP  RETPART
05F1 F9             0812 EXACT  SPHL
05F2 C1             0813        POP  B        GET PTR TO NEXT FQE
05F3 2A B2 04       0814        LHLD PREVFQE  CHAIN BACK
05F6 F9             0815        SPHL          TO PREVIOUS
05F7 E1             0816        POP  H        INCREMENT SP 2
05F8 C5             0817        PUSH B        CLOSE CHAIN
05F9 C3 DF 05       0818        JMP  EXIT
05FC C3 00 F0       0819 NOSPAC JMP  ABEND    NO MEMORY, EXECUTION ABORTED
05FF                0820 NIL    EQU  0+ATOMIC
05FF CD 49 00       0821 DEFNE  CALL NULL
0602 C8             0822        RZ
0603 E5             0823        PUSH H        SAVE
0604 CD 10 00       0824        CALL CDR
0607 CD FF 05       0825        CALL DEFNE    RECURSE
060A E3             0826        XTHL
060B CD 16 06       0827        CALL DEF
060E EB             0828        XCHG
060F E3             0829        XTHL          TOP=DEF,H=DEFINE
0610 EB             0830        XCHG
0611 CD 16 00       0831        CALL CONS
0614 D1             0832        POP  D
0615 C9             0833        RET
0616 CD 09 00       0834 DEF    CALL CAR
0619 E5             0835        PUSH H
061A D5             0836        PUSH D
061B 54             0837        MOV  D,H
061C 5D             0838        MOV  E,L      SAVE ARG
061D CD 06 00       0839        CALL CADR     FN DEF
0620 EB             0840        XCHG
0621 CD 09 00       0841        CALL CAR
0624 CD 16 00       0842        CALL CONS     FOR A-LIST
0627 EB             0843        XCHG
0628 2A B4 04       0844        LHLD EVQAL    CURRENT LIST
062B EB             0845        XCHG
062C CD 16 00       0846        CALL CONS
062F 22 B4 04       0847        SHLD EVQAL
0632 D1             0848        POP  D
0633 E1             0849        POP  H
0634 C3 09 00       0850        JMP  CAR      =CALL,RET
0637 6F 06          0851 ASMFN  DW   AF1      ASSOC LIST OF ASSM FNS
0639 3B 06          0852        DW   AF3
063B 73 06          0853        DW   AF4
063D 3F 06          0854        DW   AF6
063F 77 06          0855        DW   AF7
0641 43 06          0856        DW   AF9
0643 7B 06          0857 AF9    DW   AF10
0645 47 06          0858        DW   AF12
0647 7F 06          0859 AF12   DW   AF13
0649 4B 06          0860        DW   AF15
064B 83 06          0861 AF15   DW   AF16

064D 4F 06          0862        DW   AF18
064F 87 06          0863 AF18   DW   AF19
0651 53 06          0864        DW   AF21
0653 8B 06          0865 AF21   DW   AF22
0655 57 06          0866        DW   AF24
0657 8F 06          0867 AF24   DW   AF25
0659 5B 06          0868        DW   AF27
065B 93 06          0869 AF27   DW   AF28
065D 5F 06          0870        DW   AF30
065F 97 06          0871 AF30   DW   AF31
0661 63 06          0872        DW   AF33
0663 9B 06          0873 AF33   DW   AF34
0665 67 06          0874        DW   AF36
0667 9F 06          0875 AF36   DW   AF37
0669 6B 06          0876        DW   AF39
066B A3 06          0877 AF39   DW   AF40
066D 00 80          0878        DW   NIL
066F 36 84          0879 AF1    DW   CARAT+ATOMIC
0671 A7 06          0880        DW   AF2
0673 3A 84          0881 AF4    DW   CDRAT+ATOMIC
0675 AB 06          0882        DW   AF5
0677 43 84          0883 AF7    DW   CONSAT+ATOMIC
0679 AF 06          0884        DW   AF8
067B 31 84          0885 AF10   DW   ATOMAT+ATOMIC
067D B3 06          0886        DW   AF11
067F 4F 84          0887 AF13   DW   EQAT+ATOMIC
0681 B7 06          0888        DW   AF14
0683 48 84          0889 AF16   DW   DEFINAT+ATOMIC
0685 BB 06          0890        DW   AF17
0687 6D 84          0891 AF19   DW   CADDAT+ATOMIC
0689 BF 06          0892        DW   AF20
068B 73 84          0893 AF22   DW   CADRAT+ATOMIC
068D C3 06          0894        DW   AF23
068F 78 84          0895 AF25   DW   NULLAT+ATOMIC
0691 C7 06          0896        DW   AF26
0693 7D 84          0897 AF28   DW   EQULAT+ATOMIC
0695 CB 06          0898        DW   AF29
0697 83 84          0899 AF31   DW   PAIRAT+ATOMIC
0699 CF 06          0900        DW   AF32
069B 8B 84          0901 AF34   DW   ASOCAT+ATOMIC
069D D3 06          0902        DW   AF35
069F 91 84          0903 AF37   DW   EVALAT+ATOMIC
06A1 D7 06          0904        DW   AF38
06A3 96 84          0905 AF40   DW   APLYAT+ATOMIC
06A5 DB 06          0906        DW   AF41
06A7 09 00          0907 AF2    DW   CAR
06A9 01 00          0908        DW   1
06AB 10 00          0909 AF5    DW   CDR
06AD 01 00          0910        DW   1
06AF 16 00          0911 AF8    DW   CONS
06B1 02 00          0912        DW   2
06B3 2B 00          0913 AF11   DW   ATOM
06B5 81 00          0914        DW   81H
06B7 31 00          0915 AF14   DW   EQ
06B9 82 00          0916        DW   82H
06BB FF 05          0917 AF17   DW   DEFNE
06BD 01 00          0918        DW   1
06BF 03 00          0919 AF20   DW   CADDR
06C1 01 00          0920        DW   1
06C3 06 00          0921 AF23   DW   CADR
06C5 01 00          0922        DW   1
06C7 49 00          0923 AF26   DW   NULL
06C9 81 00          0924        DW   81H
06CB B3 00          0925 AF29   DW   EQUAL
06CD 82 00          0926        DW   82H
06CF EB 00          0927 AF32   DW   PAIRLIS
06D1 03 00          0928        DW   3
06D3 16 01          0929 AF35   DW   ASSOC
06D5 02 00          0930        DW   2
06D7 3C 01          0931 AF38   DW   EVAL
06D9 02 00          0932        DW   2
06DB 96 01          0933 AF41   DW   APPLY
06DD 03 00          0934        DW   3
