/*****************************************************************************/
/* Z80���ߥ�졼��							     */
/*									     */
/*****************************************************************************/

/******************************************************************************
 *
 *    �����ͤˤ������
 *	  fMSX �Υ�����				http://www.komkon.org/fms/
 *	  MAME �Υ�����				http://www.mame.net/
 *	  Z80 CPU User Manual			http://www.zilog.com/
 *	  The Undocumented Z80 Documented	http://www.msxnet.org/tech/
 *
 *    ��������ή��
 *
 *	  total_state = 0;	���δؿ��򽪤���ޤǤ˽����������ơ��ȿ��߷�
 *	  z80_state_goal = �¹Ԥ��������ơ��ȿ���0��̵��
 *				(����ͤϰ�����Ϳ�����뤬�������ѿ���
 *				 �������Х��ѿ��ʤΤǡ����������ͤ��ѹ�����ǽ)
 *	  for(;;){	-----------------------------------------------------
 *	    z80_state_intchk = �ʲ��Υ롼�פˤƽ������륹�ơ��ȿ�
 *				(������ȯ���ޤǤΥ��ơ��ȿ� z80->icount ����
 *				 z80_state_goal �ˤ��Ĥꥹ�ơ��ȿ�)
 *
 *	    do{			- - - - - - - - - - - - - - - - - - - - - - -
 *
 *	      z80->state0 += ̿����Υ��ơ��ȿ�;
 *	      ̿�����();	�����ˤ�ꡢ�롼�פ�ȴ����
 *				    [z80_state_intchk = 0 �ǡ��롼�פ�ȴ����]
 *				    �������߾�郎�ѹ����줿��
 *				    ��EI ��������̿�� (FD/DD��³�����)   (��1)
 *				    ��PIO���������ˤ�ꡢCPU���ڤ��ؤ����(��2)
 *				    ����˥塼�⡼�ɤʤɤ����ܤ�����      (��2)
 *				��1 �ξ�硢�롼�פ�ȴ����ľ��ϳ����ߤ�̵��
 *				    [z80->skip_intr_chk = TRUE �򥻥å�]
 *				��2 �ξ��ϡ����δؿ������ȴ���롣
 *				    [z80_state_goal = 1 �ǡ��ؿ���ȴ����]
 *
 *	    } while( z80->state0 < z80_state_intchk ) - - - - - - - - - - - -
 *
 *	    (z80->intr_update)(); ������ȯ����Ƚ��
 *				  (���δؿ�������ƽС�����ƽФޤǤδ֤˽���
 *				   �������ơ��ȿ��� z80->state0 �Ȥʤ롣)
 *				  ���ȯ���� z80->INT_active = TRUE �򥻥åȡ�
 *				  ������ȯ���ޤǤΥ��ơ��ȿ� (z80->icount)
 *				  ���碌�ƹ������Ƥ�����
 *				  ��˥塼�⡼�ɤʤɤ����ܤǡ����δؿ�����
 *				  ȴ������ϡ� z80_state_goal = 1 �򥻥åȡ�
 *
 *	    total_state += z80->state0;		�¹Ԥ������ơ��ȿ����߷׹���
 *	    z80->state0  = 0;			�¹Ԥ������ơ��ȿ����ꥢ
 *
 *	    if( z80->skip_intr_chk ){		EI̿���ľ��ʤɤϡ�
 *	      z80->skip_intr_chk = 0;		����Ͻ����롼�פ�1���ȴ����
 *	      z80->icount = 0;			������ȯ����Ƚ��˿ʤࡣ
 *	    }else{
 *	      if( z80->IFF==INT_ENABLE &&	�����ߥ����å���Ԥ�
 *		  z80->INT_active ){
 *	        level = (z80->intr_ack)();	���δؿ��ǳ����٥�����
 *	        if( level >= 0 ){
 *	          ���ʬ������();
 *	          z80->state0 += ����Υ��ơ��ȿ�;
 *	        }
 *	      }
 *	    }
 *
 *	    if( z80_state_goal ){	���ꤷ�����ơ��ȿ��¹Ԥ����齪���
 *	      if( z80_state_goal <= total_state ) break;
 *	         ������ z80_state_goal = 1 �򥻥åȤ����顢ɬ������뤳�Ȥˤʤ�
 *	    }
 *	  }		------------------------------------------------------
 *
 *    ������
 *	�������߱����ϡ�PC-8801 ���ò�
 *	���⡼��0�����ߤΥ����ɽ����Ϸ�ᤦ��
 *	���Υ�ޥ����֥�����ߤ�̤���ݡ���
 *	��IN / OUT ̿��ϡ�256��ʬ�Υݡ��ȤΤߥ��ݡ���
 *
 *****************************************************************************/

#include <stdio.h>

#include "quasi88.h"
#include "z80.h"


#define S_FLAG      (0x80)
#define Z_FLAG      (0x40)
#define H_FLAG      (0x10)
#define P_FLAG      (0x04)
#define V_FLAG      (0x04)
#define N_FLAG      (0x02)
#define C_FLAG      (0x01)

#define ACC		AF.B.h
#define	FLAG		AF.B.l

#define M_C()     (z80->FLAG & C_FLAG)
#define M_NC()    (!M_C())
#define M_Z()     (z80->FLAG & Z_FLAG)
#define M_NZ()    (!M_Z())
#define M_M()     (z80->FLAG & S_FLAG)
#define M_P()     (!M_M())
#define M_PE()    (z80->FLAG & V_FLAG)
#define M_PO()    (!M_PE())


#define M_FETCH(addr)		(z80->fetch)(addr)
#define M_RDMEM(addr)		(z80->mem_read)(addr)
#define M_WRMEM(addr,data)	(z80->mem_write)(addr,data)
#define M_RDIO(addr)		(z80->io_read)(addr)
#define M_WRIO(addr,data)	(z80->io_write)(addr,data)





