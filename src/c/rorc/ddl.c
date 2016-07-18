#include "rorc.h"
#include <unistd.h>
#include <pda.h>
#include "rorc_macros.h"
#include "ddl_def.h"

char* receivedOrderedSet[]; /// XXX The usage of this looks like a segfault
char* remoteStatus[]; /// XXX The usage of this looks like a segfault

int ddlFindDiuVersion(volatile void *buff, int pci_loop_per_usec, int *rorc_revision, int *diu_version) {
  stword_t stw[DDL_MAX_REPLY];
  int  n_reply, ret;
  long long longret;
  long long int TimeOut;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  *rorc_revision = 7;//*((uint32_t*)buff + 6*1024);

  if (*rorc_revision >= RORC_REVISION_INTEG){
    // ide johetne DIU status lekerdezes
    *diu_version = EMBEDDED;
    return (RORC_STATUS_OK);
  }

  rorcReset(buff, 0, pci_loop_per_usec);   // full reset
  *diu_version = NO_DIU;
  
  /* read DIU's HW id */
  TimeOut = DDL_RESPONSE_TIME * pci_loop_per_usec;
  ret = ddlSendCommand(buff, DDL_DEST_DIU, RHWVER, 0, 0, TimeOut);
  if (ret)
    return (ret);

  n_reply = 0;
  while (n_reply < DDL_MAX_REPLY){
    longret = ddlWaitStatus(buff, TimeOut);
    if (longret >= TimeOut)
      break;
    stw[n_reply++] = ddlReadStatus(buff);
  }

  if (n_reply == 0){
    printf("No reply arrived for HW status request\n");
    return (RORC_STATUS_OK);
  }
  printf("HW status word = 0x%08lx.\n", stw[0].stw);

  *diu_version = NEW;
  if (n_reply != 2){
    *diu_version = OLD;
  }

  return (RORC_STATUS_OK);
}

stword_t ddlReadCTSTW(volatile void *buff, int transid, int destination,
		      long long int time, int pci_loop_per_usec){
  long long int longi; 
  stword_t stw; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  for (longi = 0; longi < time; longi++) { 
    if (rorcCheckRxStatus(buff)) 
      break; 
  } 
  if (time && (longi >= time)){ 
    printf("ddlReadCTSTW: no CTSTW arrived in time-out %lld usec\n",  
	   (long long int)(time/pci_loop_per_usec)); 
    stw.part.param = -1; 
    return(stw); 
  } 
 
  stw = ddlReadStatus(buff); 
  //  printf("CTSTW = 0x%08lx\n", stw.stw); 
  if ((stw.part.code != CTSTW &&  
       stw.part.code != ILCMD && 
       stw.part.code != CTSTW_TO) || 
       stw.part.trid != transid   || 
      stw.part.dest != destination){ 
    printf("NOT CTSTW! Expected: 0x%x%x%x, received: 0x%x%x%x", 
	   transid, CTSTW, destination,  
	   stw.part.trid, stw.part.code, stw.part.dest); 
    stw.part.param = -1; 
  } 
  return(stw); 
} 
 
int ddlSendCommand(volatile void *buff,
                   int             dest, 
                   __u32           command, 
                   int             transid, 
                   __u32           param, 
                   long long int   time) 
 
/* ddlSendCommand sends one command to the given link. 
 * Parameters: dev      pointer to Rorc device. It defines the link 
 *                      where the command will be sent 
 *             dest     command destination: 0 RORC, 1 DIU, 2 SIU, 4 FEE. 
 *                      if -1 then the full command is in the 
 *                      command field 
 *             command  command code 
 *             transid  transaction id 
 *             param    command parameter, 
 *                      or the full command if dest == -1 
 *             time     if > 0 then test if command can be sent and 
 *                      wait as many cycles if necessary. 
 * 
 * Returns: 
 *    RORC_STATUS_OK   (0)     if command sent 
 *    RORC_TIMEOUT     (-64)   if the command can not be sent in timeout. 
 *    RORC_LINK_NOT_ON (-4)    if destination > 1 and the link is not on 
 */ 
 
{ 
  __u32 com; 
  int destination; 
  long long int i; 
#ifdef NOMAP 
  ret; 
#endif 
  
  if(dest == -1){ 
    com = command; 
    destination = com & 0xf; 
  } 
  else{ 
    destination = dest & 0xf;  
    com = destination + ((command & 0xf)<<4) + 
      ((transid & 0xf)<<8) + ((param & 0x7ffff)<<12); 
  } 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  //  printf("ddlSendCommand: command to send: 0x%08x\n", com); 
 
  if (destination > DDL_DEST_DIU){
    if (rorcCheckLink(buff) == RORC_LINK_NOT_ON){ 
      printf("ddlSendCommand: command can not be sent as link is not on\n"); 
      return (RORC_LINK_NOT_ON); 
    } 
  } 
 
  for (i = 0; i < time; i++){ 
    if (rorcCheckCommandRegister(buff) == 0) 
      break; 
  } 
 
  if (time && (i == time)) 
    return (RORC_TIMEOUT); 
 
  rorcPutCommandRegister (buff, com); 
 
  return (RORC_STATUS_OK); 
} 
 
 
long long int ddlWaitStatus(volatile void *buff, long long int timeout)
 
