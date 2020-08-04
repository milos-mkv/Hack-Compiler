// This file is part of www.nand2tetris.org
// and the book "The Elements of Computing Systems"
// by Nisan and Schocken, MIT Press.
// File name: projects/04/Mult.asm

// Multiplies R0 and R1 and stores the result in R2.
// (R0, R1, R2 refer to RAM[0], RAM[1], and RAM[2], respectively.)
// Put your code here.
  @i
  M=0
  @sum
  M=0
  @R1
  D=M
  @DONE
  D;JEQ
(LOOP)
  @i
  D=M
  @R0
  D=D-M
  @DONE
  D;JEQ
  @R1
  D=M
  @sum
  M=M+D
  @1
  D=A
  @i
  M=M+D
  @LOOP
  0;JMP
(DONE)
  @sum
  D=M
  @R2
  M=D
(END)
  @END
  0;JMP