/****************************************************************************
 * void	z80_reset( z80arch *z80 )
 *
 *	Z80 ���ߥ�졼���� ��¤��(z80arch)������(�ꥻ�å�)���롣
 *		PC��I��R �쥸������ 0
 *		����¾������ 0xffff
 *		����ػߡ�����⡼�� 0
 *
 *	�ʲ��ι�¤�Υ��Фϡ��ƽ�¦�ˤƽ����
 *		z80->log
 *		z80->break_if_halt
 *		�Ƽ�ؿ��ݥ���
 *****************************************************************************/
void	z80_reset( z80arch *z80 )
{
  z80->AF.W  = z80->BC.W  = z80->DE.W  = z80->HL.W  =0xffff;
  z80->AF1.W = z80->BC1.W = z80->DE1.W = z80->HL1.W =0xffff;
  z80->IX.W  = z80->IY.W  = 0xffff;

  z80->PC.W = 0x0000;	    z80->SP.W = 0xffff;
  z80->I    = 0x00;
  z80->R    = 0x00;	    z80->R_saved = z80->R;

  z80->IFF  = INT_DISABLE;  z80->IFF2 = INT_DISABLE;
  z80->IM   = 0;

  z80->state0 = 0;
  z80->icount = 0;

  z80->HALT       = FALSE;
  z80->INT_active = FALSE;

  z80->skip_intr_chk = FALSE;

  z80->PC_prev.W = 0x0000;
}


/*---------------------------------------------------------------------------*/


/*------------------------------------------------------*/
/* �����ơ��ȡ����ե�̿��Υޥ��� 			*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*		     �� �� 0  �� 0  ��			*/
/*	            (�� ��    ��        �ơ��֥뻲��)	*/
/*------------------------------------------------------*/
#define M_RLC(reg)	do{					\
			  z80->FLAG = reg>>7;			\
			  reg = (reg<<1)|z80->FLAG;		\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)
#define M_RL(reg)       do{					\
			  if( reg&0x80 ){          		\
			    reg = (reg<<1)|(z80->FLAG&C_FLAG); 	\
			    z80->FLAG = SZP_table[reg]|C_FLAG;	\
			  }else{				\
			    reg = (reg<<1)|(z80->FLAG&C_FLAG);  \
			    z80->FLAG = SZP_table[reg];        	\
			  }					\
			}while(0)
#define M_RRC(reg)	do{					\
			  z80->FLAG = reg&0x01;			\
			  reg = (reg>>1)|(z80->FLAG<<7);	\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)
#define M_RR(reg)       do{					\
			  if( reg&0x01 ){          		\
			    reg = (reg>>1)|(z80->FLAG<<7);     	\
			    z80->FLAG = SZP_table[reg]|C_FLAG;	\
			  }else{				\
			    reg = (reg>>1)|(z80->FLAG<<7);     	\
			    z80->FLAG = SZP_table[reg];        	\
			  }					\
			}while(0)
#define M_SLA(reg)	do{					\
			  z80->FLAG = reg>>7;			\
			  reg <<= 1;				\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)
#define M_SRA(reg)	do{					\
			  z80->FLAG = reg&C_FLAG;		\
			  reg = (reg>>1)|(reg&0x80);		\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)
#define M_SLL(reg)	do{					\
			  z80->FLAG = reg>>7;			\
			  reg = (reg<<1)|0x01;			\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)
#define M_SRL(reg)	do{					\
			  z80->FLAG = reg&0x01;			\
			  reg >>= 1;				\
			  z80->FLAG |= SZP_table[reg];		\
			}while(0)

/*------------------------------------------------------*/
/* �ӥåȱ黻�Υޥ���					*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(BIT)	     �� �� 1  �� 0  ��			*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(SET/RES)    �� �� �� �� �� ��			*/
/*							*/
/*	BIT 7,reg �Ǥϡ�������ե饰���Ѳ����롣	*/
/*------------------------------------------------------*/
#define M_BIT(bit,reg)	do{						    \
			  z80->FLAG = (z80->FLAG&~(Z_FLAG|N_FLAG|S_FLAG)) | \
			              H_FLAG |                              \
				      ((reg&(1<<bit))?((bit==7)?S_FLAG:0)   \
				                     :Z_FLAG);              \
			}while(0)

#define M_SET(bit,reg)	do{  reg |=   1<<bit;   }while(0)
#define M_RES(bit,reg)	do{  reg &= ~(1<<bit);  }while(0)

/*------------------------------------------------------*/
/* PUSH/POP/ʬ��̿��Υޥ���				*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*		     �� �� �� �� �� ��			*/
/*------------------------------------------------------*/
#define M_POP(reg)	do{						\
			  z80->reg.B.l = M_RDMEM( z80->SP.W++ );	\
			  z80->reg.B.h = M_RDMEM( z80->SP.W++ );	\
			}while(0)
#define M_PUSH(reg)	do{						\
			  M_WRMEM( --z80->SP.W, z80->reg.B.h );		\
			  M_WRMEM( --z80->SP.W, z80->reg.B.l );		\
			}while(0)
#define M_CALL()	do{						\
			  J.B.l = M_RDMEM( z80->PC.W++ );		\
			  J.B.h = M_RDMEM( z80->PC.W++ );		\
			  M_WRMEM( --z80->SP.W, z80->PC.B.h );		\
			  M_WRMEM( --z80->SP.W, z80->PC.B.l );		\
			  z80->PC.W = J.W;				\
			  z80->state0 += 7;				\
			}while(0)
#define M_JP()		do{						\
			  J.B.l = M_RDMEM( z80->PC.W++ );		\
			  J.B.h = M_RDMEM( z80->PC.W );			\
			  z80->PC.W = J.W;				\
			}while(0)
#define M_JR()		do{						\
			  z80->PC.W += (offset)M_RDMEM(z80->PC.W)+1;	\
			  z80->state0 += 5;				\
			}while(0)