/* Checks whether status mail box or register is not empty in timeout        
 * 
 * Parameters: prorc    pointer to pRORC device  
 *             timeout  # of check cycles 
 * 
 * Returns:    # of executed cycles 
 * 
*/ 
 
{ 
  long long int tries = 0; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  do {       
    if(rorcCheckRxStatus(buff)) 
      break; 
    tries++;   
  }while(tries <= timeout); 
 
  return(tries); 
} 
 
stword_t ddlReadStatus(volatile void *buff)
{ 
  stword_t stw; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  /* call ddlWaitStatus() before this routine 
     if status timeout is needed */  
  stw.stw = rorcReadReg (buff, C_DSR); 
  //  printf("ddlReadStatus: status = %08lx\n", stw.stw); 
  
  return(stw); 
} 
 
long ddlReadDiu(volatile void *buff, int transid,
			 long long int time, int pci_loop_per_usec){
  long retval;
  stword_t stw; 
  int dest; 
  long long int longi; 
  __u32 param, command; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  /* prepare and send DDL command */ 
  
  dest = DDL_DEST_DIU;
  command =  RandCIFST; 
  param = 0; 
  retval = ddlSendCommand(buff, dest, command, transid, param, time); 
  if (retval == RORC_TIMEOUT){ 
    printf("ddlReadDiu: DIU command can not be sent in timeout %lld", time); 
    retval = -1; 
    return(retval); 
  } 
 
  /* read and check the answer */ 
  for (longi = 0; longi < time; longi++) { 
    if (rorcCheckRxStatus(buff)) 
      break; 
  } 
  if (time && (longi >= time)){ 
    printf("ddlReadDiu: no status arrived in time-out %lld usec",  
	   (long long int)(time/pci_loop_per_usec)); 
    retval = -1; 
    return(retval); 
  } 
 
  stw = ddlReadStatus (buff); 
  retval = stw.stw; 
  if (stw.part.code != IFSTW || 
      stw.part.trid != transid   || 
      stw.part.dest != dest){ 
    printf("NOT DIU IFSTW! Expected: 0x00000%x%x%x, received: 0x%08lx\n", 
	   transid, IFSTW, dest, stw.stw); 
    retval = -1; 
  } 
 
  stw = ddlReadCTSTW(buff, transid, dest, time, pci_loop_per_usec);
 
  return(retval); 
} 

long ddlReadSiu(volatile void *buff, int transid,
			 long long int time, int pci_loop_per_usec){
  long retval;
  stword_t stw;
  int dest;
  long long int longi;
  __u32 param, command;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  /* prepare and send DDL command */
  
  dest = DDL_DEST_SIU;
  command = RandCIFST;
  param = 0;
  retval=ddlSendCommand(buff, dest, command, transid, param, time);
  if (retval == RORC_LINK_NOT_ON){
    printf("ddlReadSiu: SIU command can not be send since link is not on\n");
    retval = -1;
    return(retval);
  }
  if (retval == RORC_TIMEOUT){
    printf("ddlReadSiu: SIU command can not be sent in timeout %lld\n", time);
    retval = -1;
    return(retval);
  }

  /* read and check the answer */

  for (longi = 0; longi < time; longi++){
    if (rorcCheckRxStatus(buff))
      break;
  }
  if (time && (longi >= time)){
    printf("ddlReadSiu: no status arrived in time-out %lld usec\n",
                          (long long int)(pci_loop_per_usec));
    retval = -1;
    return(retval);
  }

  stw = ddlReadStatus(buff);
  if (stw.part.code != IFSTW ||
      stw.part.trid != transid   ||
      stw.part.dest != dest){
    printf("NOT SIU IFSTW! Expected: 0x00000%x%x%x, received: 0x%08lx\n",
                             transid, IFSTW, dest, stw.stw);
    retval = -1;
  }
  retval = stw.stw;

  stw = ddlReadStatus (buff);
  if ((stw.part.code != CTSTW &&
       stw.part.code != ILCMD &&
       stw.part.code != CTSTW_TO) ||
       stw.part.trid != transid   ||
       stw.part.dest != dest){
    printf( "NOT CTSTW! Expected: 0x00000%x%x%x, received: 0x%08lx\n",
	    transid, CTSTW, dest, stw.stw);
  }
  
  return(retval);
}
 
void ddlInterpretIFSTW(__u32 ifstw, char* pref,
		       char* suff, int diu_version){
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  if (diu_version == OLD) 
    ddlInterpret_OLD_IFSTW(ifstw, pref, suff); 
  else 
    ddlInterpret_NEW_IFSTW(ifstw, pref, suff); 
} 
 
void ddlInterpret_OLD_IFSTW(__u32 ifstw, char* pref, char* suff) 
/* 
 * Interpret DIU or SIU IFSTW 
 */ 
 