#define M_RET()		do{						\
			  z80->PC.B.l = M_RDMEM( z80->SP.W++ );		\
			  z80->PC.B.h = M_RDMEM( z80->SP.W++ );		\
			  z80->state0 += 6;				\
			}while(0)
#define M_RST(addr)	do{						\
			  M_WRMEM( --z80->SP.W, z80->PC.B.h );		\
			  M_WRMEM( --z80->SP.W, z80->PC.B.l );		\
			  z80->PC.W = addr;				\
			}while(0)


#define M_CALL_SKIP()	do{ z80->PC.W += 2; }while(0)
#define M_JP_SKIP()     do{ z80->PC.W += 2; }while(0)
#define M_JR_SKIP()     do{ z80->PC.W ++;   }while(0)
#define M_RET_SKIP()    do{                 }while(0)

/*------------------------------------------------------*/
/* 16�ӥåȥ�����̿��Υޥ���				*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*		     �� �� �� �� �� ��			*/
/*------------------------------------------------------*/
#define M_LDWORD(reg)	do{						\
			  z80->reg.B.l = M_RDMEM( z80->PC.W++ );	\
			  z80->reg.B.h = M_RDMEM( z80->PC.W++ );	\
			}while(0)

/*------------------------------------------------------*/
/* 8�ӥåȻ��ѱ黻̿��Υޥ���				*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*		     �� �� �� �� �� ��			*/
/*	            (�� ��              �ơ��֥뻲��)	*/
/*------------------------------------------------------*/
#define M_ADD_A(reg)	do{						     \
			  J.W = z80->ACC+reg;				     \
			  z80->FLAG =					     \
			   SZ_table[J.B.l] | ((z80->ACC^reg^J.B.l)&H_FLAG) | \
			   (~(z80->ACC^reg)&(reg^J.B.l)&0x80? V_FLAG:0) |    \
			   J.B.h;					     \
			  z80->ACC = J.B.l;				     \
			}while(0)
#define M_ADC_A(reg)	do{						     \
			  J.W = z80->ACC +reg +(z80->FLAG&C_FLAG);	     \
			  z80->FLAG =					     \
			   SZ_table[J.B.l] | ((z80->ACC^reg^J.B.l)&H_FLAG) | \
			   (~(z80->ACC^reg)&(reg^J.B.l)&0x80? V_FLAG:0) |    \
			   J.B.h;					     \
			  z80->ACC = J.B.l;				     \
			}while(0)
#define M_SUB(reg)	do{						     \
			  J.W = z80->ACC-reg;				     \
			  z80->FLAG =					     \
			   SZ_table[J.B.l] | ((z80->ACC^reg^J.B.l)&H_FLAG) | \
			   ((z80->ACC^reg)&(z80->ACC^J.B.l)&0x80? V_FLAG:0)| \
			   N_FLAG | -J.B.h;				     \
			  z80->ACC = J.B.l;				     \
			}while(0)
#define M_SBC_A(reg)	do{						     \
			  J.W = z80->ACC-reg-(z80->FLAG&C_FLAG);	     \
			  z80->FLAG =					     \
			   SZ_table[J.B.l] | ((z80->ACC^reg^J.B.l)&H_FLAG) | \
			   ((z80->ACC^reg)&(z80->ACC^J.B.l)&0x80? V_FLAG:0)| \
			   N_FLAG | -J.B.h;				     \
			  z80->ACC = J.B.l;				     \
			}while(0)
#define M_CP(reg)	do{						     \
			  J.W = z80->ACC-reg;				     \
			  z80->FLAG =					     \
			   SZ_table[J.B.l] | ((z80->ACC^reg^J.B.l)&H_FLAG) | \
			   ((z80->ACC^reg)&(z80->ACC^J.B.l)&0x80? V_FLAG:0)| \
			   N_FLAG | -J.B.h;				     \
			}while(0)
#define M_INC(reg)	do{						     \
			  reg++;					     \
			  z80->FLAG =					     \
			   SZ_table[reg] | (reg&0x0f? 0:H_FLAG) |	     \
			   (reg==0x80? V_FLAG:0) | (z80->FLAG&C_FLAG);	     \
			}while(0)
#define M_DEC(reg)	do{						     \
			  reg--;					     \
			  z80->FLAG =					     \
			   SZ_table[reg] | ((reg&0x0f)==0x0f? H_FLAG:0) |    \
			   (reg==0x7f? V_FLAG:0)| N_FLAG |(z80->FLAG&C_FLAG);\
			}while(0)

/*------------------------------------------------------*/
/* 8�ӥå�����̿��Υޥ���				*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(AND)	     �� �� 1  �� 0  0			*/
/*	            (�� ��    ��        �ơ��֥뻲��)	*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(OR/XOR)     �� �� 0  �� 0  0			*/
/*	            (�� ��    ��        �ơ��֥뻲��)	*/
/*------------------------------------------------------*/
#define M_AND(reg)	do{						\
			  z80->ACC &= reg;				\
			  z80->FLAG = SZP_table[z80->ACC]|H_FLAG;	\
			}while(0)
#define M_OR(reg)	do{						\
			  z80->ACC |= reg;				\
			  z80->FLAG = SZP_table[z80->ACC];		\
			}while(0)
#define M_XOR(reg)	do{						\
			  z80->ACC ^= reg;				\
			  z80->FLAG = SZP_table[z80->ACC];		\
			}while(0)

/*------------------------------------------------------*/
/* ������̿��Υޥ���					*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(IN)	     �� �� 0  �� 0  ��			*/
/*	            (�� ��    ��        �ơ��֥뻲��)	*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(OUT)	     �� �� �� �� �� ��			*/
/*------------------------------------------------------*/
#define M_IN_C(reg)	do{						\
			  I = M_RDIO( z80->BC.B.l );			\
			  reg = I;					\
			  z80->FLAG = SZP_table[reg]|(z80->FLAG&C_FLAG);\
			}while(0)

#define M_OUT_C(reg)	do{						\
			  M_WRIO( z80->BC.B.l, reg );			\
			}while(0)


/*------------------------------------------------------*/
/* 16�ӥåȻ��ѱ黻̿��Υޥ���				*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(ADD)	     �� �� �� �� 0  ��			*/
/*	�ե饰�Ѳ� : S  Z  H  PV N  C			*/
/*	(ADC/SBC)    �� �� �� �� �� ��			*/
/*------------------------------------------------------*/
#define M_ADDW(reg1,reg2)	do{					      \
				  J.W = (reg1+reg2)&0xffff;		      \
				  z80->FLAG =				      \
				   (z80->FLAG&~(H_FLAG|N_FLAG|C_FLAG))|	      \
				   ((reg1^reg2^J.W)&0x1000? H_FLAG:0) |       \
				   (((long)reg1+(long)reg2)&0x10000?C_FLAG:0);\
				  reg1 = J.W;				      \
				}while(0)
#define M_ADCW(reg)  do{						      \
		       I = z80->FLAG&C_FLAG;				      \
		       J.W = (z80->HL.W+reg+I)&0xffff;			      \
		       z80->FLAG = 					      \
		       (J.B.h&S_FLAG) | (J.W? 0:Z_FLAG) |		      \
		       ((z80->HL.W^reg^J.W)&0x1000? H_FLAG:0)        |        \
		       (~(z80->HL.W^reg)&(reg^J.W)&0x8000? V_FLAG:0) |        \
		       (((long)z80->HL.W+(long)reg+(long)I)&0x10000?C_FLAG:0);\
		       z80->HL.W = J.W;					      \
		     }while(0)
#define M_SBCW(reg)  do{						      \
		       I = z80->FLAG&C_FLAG;				      \
		       J.W = (z80->HL.W-reg-I)&0xffff;			      \
		       z80->FLAG =					      \
		       (J.B.h&S_FLAG) | (J.W? 0:Z_FLAG) |		      \
		       ((z80->HL.W^reg^J.W)&0x1000? H_FLAG:0)            |\
		       ((z80->HL.W^reg)&(z80->HL.W^J.W)&0x8000? V_FLAG:0)|\
		       N_FLAG |					\
		       (((long)z80->HL.W-(long)reg-(long)I)&0x10000?C_FLAG:0);\
		       z80->HL.W = J.W;				      \
		     }while(0)



/*---------------------------------------------------------------------------*/

enum Codes
{
  NOP,     LD_BC_16, LD_xBC_A, INC_BC,   INC_B,   DEC_B,   LD_B_8,   RLCA,
  EX_AF_AF,ADD_HL_BC,LD_A_xBC, DEC_BC,   INC_C,   DEC_C,   LD_C_8,   RRCA,
  DJNZ,    LD_DE_16, LD_xDE_A, INC_DE,   INC_D,   DEC_D,   LD_D_8,   RLA,
  JR,      ADD_HL_DE,LD_A_xDE, DEC_DE,   INC_E,   DEC_E,   LD_E_8,   RRA,
  JR_NZ,   LD_HL_16, LD_x16_HL,INC_HL,   INC_H,   DEC_H,   LD_H_8,   DAA,
  JR_Z,    ADD_HL_HL,LD_HL_x16,DEC_HL,   INC_L,   DEC_L,   LD_L_8,   CPL,
  JR_NC,   LD_SP_16, LD_x16_A, INC_SP,   INC_xHL, DEC_xHL, LD_xHL_8, SCF,
  JR_C,    ADD_HL_SP,LD_A_x16, DEC_SP,   INC_A,   DEC_A,   LD_A_8,   CCF,
  LD_B_B,  LD_B_C,   LD_B_D,   LD_B_E,   LD_B_H,  LD_B_L,  LD_B_xHL, LD_B_A,
  LD_C_B,  LD_C_C,   LD_C_D,   LD_C_E,   LD_C_H,  LD_C_L,  LD_C_xHL, LD_C_A,
  LD_D_B,  LD_D_C,   LD_D_D,   LD_D_E,   LD_D_H,  LD_D_L,  LD_D_xHL, LD_D_A,
  LD_E_B,  LD_E_C,   LD_E_D,   LD_E_E,   LD_E_H,  LD_E_L,  LD_E_xHL, LD_E_A,
  LD_H_B,  LD_H_C,   LD_H_D,   LD_H_E,   LD_H_H,  LD_H_L,  LD_H_xHL, LD_H_A,
  LD_L_B,  LD_L_C,   LD_L_D,   LD_L_E,   LD_L_H,  LD_L_L,  LD_L_xHL, LD_L_A,
  LD_xHL_B,LD_xHL_C, LD_xHL_D, LD_xHL_E, LD_xHL_H,LD_xHL_L,HALT,     LD_xHL_A,
  LD_A_B,  LD_A_C,   LD_A_D,   LD_A_E,   LD_A_H,  LD_A_L,  LD_A_xHL, LD_A_A,
  ADD_A_B, ADD_A_C,  ADD_A_D,  ADD_A_E,  ADD_A_H, ADD_A_L, ADD_A_xHL,ADD_A_A,
  ADC_A_B, ADC_A_C,  ADC_A_D,  ADC_A_E,  ADC_A_H, ADC_A_L, ADC_A_xHL,ADC_A_A,
  SUB_B,   SUB_C,    SUB_D,    SUB_E,    SUB_H,   SUB_L,   SUB_xHL,  SUB_A,
  SBC_A_B, SBC_A_C,  SBC_A_D,  SBC_A_E,  SBC_A_H, SBC_A_L, SBC_A_xHL,SBC_A_A,
  AND_B,   AND_C,    AND_D,    AND_E,    AND_H,   AND_L,   AND_xHL,  AND_A,
  XOR_B,   XOR_C,    XOR_D,    XOR_E,    XOR_H,   XOR_L,   XOR_xHL,  XOR_A,
  OR_B,    OR_C,     OR_D,     OR_E,     OR_H,    OR_L,    OR_xHL,   OR_A,
  CP_B,    CP_C,     CP_D,     CP_E,     CP_H,    CP_L,    CP_xHL,   CP_A,
  RET_NZ,  POP_BC,   JP_NZ,    JP,       CALL_NZ, PUSH_BC, ADD_A_8,  RST00,
  RET_Z,   RET,      JP_Z,     PFX_CB,   CALL_Z,  CALL,    ADC_A_8,  RST08,
  RET_NC,  POP_DE,   JP_NC,    OUT_x8_A, CALL_NC, PUSH_DE, SUB_8,    RST10,
  RET_C,   EXX,      JP_C,     IN_A_x8,  CALL_C,  PFX_DD,  SBC_A_8,  RST18,
  RET_PO,  POP_HL,   JP_PO,    EX_xSP_HL,CALL_PO, PUSH_HL, AND_8,    RST20,
  RET_PE,  JP_xHL,   JP_PE,    EX_DE_HL, CALL_PE, PFX_ED,  XOR_8,    RST28,
  RET_P,   POP_AF,   JP_P,     DI,       CALL_P,  PUSH_AF, OR_8,     RST30,
  RET_M,   LD_SP_HL, JP_M,     EI,       CALL_M,  PFX_FD,  CP_8,     RST38,
  EndofCodes
};