{ 
  int siuSend; 
  int destination; 
  unsigned long status, siuStatus; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  destination = ST_DEST(ifstw); 
  status = ifstw & STMASK; 
  if (destination == DDL_DEST_DIU){
    if (mask(status, oDIU_LOOP)) 
      printf("%sDIU is set in loop-back mode%s", pref, suff); 
    if (mask(status, ERROR_BIT)){ 
      if (strlen(pref) == 0) 
        printf("%sDIU error bit(s) set:%s", pref, suff); 
      if (mask(status, oLOSS_SIGN)) 
         printf("%s Loss of signal%s", pref, suff); 
      else{ 
        if (mask(status, oD_RTOUT)) 
          printf("%s Receiver synchronisation timeout%s", pref, suff); 
        if (mask(status, oD_LOSY)) 
          printf("%s Loss of word synchronisation%s", pref, suff); 
        if (mask(status, oD_RDERR)) 
          printf("%s Running disparity error%s", pref, suff); 
        if (mask(status, oD_INVRX)) 
          printf("%s Invalid receive word%s", pref, suff); 
        if (mask(status, oD_CERR)) 
          printf("%s CRC error%s", pref, suff); 
        if (mask(status, oD_UNREC)) 
          printf("%s Unrecognised ordered set received%s", pref, suff); 
        if (mask(status, oD_DOUT)) 
          printf("%s Data word out of frame%s", pref, suff); 
        if (mask(status, oD_IFDL)) 
          printf("%s Illegal frame delimiter%s", pref, suff); 
        if (mask(status, oD_LONG)) 
          printf("%s Too long frame%s", pref, suff); 
        if (mask(status, oD_RXOV)) 
          printf("%s Received data/status overflow%s", pref, suff); 
        if (mask(status, oD_LTOUT)) 
          printf("%s Link initialisation timeout%s", pref, suff); 
      } 
    } 
    
    if      (mask(status, DIUSTMASK) == oDIU_NOSYNC) 
      printf("%s DIU port not synchronised%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_RSTSIU) 
      printf("%s DIU port in reset SIU state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_FAIL) 
      printf("%s DIU port in fail state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_ACCED) 
      printf("%s DIU port in metastable ACCED state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_START) 
      printf("%s DIU port in metastable START state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_LRES) 
      printf("%s DIU port in metastable LRES state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oDIU_OFFL) 
      printf("%s DIU port in Off Line state%s", pref, suff); 
    else if (mask(status, DIUSTMASK) == oLINK_ACT) 
      printf("%s DIU port is in Active state%s", pref, suff); 
 
    siuStatus = status & REMMASK; 
    switch (siuStatus){ 
    case oSIU_SRST : 
        siuSend = 0; 
        break; 
      case oSIU_FAIL : 
        siuSend = 1; 
        break; 
      case oSIU_OFFL : 
        siuSend = 2; 
        break; 
      case oSIU_LINIT : 
        siuSend = 3; 
        break; 
      case oSIU_ACT : 
        siuSend = 4; 
        break; 
      case oSIU_XOFF : 
        siuSend = 5; 
        break; 
      case oSIU_XON : 
        siuSend = 6; 
        break; 
      case oSIU_ELSE : 
        siuSend = 7; 
        break; 
      default : 
        siuSend = 8; 
    } 
    printf("%sDIU port receives %s%s", pref, receivedOrderedSet[siuSend], suff); 
  } 
  else  /* SIU */ 
  { 
    if (mask(status, ERROR_BIT)){ 
      if (strlen(pref) == 0) 
        printf("%sSIU error bit(s) set:%s", pref, suff); 
      if (mask(status, oS_LONGE)) 
        printf("%s Too long event or read data block%s", pref, suff); 
      if (mask(status, oS_IFEDS)) 
        printf("%s Illegal FEE data/status%s", pref, suff); 
      if (mask(status, oS_TXOF)) 
        printf("%s Transmit FIFO overflow%s", pref, suff); 
      if (mask(status, oS_IWDAT)) 
        printf("%s Illegal write data word%s", pref, suff); 
      if (mask(status, oS_WBLER)) 
        printf("%s Write data block length error%s", pref, suff); 
      if (mask(status, oS_RXOV)) 
        printf("%s Receive FIFO overflow%s", pref, suff); 
      if (mask(status, oS_LONGD)) 
        printf("%s Too long data frame%s", pref, suff); 
      if (mask(status, oS_LONGC)) 
        printf("%s Too long command frame%s", pref, suff); 
      if (mask(status, oS_OSIN)) 
        printf("%s Ordered set inside a frame%s", pref, suff); 
      if (mask(status, oS_DOUT)) 
        printf("%s Data out of receive frame%s", pref, suff); 
      if (mask(status, oS_LPERR)) 
        printf("%s Link protocol error%s", pref, suff); 
      if (mask(status, oS_CHERR)) 
        printf("%s Check summ error in receive frame%s", pref, suff); 
      if (mask(status, oS_UNREC)) 
        printf("%s Unrecognised ordered set%s", pref, suff); 
      if (mask(status, oS_INVRX)) 
        printf("%s Invalid receive word%s", pref, suff); 
      if (mask(status, oS_WALER)) 
        printf("%s Word alignment error%s", pref, suff); 
      if (mask(status, oS_ISPCH)) 
        printf("%s Illegal special character%s", pref, suff); 
      if (mask(status, oS_RDERR)) 
        printf("%s Running disparity error%s", pref, suff); 
      if (mask(status, oS_IRXCD)) 
        printf("%s Illegal receive code%s", pref, suff); 
      if (mask(status, oS_BUFER)) 
        printf("%s Elastic buffer over/under run%s", pref, suff); 
    } 
    else 
      printf("%sSIU error bit not set, SIU is in normal state%s", pref, suff); 
  } 
} 
 
void ddlInterpret_NEW_IFSTW(__u32 ifstw, char *pref, char *suff) 
/* 
 * Interpret DIU or SIU IFSTW 
 */  
 
{ 
  int destination; 
  int siuStatus; 
  unsigned long status; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  destination = ST_DEST(ifstw); 
 
  status = ifstw & STMASK; 
  if (destination == DDL_DEST_DIU) {
    if (mask(status, DIU_LOOP)) 
      printf("%sDIU is set in loop-back mode%s", pref, suff); 
    if (mask(status, ERROR_BIT)){ 
      if (strlen(pref) == 0) 
        printf("%sDIU error bit(s) set:%s", pref, suff); 
      if (mask(status, LOSS_SYNC)) 
        printf("%s Loss of synchronization%s", pref, suff); 
      if (mask(status, D_TXOF)) 
        printf("%s Transmit data/status overflow%s", pref, suff); 
      if (mask(status, D_RES1)) 
        printf("%s Undefined DIU error%s", pref, suff); 
      if (mask(status, D_OSINFR)) 
        printf("%s Ordered set in frame%s", pref, suff); 
      if (mask(status, D_INVRX)) 
        printf("%s Invalid receive character in frame%s", pref, suff); 
      if (mask(status, D_CERR)) 
        printf("%s CRC error%s", pref, suff); 
      if (mask(status, D_RES2)) 
        printf("%s Undefined DIU error%s", pref, suff); 
      if (mask(status, D_DOUT)) 
        printf("%s Data out of frame%s", pref, suff); 
      if (mask(status, D_IFDL)) 
        printf("%s Illegal frame delimiter%s", pref, suff); 
      if (mask(status, D_LONG)) 
        printf("%s Too long frame%s", pref, suff); 
      if (mask(status, D_RXOF)) 
        printf("%s Received data/status overflow%s", pref, suff); 
      if (mask(status, D_FRERR)) 
        printf("%s Error in receive frame%s", pref, suff); 
    } 
 
    switch (mask(status, DIUSTMASK)){ 
      case DIU_TSTM: 
        printf("%sDIU port in PRBS Test Mode state%s", pref, suff); break; 
      case DIU_POFF: 
        printf("%sDIU port in Power Off state%s", pref, suff); break; 
      case DIU_LOS: 
        printf("%sDIU port in Offline Loss of Synchr. state%s", pref, suff); break; 
      case DIU_NOSIG: 
        printf("%sDIU port in Offline No Signal state%s", pref, suff); break; 
      case DIU_WAIT: 
        printf("%sDIU port in Waiting for Power Off state%s", pref, suff); break; 
      case DIU_ONL: 
        printf("%sDIU port in Online state%s", pref, suff); break; 
      case DIU_OFFL: 
        printf("%sDIU port in Offline state%s", pref, suff); break; 
      case DIU_POR: 
        printf("%sDIU port in Power On Reset state%s", pref, suff); break; 
    } 
 
    siuStatus = (status & REMMASK) >> 15; 
    printf("%sremote SIU/DIU port in %s state%s", pref, remoteStatus[siuStatus], suff); 
  } 
  else  /* SIU */ 
  { 
    if (mask(status, ERROR_BIT)){ 
      if (strlen(pref) == 0) 
        printf("%sSIU error bit(s) set:%s", pref, suff); 
      if (mask(status, S_LONGE)) 
        printf("%s Too long event or read data block%s", pref, suff); 
      if (mask(status, S_IFEDS)) 
        printf("%s Illegal FEE data/status%s", pref, suff); 
      if (mask(status, S_TXOF)) 
        printf("%s Transmit FIFO overflow%s", pref, suff); 
      if (mask(status, S_IWDAT)) 
        printf("%s Illegal write data word%s", pref, suff); 
      if (mask(status, S_OSINFR)) 
        printf("%s Ordered set in frame%s", pref, suff); 
      if (mask(status, S_INVRX)) 
        printf("%s Invalid character in receive frame%s", pref, suff); 
      if (mask(status, S_CERR)) 
        printf("%s CRC error%s", pref, suff); 
      if (mask(status, S_DJLERR)) 
        printf("%s DTCC or JTCC error%s", pref, suff); 
      if (mask(status, S_DOUT)) 
        printf("%s Data out of receive frame%s", pref, suff); 
      if (mask(status, S_IFDL)) 
        printf("%s Illegal frame delimiter%s", pref, suff); 
      if (mask(status, S_LONG)) 
        printf("%s Too long receive frame%s", pref, suff); 
      if (mask(status, S_RXOF)) 
        printf("%s Receive FIFO overflow%s", pref, suff); 
      if (mask(status, S_FRERR)) 
        printf("%s Error in receive frame%s", pref, suff); 
      if (mask(status, S_LPERR)) 
        printf("%s Link protocol error%s", pref, suff); 
    } 
    else 
      printf("%sSIU error bit not set%s", pref, suff); 
 
    if (mask(status, S_LBMOD)) 
      printf("%sSIU in Loopback Mode%s", pref, suff); 
    if (mask(status, S_OPTRAN)) 
      printf("%sOne FEE transaction is open%s", pref, suff); 
 
    if      (mask(status, SIUSTMASK) == SIU_RESERV) { 
      printf("%sSIU port in undefined state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_POFF) { 
      printf("%sSIU port in Power Off state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_LOS) { 
      printf("%sSIU port in Offline Loss of Synchr. state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_NOSIG) { 
      printf("%sSIU port in Offline No Signal state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_WAIT ) { 
      printf("%sSIU port in Waiting for Power Off state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_ONL ) { 
      printf("%sSIU port in Online state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_OFFL) { 
      printf("%sSIU port in Offline state%s", pref, suff); 
    } else if (mask(status, SIUSTMASK) == SIU_POR) { 
      printf("%sSIU port in Power On Reset state%s", pref, suff); 
    } 
  } 
} 
 
unsigned long ddlResetSiu(volatile void *buff, int print, int cycle,
			  long long int time, int diu_version, int pci_loop_per_usec)
 
/* ddlResetSiu tries to reset the SIU. 
 *             print	# if != 0 then print link status 
 *             cycle	# of status checks 
 *             time	# of cycles to wait for command sending and replies 
 * Returns:    SIU status word or -1 if no status word can be read 
 */ 
 
{ 
  int i, retval, transid; 
  long long longret; 
  int diu_ok, siu_ok, trial; 
  long retlong;
  stword_t stw; 
  char* pref=""; 
  char* suff="\n"; 
  DEBUG_PRINTF(PDADEBUG_ENTER, ""); 
  retval = ddlSendCommand(buff, DDL_DEST_DIU, SRST, 0,  0, time);
 
  if (retval == RORC_LINK_NOT_ON){ 
    if (print) 
      printf("SIU reset can not be sent because the link is not on\n"); 
    return (-1); 
  } 
  else if (retval == RORC_TIMEOUT){ 
    if (print) 
      printf("SIU reset can not be sent out in timeout %lld", time); 
    return (-1); 
  } 
  else{ 
    longret = ddlWaitStatus(buff, time); 
    if (longret >= time){ 
      if (print) 
        printf("SIU reset: No reply arrived in timeout %lld", time); 
      return (-1); 
    } 
    else{ 
      stw = ddlReadStatus(buff); 
      if (print) 
        printf("SIU reset: reply = 0x%08lx\n", stw.stw); 
    } 
  } 
 
  if ((diu_version != NEW) && (diu_version != EMBEDDED)) 
    return (-0); 
  
  trial = cycle; 
  transid = 0xf; 
  retlong = -1; 
  i = 0; 
 
  while (trial){ 
    usleep(10000);   // sleep 10 msec 
 
    i++; 
    if (print) 
      printf("Cycle #%d:", i); 
 
    diu_ok = 0; 
    siu_ok = 0; 
 
    transid = incr15(transid); 
    retlong = ddlReadDiu(buff, transid, time, pci_loop_per_usec);
    if (retlong == -1){ 
      if (print) { 
        printf(" ddlReadDiu returns -1: Error in ddlReadDiu "); 
      } 
      trial--;  
      continue; 
    } 
 
    retlong &= STMASK; 
    if (print){ 
      //  printf(" RORC %d/%d. DIU status word: 0x%08lx ", 
      //                  prorc->minor, prorc->ddl_channel, retlong); 
    } 
 
    if (retlong & ERROR_BIT){ 
      if (print){ 
	// printf(" DIU error at RORC /dev/prorc %d/%d. Status word: 0x%0lx ", 
	//                 prorc->minor, prorc->ddl_channel, retlong); 
        // // statInfo[0] = '\0';
        ddlInterpretIFSTW(retlong, pref, suff, diu_version);
        // printf("%s", statInfo);
      } 
    } 
    else if (mask(retlong, S_OPTRAN)){ 
      if (print){ 
	//  printf(" SIU transaction open at RORC /dev/prorc %d/%d. Status word: 0x%0lx ", 
	//                  prorc->minor, prorc->ddl_channel, retlong); 
        // // statInfo[0] = '\0';
        ddlInterpretIFSTW(retlong, pref, suff, diu_version);
        // printf("%s", statInfo);
      } 
    } 
    else 
      diu_ok = 1; 
 
    transid = incr15(transid); 
    retlong = ddlReadSiu(buff, transid, time, pci_loop_per_usec);
    if (retlong == -1){ 
      if (print){ 
        printf(" ddlReadSiu returns -1: Error in ddlReadSiu "); 
      } 
      trial--; 
      continue; 
    } 
 
    retlong &= STMASK; 
    if (print){ 
      // printf(" RORC %d/%d. SIU status word: 0x%08lx ", 
      //                 prorc->minor, prorc->ddl_channel, retlong); 
    } 
 
    if (retlong & ERROR_BIT){ 
      if (print){ 
	// printf(" SIU error at RORC /dev/prorc %d/%d. Status word: 0x%0lx ", 
	//                 prorc->minor, prorc->ddl_channel, retlong); 
        // // statInfo[0] = '\0';
        ddlInterpretIFSTW(retlong, pref, suff, diu_version);
        // printf("%s", statInfo);
      } 
    } 
    else 
      siu_ok = 1; 
 
    if (diu_ok && siu_ok) 
      return (retlong); 
    
    trial--; 
  } 
 
  if (print) 
    printf(" Too many trials"); 
  return(retlong);  
}

unsigned long ddlLinkUp(volatile void *buff, int master, int print, int stop,
			long long int time, int diu_version, int pci_loop_per_usec) {
  unsigned long retlong;
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  if (diu_version == OLD) {
    retlong = ddlLinkUp_OLD(buff, master, print, stop, time, diu_version, pci_loop_per_usec);
  } else {
    retlong = ddlLinkUp_NEW(buff, master, print, stop, time, diu_version, pci_loop_per_usec);
  }

  return (retlong);
}

unsigned long ddlLinkUp_OLD(volatile void *buff, int master, int print,
			    int stop, long long int time, int diu_version, int pci_loop_per_usec)

/* ddlLinkUp tries to reset and initialize (if master) the DDL link.
 * Parameter:  master   if !=0 then tries to initialize the link,
 *                      otherwise only reset the link
 *             print    # of link status prints (-1 means: no limit)
 *             stop     if !=0 returns when active state or print limit
 *                      (if print > 0) is reached
 *             time     # of cycles to wait for command sending and replies
 * Returns:    DIU status word or -1 if no status word can be read
 *             The DIU status word reflects the status of the link after
 *             resetting it. 
 *             Important bits of the status word are
 *             bit 31   must be 0, otherwise one of the error bits is set
 *             bit 29   must be 0, otherwise there is no signal on the line
 *             if bits 17,16,15 are 1 1 0 the other end is in failure state
 *             if bits 14,13,12 are 1 0 0 it means DIU is in off_line state
 *                            or    0 0 0 which means the link is active.
 */

{
  int retval, siuSend, transid;
  long retlong;
  long last_stw;
  long siuStatus;
  stword_t stw;
  char pref[13] = "DIU status: ";
  char suff[2] = "\n";
  DEBUG_PRINTF(PDADEBUG_ENTER, "");
  last_stw = 0xf00; // Not possible value
  transid = 0xf;
  do{
    transid = incr15(transid);
    retlong = ddlReadDiu(buff, transid, time, pci_loop_per_usec);
    if (retlong == -1){
      if (last_stw != retlong){
        if (print){
          printf(" ddlReadDiu returns -1: Error in ddlReadDiu");
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return (retlong);
            }
          }
        }
        if (stop)
          return (retlong);
        last_stw = retlong;
      }
      continue;
    }

    retlong &= STMASK;
    siuStatus = retlong & REMMASK;
    switch (siuStatus)
    {
      case oSIU_SRST :
        siuSend = 0;
        break;
      case oSIU_FAIL :
        siuSend = 1;
        break;
      case oSIU_OFFL :
        siuSend = 2;
        break;
      case oSIU_LINIT :
        siuSend = 3;
        break;
      case oSIU_ACT :
        siuSend = 4;
        break;
      case oSIU_XOFF :
        siuSend = 5;
        break;
      case oSIU_XON :
        siuSend = 6;
        break;
      case oSIU_ELSE :
        siuSend = 7;
        break;
      default :
        siuSend = 8;
    }

    if (retlong & ERROR_BIT){
      if (last_stw != retlong){
        if (print){
	  // printf(" DIU error at pRORC /dev/prorc %d/%d. Status word: 0x%0lx",
	  //                  prorc->minor, prorc->ddl_channel, retlong);
          // statInfo[0] = '\0';
          ddlInterpretIFSTW(retlong, pref, suff, diu_version);
          // printf("%s", statInfo);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
      retlong = mask(retlong, ~DIUERMASK);
    }

    if (retlong & oLOSS_SIGN){
      if (!(last_stw & oLOSS_SIGN)){
        if (print){
          printf(" Loss of signal. Status word: 0x%0lx", retlong);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_NOSYNC){
      if (last_stw != retlong){
        if (print){
          printf(" Receiver not synchronised. Status word: 0x%0lx", retlong);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_RSTSIU){
      if (last_stw != retlong){
        if (print){
          printf(" DIU in reset SIU state. Status word: 0x%0lx", retlong);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_FAIL){
      if (last_stw != retlong){
        if (print){
          printf(" DIU port in fail state, receiving %s",
		 receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }

      // Send reset command to DIU 
      retval = ddlSendCommand(buff, DDL_DEST_DIU, LRST, transid ,0, time);
      transid = incr15(transid);
      if (retval == RORC_TIMEOUT)
        if (print){
          printf("Error: timeout is over for LRST");
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
      
      // Read status FIFO 
      stw = ddlReadStatus(buff);
      printf("The LRST returned status: %8lx\n", stw.stw);
    }
    else if ((retlong & DIUSTMASK) == oDIU_ACCED){
      if (last_stw != retlong){
        if (print){
          printf(" DIU port in metastable ACCED state, receiving %s",
		 receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_START){
      if (last_stw != retlong){
        if (print){
          printf(" DIU port in metastable START state, receiving %s",
		 receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_LRES){
      if (last_stw != retlong){
        if (print){
          printf(" DIU port in metastable LRES state, receiving %s",
		 receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
    }
    else if ((retlong & DIUSTMASK) == oDIU_OFFL){
      if (last_stw != retlong){
        if (print){
          //printf(" pRORC /dev/prorc %d/%d, is Off Line, receiving %s",
          //       prorc->minor, prorc->ddl_channel, receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
      }

     if ((retlong & REMMASK) == oSIU_FAIL){
        if (last_stw != retlong){
          if (print){
            printf(" The remote DIU or SIU is in fail state");
            if (print > 0){
              print--;
              if (print == 0){
                printf(" Too many messages");
                if (stop) return(retlong);
              }
            }
          }
        }
        last_stw = retlong;
      }
      else if (master){
        last_stw = retlong;
        // Send a command to DIU to initialize 
        retval=ddlSendCommand(buff, DDL_DEST_DIU, LINIT, transid, 0, time);
        transid = incr15(transid);

        if (retval == RORC_TIMEOUT)
          if (print){
            printf("Error: timeout is over for LINIT");
            if (print > 0){
              print--;
              if (print == 0){
                printf(" Too many messages");
                if (stop) return(retlong);
              }
            }
          }
	
        // Read status FIFO 
        stw = ddlReadStatus(buff);
        printf("The LINIT returned status: %8lx\n", stw.stw);
        continue;
      }
     // not master 
     last_stw = retlong;

     // if (stop) return (retlong); 
    }
    else if ((retlong & DIUMASK) == oLINK_ACT){
      if (last_stw != retlong){
        if (print){
	  // printf(" pRORC /dev/prorc %d/%d Active, receiving %s",
	  //       prorc->minor, prorc->ddl_channel, receivedOrderedSet[siuSend]);
          if (print > 0){
            print--;
            if (print == 0){
              printf(" Too many messages");
              if (stop) return(retlong);
            }
          }
        }
        last_stw = retlong;
      }
      if (stop)
        return (retlong);
    }
  } while (1);

  return (retlong);
}

unsigned long ddlLinkUp_NEW(volatile void *buff, int master, int print,
			    int stop, long long int time, int diu_version, int pci_loop_per_usec)

/* ddlLinkUp tries to reset and initialize (if master) the DDL link.
 * Parameter:  master   if !=0 then tries to initialize the link,
 *                      otherwise only reset the link
 *             print	# of link status prints (-1 means: no limit)
 *             stop	if !=0 returns when active state or print limit
 *                      (if print > 0) is reached
 *             time	# of cycles to wait for command sending and replies
 * Returns:    DIU status word or -1 if no status word can be read
 *             The DIU status word reflects the status of the link after
 *             resetting it. 
 *             Important bits of the status word are
 *             bit 31	must be 0, otherwise one of the error bits is set
 *             if bits 17,16,15 are 1 1 0 the remote SIU or DIU  does not see us 
 *             if bits 14,13,12 are 1 1 0 it means DIU is in power off state,
 *                                  1 0 0 means DIU receives no signal,
 *                            or    0 1 0 which means the link is active.
 */

{
DEBUG_PRINTF(PDADEBUG_ENTER, "");
#define PRINTEND \
      if (print > 0) \
      { \
        print--; \
        if (print == 0) \
        { \
          printf(" Too many messages"); \
          if (stop) return(retlong); \
        } \
      } \
    }

#define PRINT_IF_NEW(a) \
    if (print) \
    { \
      printf(a); \
      printf(" remote SIU/DIU in %s state ", remoteStatus[siuStatus]); \
    PRINTEND

  int siuStatus;
  int transid;
  long retlong;
  long last_stw;
  stword_t stw;
  char pref[13] = "DIU status: ";
  char suff[2] = "\n";

  //  printf ("ddlLinkUp called");
  last_stw = 0xf00; // Not possible value
  transid = 0xf;
  do{
    transid = incr15(transid);
    retlong = ddlReadDiu(buff, transid, time, pci_loop_per_usec);
    if (retlong == -1){
      if (last_stw != retlong){
        if (print){
          printf("-ddlReadDiu returns -1: Error in ddlReadDiu ");
	  PRINTEND
	    last_stw = retlong;
	}
	continue;
      }

      retlong &= STMASK;
      if (last_stw != retlong){
	if (print){
	  //  printf("-pRORC%d. DIU status word: 0x%08lx ",
	  //                  prorc->minor, retlong);
	  PRINTEND
	    last_stw = retlong;
	}
	else
	  continue;
	
    siuStatus = (retlong & REMMASK) >> 15;
    
    if (retlong & ERROR_BIT){
      if (print){
	//  printf(" DIU error at pRORC /dev/prorc %d/%d. Status word: 0x%0lx ",
	//                  prorc->minor, prorc->ddl_channel, retlong);
        // statInfo[0] = '\0';
        ddlInterpretIFSTW(retlong, pref, suff, diu_version);
        // printf("%s", statInfo);
	PRINTEND
	continue;
      }

      switch (retlong & DIUSTMASK)
	{
	case DIU_WAIT:
	  PRINT_IF_NEW(" DIU port in Waiting for Power Off state"); break;
	case DIU_LOS:
	  PRINT_IF_NEW(" DIU port in Offline Loss of Synchr. state"); break;
	case DIU_NOSIG:
	  PRINT_IF_NEW(" DIU port in Offline No Signal state"); break;
	case DIU_TSTM:
	  PRINT_IF_NEW(" DIU in PRBS Test Mode state"); break;
	case DIU_OFFL:
	  PRINT_IF_NEW(" DIU port in Offline state"); break;
	case DIU_POR:
	  PRINT_IF_NEW(" DIU port in Power On Reset state"); break;
	case DIU_POFF:
	  PRINT_IF_NEW(" DIU port in Power Off state");
	  if (master)
	    {
	      // Send a command to DIU to wake up
	      /*retval=*/ddlSendCommand(buff, DDL_DEST_DIU, WAKEUP, transid, 0, time);
	      transid = incr15(transid);
	      
	      // Read status FIFO 
	      stw = ddlReadStatus(buff);
	      printf("The WAKEUP returned status: %8lx\n", stw.stw);
	      continue;
	    }
	  // not master 
	  // if (stop) return (retlong); 
	  break;
	case DIU_ONL:
	  PRINT_IF_NEW(" DIU port in Online state");
	  if (stop) return (retlong);
	  break;
	}
    } while (1);

    return (retlong);   
}

int ddlSetSiuLoopBack(volatile void *buff, long long int timeout, int pci_loop_per_usec, stword_t *stw){
  int ret;
  long long int longret;
  long retlong;

  /* check SIU fw version */

  ret = ddlSendCommand(buff, DDL_DEST_SIU, IFLOOP, 0, 0, timeout);
  if (ret == RORC_LINK_NOT_ON)
    return (ret);
  if (ret == RORC_TIMEOUT)
    return RORC_STATUS_ERROR;

  longret = ddlWaitStatus(buff, timeout);
  if (longret >= timeout)
    return RORC_NOT_ACCEPTED;
  
  *stw = ddlReadStatus(buff);
  if (stw->part.code == ILCMD){
    /* illegal command => old version => send TSTMODE for loopback */
    ret = ddlSendCommand(buff, DDL_DEST_SIU, TSTMODE, 0, 0, timeout);
    if (ret == RORC_LINK_NOT_ON)
      return (ret);
    if (ret == RORC_TIMEOUT)
      return RORC_STATUS_ERROR;

    longret = ddlWaitStatus(buff, timeout);
    if (longret >= timeout)
      return RORC_NOT_ACCEPTED;
    else
      return RORC_STATUS_OK;
  }
  if (stw->part.code != CTSTW)
    return RORC_STATUS_ERROR;

  /* SIU loopback command accepted => check SIU loopback status */
  retlong = ddlReadSiu(buff, 0, timeout, pci_loop_per_usec);
  if (retlong == -1)
    return RORC_STATUS_ERROR;

  if (retlong & S_LBMOD)
    return RORC_STATUS_OK; // SIU loopback set 

  /* SIU loopback not set => set it */
  ret = ddlSendCommand(buff, DDL_DEST_SIU, IFLOOP, 0, 0, timeout);
  if (ret == RORC_LINK_NOT_ON)
    return (ret);
  if (ret == RORC_TIMEOUT)
    return RORC_STATUS_ERROR;

  longret = ddlWaitStatus(buff, timeout);
  if (longret >= timeout)
    return RORC_NOT_ACCEPTED;
  else
    *stw = ddlReadStatus(buff);

  return RORC_STATUS_OK;

}

int ddlDiuLoopBack(uint32_t *buff, long long int timeout, stword_t *stw){
  int ret;
  long long int longret;

  ret = ddlSendCommand(buff, DDL_DEST_DIU, IFLOOP, 0, 0, timeout);
  if (ret == RORC_TIMEOUT)
    return RORC_STATUS_ERROR;

  longret = ddlWaitStatus(buff, timeout);
  if (longret >= timeout)
    return RORC_NOT_ACCEPTED;
  else
    *stw = ddlReadStatus(buff);

  return RORC_STATUS_OK;
}