enum CodesCB
{
  RLC_B,   RLC_C,   RLC_D,   RLC_E,   RLC_H,   RLC_L,   RLC_xHL,   RLC_A,
  RRC_B,   RRC_C,   RRC_D,   RRC_E,   RRC_H,   RRC_L,   RRC_xHL,   RRC_A,
  RL_B,    RL_C,    RL_D,    RL_E,    RL_H,    RL_L,    RL_xHL,    RL_A,
  RR_B,    RR_C,    RR_D,    RR_E,    RR_H,    RR_L,    RR_xHL,    RR_A,
  SLA_B,   SLA_C,   SLA_D,   SLA_E,   SLA_H,   SLA_L,   SLA_xHL,   SLA_A,
  SRA_B,   SRA_C,   SRA_D,   SRA_E,   SRA_H,   SRA_L,   SRA_xHL,   SRA_A,
  SLL_B,   SLL_C,   SLL_D,   SLL_E,   SLL_H,   SLL_L,   SLL_xHL,   SLL_A,
  SRL_B,   SRL_C,   SRL_D,   SRL_E,   SRL_H,   SRL_L,   SRL_xHL,   SRL_A,
  BIT_0_B, BIT_0_C, BIT_0_D, BIT_0_E, BIT_0_H, BIT_0_L, BIT_0_xHL, BIT_0_A,
  BIT_1_B, BIT_1_C, BIT_1_D, BIT_1_E, BIT_1_H, BIT_1_L, BIT_1_xHL, BIT_1_A,
  BIT_2_B, BIT_2_C, BIT_2_D, BIT_2_E, BIT_2_H, BIT_2_L, BIT_2_xHL, BIT_2_A,
  BIT_3_B, BIT_3_C, BIT_3_D, BIT_3_E, BIT_3_H, BIT_3_L, BIT_3_xHL, BIT_3_A,
  BIT_4_B, BIT_4_C, BIT_4_D, BIT_4_E, BIT_4_H, BIT_4_L, BIT_4_xHL, BIT_4_A,
  BIT_5_B, BIT_5_C, BIT_5_D, BIT_5_E, BIT_5_H, BIT_5_L, BIT_5_xHL, BIT_5_A,
  BIT_6_B, BIT_6_C, BIT_6_D, BIT_6_E, BIT_6_H, BIT_6_L, BIT_6_xHL, BIT_6_A,
  BIT_7_B, BIT_7_C, BIT_7_D, BIT_7_E, BIT_7_H, BIT_7_L, BIT_7_xHL, BIT_7_A,
  RES_0_B, RES_0_C, RES_0_D, RES_0_E, RES_0_H, RES_0_L, RES_0_xHL, RES_0_A,
  RES_1_B, RES_1_C, RES_1_D, RES_1_E, RES_1_H, RES_1_L, RES_1_xHL, RES_1_A,
  RES_2_B, RES_2_C, RES_2_D, RES_2_E, RES_2_H, RES_2_L, RES_2_xHL, RES_2_A,
  RES_3_B, RES_3_C, RES_3_D, RES_3_E, RES_3_H, RES_3_L, RES_3_xHL, RES_3_A,
  RES_4_B, RES_4_C, RES_4_D, RES_4_E, RES_4_H, RES_4_L, RES_4_xHL, RES_4_A,
  RES_5_B, RES_5_C, RES_5_D, RES_5_E, RES_5_H, RES_5_L, RES_5_xHL, RES_5_A,
  RES_6_B, RES_6_C, RES_6_D, RES_6_E, RES_6_H, RES_6_L, RES_6_xHL, RES_6_A,
  RES_7_B, RES_7_C, RES_7_D, RES_7_E, RES_7_H, RES_7_L, RES_7_xHL, RES_7_A,  
  SET_0_B, SET_0_C, SET_0_D, SET_0_E, SET_0_H, SET_0_L, SET_0_xHL, SET_0_A,
  SET_1_B, SET_1_C, SET_1_D, SET_1_E, SET_1_H, SET_1_L, SET_1_xHL, SET_1_A,
  SET_2_B, SET_2_C, SET_2_D, SET_2_E, SET_2_H, SET_2_L, SET_2_xHL, SET_2_A,
  SET_3_B, SET_3_C, SET_3_D, SET_3_E, SET_3_H, SET_3_L, SET_3_xHL, SET_3_A,
  SET_4_B, SET_4_C, SET_4_D, SET_4_E, SET_4_H, SET_4_L, SET_4_xHL, SET_4_A,
  SET_5_B, SET_5_C, SET_5_D, SET_5_E, SET_5_H, SET_5_L, SET_5_xHL, SET_5_A,
  SET_6_B, SET_6_C, SET_6_D, SET_6_E, SET_6_H, SET_6_L, SET_6_xHL, SET_6_A,
  SET_7_B, SET_7_C, SET_7_D, SET_7_E, SET_7_H, SET_7_L, SET_7_xHL, SET_7_A,
  EndofCodesCB
};
  
enum CodesED
{
  ED_00,   ED_01,    ED_02,     ED_03,      ED_04,  ED_05,   ED_06,   ED_07,
  ED_08,   ED_09,    ED_0A,     ED_0B,      ED_0C,  ED_0D,   ED_0E,   ED_0F,
  ED_10,   ED_11,    ED_12,     ED_13,      ED_14,  ED_15,   ED_16,   ED_17,
  ED_18,   ED_19,    ED_1A,     ED_1B,      ED_1C,  ED_1D,   ED_1E,   ED_1F,
  ED_20,   ED_21,    ED_22,     ED_23,      ED_24,  ED_25,   ED_26,   ED_27,
  ED_28,   ED_29,    ED_2A,     ED_2B,      ED_2C,  ED_2D,   ED_2E,   ED_2F,
  ED_30,   ED_31,    ED_32,     ED_33,      ED_34,  ED_35,   ED_36,   ED_37,
  ED_38,   ED_39,    ED_3A,     ED_3B,      ED_3C,  ED_3D,   ED_3E,   ED_3F,
  IN_B_xC, OUT_xC_B, SBC_HL_BC, LD_x16x_BC, NEG,    RETN,    IM_0,    LD_I_A,
  IN_C_xC, OUT_xC_C, ADC_HL_BC, LD_BC_x16x, NEG_4C, RETI,    IM_0_4E, LD_R_A,
  IN_D_xC, OUT_xC_D, SBC_HL_DE, LD_x16x_DE, NEG_54, RETN_55, IM_1,    LD_A_I,
  IN_E_xC, OUT_xC_E, ADC_HL_DE, LD_DE_x16x, NEG_5C, RETN_5D, IM_2,    LD_A_R,
  IN_H_xC, OUT_xC_H, SBC_HL_HL, LD_x16x_HL, NEG_64, RETN_65, IM_0_66, RRD,
  IN_L_xC, OUT_xC_L, ADC_HL_HL, LD_HL_x16x, NEG_6C, RETN_6D, IM_0_6E, RLD,
  IN_F_xC, OUT_xC_F, SBC_HL_SP, LD_x16x_SP, NEG_74, RETN_75, IM_1_76, ED_77,
  IN_A_xC, OUT_xC_A, ADC_HL_SP, LD_SP_x16x, NEG_7C, RETN_7D, IM_2_7E, ED_7F,
  ED_80,   ED_81,    ED_82,     ED_83,      ED_84,  ED_85,   ED_86,   ED_87,
  ED_88,   ED_89,    ED_8A,     ED_8B,      ED_8C,  ED_8D,   ED_8E,   ED_8F,
  ED_90,   ED_91,    ED_92,     ED_93,      ED_94,  ED_95,   ED_96,   ED_97,
  ED_98,   ED_99,    ED_9A,     ED_9B,      ED_9C,  ED_9D,   ED_9E,   ED_9F,
  LDI,     CPI,      INI,       OUTI,       ED_A4,  ED_A5,   ED_A6,   ED_A7,
  LDD,     CPD,      IND,       OUTD,       ED_AC,  ED_AD,   ED_AE,   ED_AF,
  LDIR,    CPIR,     INIR,      OTIR,       ED_B4,  ED_B5,   ED_B6,   ED_B7,
  LDDR,    CPDR,     INDR,      OTDR,       ED_BC,  ED_BD,   ED_BE,   ED_BF,
  ED_C0,   ED_C1,    ED_C2,     ED_C3,      ED_C4,  ED_C5,   ED_C6,   ED_C7,
  ED_C8,   ED_C9,    ED_CA,     ED_CB,      ED_CC,  ED_CD,   ED_CE,   ED_CF,
  ED_D0,   ED_D1,    ED_D2,     ED_D3,      ED_D4,  ED_D5,   ED_D6,   ED_D7,
  ED_D8,   ED_D9,    ED_DA,     ED_DB,      ED_DC,  ED_DD,   ED_DE,   ED_DF,
  ED_E0,   ED_E1,    ED_E2,     ED_E3,      ED_E4,  ED_E5,   ED_E6,   ED_E7,
  ED_E8,   ED_E9,    ED_EA,     ED_EB,      ED_EC,  ED_ED,   ED_EE,   ED_EF,
  ED_F0,   ED_F1,    ED_F2,     ED_F3,      ED_F4,  ED_F5,   ED_F6,   ED_F7,
  ED_F8,   ED_F9,    ED_FA,     ED_FB,      ED_FC,  ED_FD,   ED_FE,   ED_FF,
  EndofCodesED
};


/*---------------------------------------------------------------------------*/


#include "z80-table.h"


/*---------------------------------------------------------------------------*/

INLINE	void	z80_code_CB( z80arch *z80 )
{
  int	opcode;
  byte	I;

  opcode = M_FETCH( z80->PC.W++ );
  z80->R ++;
  z80->state0 += state_CB_table[ opcode ];

  switch( opcode ){
#include "z80-codeCB.h"				/* CB XX */
  default:					/* CB ?? */
    printf("!! Internal Error in Z80-Emulator !!\n");
    printf("  PC = %04X : code = CB %02X\n", z80->PC.W-2, opcode );
  }
}


INLINE	void	z80_code_ED( z80arch *z80 )
{
  int	opcode;
  byte	I;
  pair	J;

  opcode = M_FETCH( z80->PC.W++ );
  z80->R ++;
  z80->state0 += state_ED_table[ opcode ];

  switch( opcode ){
#include "z80-codeED.h"				/* ED XX */
  default:					/* ED ?? */
    if( verbose_z80 )
      printf( "Unrecognized instruction: ED %02X at PC=%04X\n",
	      M_RDMEM(z80->PC.W-1), z80->PC.W-2 );
    z80->state0 += 8;	/* ED ?? == NOP NOP */
  }
}


INLINE	void	z80_code_DD( z80arch *z80 )
{
  int	opcode;
  byte	I;
  pair	J;

  opcode = M_FETCH( z80->PC.W++ );
  z80->R ++;
  z80->state0 += state_XX_table[ opcode ];

#define XX IX

  switch( opcode ){
#include "z80-codeXX.h"				/* DD XX */
  case PFX_CB:					/* DD CB �ξ�� */
    J.W = z80->XX.W +(offset)M_RDMEM( z80->PC.W++ );
    opcode = M_FETCH( z80->PC.W++ );
    z80->state0 += state_XXCB_table[ opcode ];
    switch( opcode ){
#include "z80-codeXXCB.h"			/* DD CB XX XX */
    default:					/* DD CB ?? ?? */
      printf("!! Internal Error in Z80-Emulator !!\n");
      printf("  PC = %04x : code = DD CB %02X %02X\n", z80->PC.W-4,
	     M_RDMEM(z80->PC.W-2), M_RDMEM(z80->PC.W-1) );
    }
    break;
  default:					/* DD ?? */
    if( verbose_z80 )
      printf( "Unrecognized instruction: DD %02X at PC=%04X\n",
	      M_RDMEM(z80->PC.W-1), z80->PC.W-2 );
    z80->PC.W --;
    z80->R --;					/* ?? �ΰ��֤�PC���᤹ */
    z80->state0 += 4;				/* DD == NOP */
    z80->skip_intr_chk = TRUE;			/* ������Ƚ��ʤ�    */
    z80_state_intchk = 0;
    break;
  }
#undef XX
}


INLINE	void	z80_code_FD( z80arch *z80 )
{
  int	opcode;
  byte	I;
  pair	J;

  opcode = M_FETCH( z80->PC.W++ );
  z80->R ++;
  z80->state0 += state_XX_table[ opcode ];

#define XX IY

  switch( opcode ){
#include "z80-codeXX.h"				/* FD XX */
  case PFX_CB:					/* FD CB �ξ�� */
    J.W = z80->XX.W +(offset)M_RDMEM( z80->PC.W++ );
    opcode = M_FETCH( z80->PC.W++ );
    z80->state0 += state_XXCB_table[ opcode ];
    switch( opcode ){
#include "z80-codeXXCB.h"			/* FD CB XX XX */
    default:					/* FD CB ?? ?? */
      printf("!! Internal Error in Z80-Emulator !!\n");
      printf("  PC = %04x : code = FD CB %02X %02X\n", z80->PC.W-4,
	     M_RDMEM(z80->PC.W-2), M_RDMEM(z80->PC.W-1) );
    }
    break;
  default:					/* FD ?? */
    if( verbose_z80 )
      printf( "Unrecognized instruction: FD %02X at PC=%04X\n",
	      M_RDMEM(z80->PC.W-1), z80->PC.W-2 );
    z80->PC.W --;
    z80->R --;					/* ?? �ΰ��֤�PC���᤹ */
    z80->state0 += 4;				/* FD == NOP */
    z80->skip_intr_chk = TRUE;			/* ������Ƚ��ʤ�    */
    z80_state_intchk = 0;
    break;
  }
#undef XX
}




static	int	z80_im0_interrupt( z80arch *z80, int level )
{
  int	state;
  pair	J;

  state = state_table[ level ];

  switch( level ){
  case 0:						/* NOP       */
    break;
  case 1:						/* LD (BC),A */
    M_WRMEM(z80->BC.W,z80->ACC);
    break;
  case 2:						/* INC B     */
    M_INC(z80->BC.B.h);
    break;
  case 3:						/* LD B,n    */
    z80->BC.B.h=M_RDMEM(z80->PC.W++);
    break;
  case 4:						/* EX AF,AF' */
    J.W=z80->AF.W; z80->AF.W=z80->AF1.W; z80->AF1.W=J.W;
    break;
  case 5:						/* LD A,(BC) */
    z80->ACC=M_RDMEM(z80->BC.W);
    break;
  case 6:						/* INC C     */
    M_INC(z80->BC.B.l);
    break;
  case 7:						/* LD C,n    */
    z80->BC.B.l=M_RDMEM(z80->PC.W++); 
    break;
  default:
    if( verbose_z80 )
    printf("Unexpected interrupt signal %X\n",level );
    break;
  }

  return state;
}


INLINE	void	z80_interrupt( z80arch *z80 )
{
  int level;

  level = (z80->intr_ack)();
  if( level >= 0 ){
    z80->IFF = INT_DISABLE;			/* �����߶ػ� */
    z80->R ++;					/* �����߱����������� */
    z80->state0 += 2;

    if( z80->HALT ){				/* HALT ���ֲ�� */
      z80->HALT = FALSE;
      z80->PC.W ++;
    }

    switch( z80->IM ){				/* ������ʬ������ */
    case 0:					/*		IM 0 �λ� */
      z80->state0 += z80_im0_interrupt( z80, level );
      break;
    case 1:					/*		IM 1 �λ� */
      z80->state0 += state_table[ RST38 ];
      M_RST(0x0038);
      break;
    case 2:					/*		IM 2 �λ� */
      M_PUSH(PC);
      level = ((word)z80->I << 8) | (level << 1);
      z80->PC.B.l = M_RDMEM( level++ );
      z80->PC.B.h = M_RDMEM( level );
      z80->state0 += 17;
      break;
    }
  }
}



/****************************************************************************
 * int z80_emu( z80arch *z80, int state_of_exec )
 *
 *	���� *z80          �� z80arch ����Υݥ���
 *	     state_of_exec �� �������٤����ơ��ȿ�
 *			      ==0 �λ��ϡ�̵�¤˽�����Ԥʤ���
 *			      > 0 �λ��ϡ����Υ��ơ��ȿ�ʬ������Ԥʤ���
 *		state_of_exec==0 �ξ��Ǥ⡢CPU_BREAKOFF() ���ƤӽФ��줿��
 *		���λ����ǽ�����λ���롣
 *
 *	����	�� �ºݤ˽������������ơ��ȿ�
 *
 *****************************************************************************/

/* �ʲ��Υ���ϡ����������ѹ������Τǥ������Х��ѿ��Ȥ��Ƥ��롣	*/
/* ����������ѹ��ϡ�(¿ʬ) ���߽������ CPU���Ф��ƤΤߤʤ����ϥ�	*/
/* �ʤΤǡ�z80arch ��¤�Τˤϴޤ�ʤ����Ȥˤ��롣			*/

int	z80_state_goal;		/* ����state��ʬ�������򷫤��֤�(0��̵��) */
int	z80_state_intchk;	/* ����state���¹Ը塢���Ƚ�ꤹ��	  */


int	z80_emu( z80arch *z80, int state_of_exec )
{
  int	opcode, istate = 0;
  byte	I;
  pair	J;
  int	total_state    = 0;	/* �ؿ���λ���ޤǤˡ������������ơ��ȿ�	     */


  z80_state_goal = state_of_exec;

  for( ;; ){

	/* ==== CPU�¹�state������� (�̾�ϼ����ȯ�������ߥ�) ==== */

    if( z80_state_goal ){				/* state������� */
      z80_state_intchk = MIN( z80->icount, z80_state_goal - total_state );
    }
    else{						/* ̵�»����    */
      z80_state_intchk = z80->icount;
    }


	/* ============ ��ۤɷ�᤿ state��ʬ���¹Ԥ��� ============ */

    do{

#ifdef	DEBUGLOG
      if( z80->log ) z80_logging( z80 );	/* ������Ͽ */
#endif

#ifdef	USE_MONITOR
      z80->PC_prev = z80->PC;			/* ľ���Τ�Τ򵭲� */
#endif

      opcode = M_FETCH(z80->PC.W++);		/* ̿��ե��å� */
      z80->R ++;
      z80->state0 += state_table[ opcode ];

      switch( opcode ){				/* ̿��ǥ����� */

      #include "z80-code.h"			    /* �̾�̿��ξ�� */

      case PFX_CB:  z80_code_CB( z80 );  break;	    /* CB ̿��ξ�� */

      case PFX_ED:  z80_code_ED( z80 );  break;	    /* ED ̿��ξ�� */

      case PFX_DD:  z80_code_DD( z80 );  break;	    /* DD ̿��ξ�� */

      case PFX_FD:  z80_code_FD( z80 );  break;	    /* FD ̿��ξ�� */

      default:					    /* �������ʤ��ϥ� */
	printf("!! Internal Error in Z80-Emulator !!\n");
	printf("  PC = %04X : code = %02X\n", z80->PC.W-1, opcode );
      }

    } while( z80->state0 < z80_state_intchk );

	/* ===================== ����򹹿����� ====================== */

    /* ������ȯ����Ƚ�ꤹ��ؿ���ƤӽФ���				*/
    /*									*/
    /*  �ƤӽФ����ؿ�¦����������					*/
    /*	    z80->state0 �� CPU�������������֤ʤΤǤ���򸵤�Ƚ�ꤹ�롣	*/
    /*      (z80->icount�ϲ�����Ƥ��뤫�⤷��ʤ��Τǡ����Ƥˤ��ʤ�)	*/
    /*      ���ȯ�����ϡ� z80->INT_active = TRUE �򥻥åȤ��롣	*/
    /*      z80->icount �ι����⤳�δؿ��ˤƹԤ���			*/
    /*      ��˥塼���ؤ����ܻ��ϡ� CPU_BREAKOFF() �ǽ����򽪤��롣	*/

    (z80->intr_update)();

    total_state += z80->state0;		/* �������� state �����߷� */
    z80->state0  = 0;

	/* ======== ���ȯ�������å�����ȯ�����Ƥ������������ ======== */

    if( z80->skip_intr_chk ){	/* ����������ʤ���� (EI,DD/FD����̿��ľ��)*/
      z80->skip_intr_chk = FALSE;

      if( z80->IFF==INT_ENABLE &&	    /* �����Ȥꤳ�ܤ��Ƥ���  */
	  z80->INT_active ){		    /* ľ��˳���������뤿�ᡢ*/
	z80->icount = 0;		    /* ����� 1step�����¹�    */
      }
    }
    else{			/* �̾�ϡ������������Τǡ����ä� */

      if( z80->IFF==INT_ENABLE &&	    /* ������Ĥ��ĳ��ȯ������ */
	  z80->INT_active ){

	z80_interrupt( z80 );

	/* �����ߤ�����դ���ȡ����ν���ʬ z80->state0 �����ä��뤬��
	   ����Ϥޤ� total_state �ˤ�ȿ�Ǥ���Ƥ��ʤ��Τǡ����ա�  */
      }
    }

	/* ========================== ��λȽ�� ======================= */

    if( z80_state_goal ){	/* �¹�state��������ϡ������ǽ�λȽ�� */
      if( (unsigned int)z80_state_goal <= (unsigned int)total_state ) break;

      /* ���ա�	����state���¹Ԥ���ľ��˳����߱���������硢���α���
		���פ��� state ���� z80->state0 �ˤ�ȿ�Ǥ���Ƥ��뤬��
		total_state �ˤ�ȿ�Ǥ���Ƥ��ʤ�����äơ������Ǥ�����ͤ�
		z80->state0 �������ʤ��ͤˤʤ롣�����ͤϡ����� z80_emu ��
		�ƤӽФ��줿���˲û�����Ƥ���Τǡ��ޤ��褷�Ȥ��褦��*/
    }
  }

  return	total_state;
}