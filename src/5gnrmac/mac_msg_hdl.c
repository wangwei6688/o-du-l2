/*******************************************************************************
################################################################################
#   Copyright (c) [2017-2019] [Radisys]                                        #
#                                                                              #
#   Licensed under the Apache License, Version 2.0 (the "License");            #
#   you may not use this file except in compliance with the License.           #
#   You may obtain a copy of the License at                                    #
#                                                                              #
#       http://www.apache.org/licenses/LICENSE-2.0                             #
#                                                                              #
#   Unless required by applicable law or agreed to in writing, software        #
#   distributed under the License is distributed on an "AS IS" BASIS,          #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
#   See the License for the specific language governing permissions and        #
#   limitations under the License.                                             #
################################################################################
 *******************************************************************************/

/* header include files -- defines (.h)  */
#include "common_def.h"
#include "lrg.h"
#include "rgu.h"
#include "lrg.x"
#include "rgu.x"
#include "du_app_mac_inf.h"
#include "mac_sch_interface.h"
#include "mac_upr_inf_api.h"
#include "lwr_mac.h"
#ifdef INTEL_FAPI
#include "fapi_interface.h"
#endif
#include "lwr_mac_fsm.h"
#include "lwr_mac_upr_inf.h"
#include "mac.h"
#include "mac_utils.h"

/* This file contains message handling functionality for MAC */

extern MacCb  macCb;

extern void sendToLowerMac(uint16_t msgType, uint32_t msgLen, void *msg);
uint16_t buildMacPdu(RlcMacData *dlData);

/* Function pointer for sending crc ind from MAC to SCH */
MacSchCrcIndFunc macSchCrcIndOpts[]=
{
   packMacSchCrcInd,
   MacSchCrcInd,
   packMacSchCrcInd
};

/* Function pointer for sending DL RLC BO Info from MAC to SCH */
MacSchDlRlcBoInfoFunc macSchDlRlcBoInfoOpts[]=
{
   packMacSchDlRlcBoInfo,
   MacSchDlRlcBoInfo,
   packMacSchDlRlcBoInfo
};

/* Function pointer for sending short BSR from MAC to SCH */
MacSchBsrFunc macSchBsrOpts[]=
{
   packMacSchBsr,
   MacSchBsr,
   packMacSchBsr
};

/* Function pointer for sending SR Uci ind from MAC to SCH */
MacSchSrUciIndFunc macSchSrUciIndOpts[]=
{
   packMacSchSrUciInd,
   MacSchSrUciInd,
   packMacSchSrUciInd
};

/*******************************************************************
 *
 * @brief Sends DL BO Info to SCH
 *
 * @details
 *
 *    Function : sendDlRlcBoInfoMacToSch
 *
 *    Functionality:
 *       Sends DL BO Info to SCH
 *
 * @params[in] 
 * @return ROK     - success
 *         RFAILED - failure
 *
 ****************************************************************/
uint8_t sendDlRlcBoInfoMacToSch(DlRlcBOInfo *dlBoInfo)
{
   Pst pst;

   FILL_PST_MAC_TO_SCH(pst, EVENT_DL_RLC_BO_INFO_TO_SCH);
   return(*macSchDlRlcBoInfoOpts[pst.selector])(&pst, dlBoInfo);
}

/*******************************************************************
 *
 * @brief Sends CRC Indication to SCH
 *
 * @details
 *
 *    Function : sendCrcIndMacToSch 
 *
 *    Functionality:
 *       Sends CRC Indication to SCH
 *
 * @params[in] 
 * @return ROK     - success
 *         RFAILED - failure
 *
 ****************************************************************/
uint8_t sendCrcIndMacToSch(CrcIndInfo *crcInd)
{
   Pst pst;

   FILL_PST_MAC_TO_SCH(pst, EVENT_CRC_IND_TO_SCH);
   return(*macSchCrcIndOpts[pst.selector])(&pst, crcInd);
}

/*******************************************************************
 *
 * @brief Processes CRC Indication from PHY
 *
 * @details
 *
 *    Function : fapiMacCrcInd
 *
 *    Functionality:
 *       Processes CRC Indication from PHY
 *
 * @params[in] Post Structure Pointer
 *             Crc Indication Pointer
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t fapiMacCrcInd(Pst *pst, CrcInd *crcInd)
{
   uint16_t     cellIdx;
   CrcIndInfo   crcIndInfo;

   DU_LOG("\nMAC : Received CRC indication");

   GET_CELL_IDX(crcInd->cellId, cellIdx);
   /* Considering one pdu and one preamble */ 
   crcIndInfo.cellId = macCb.macCell[cellIdx]->cellId;;
   crcIndInfo.crnti = crcInd->crcInfo[0].rnti;
   crcIndInfo.timingInfo.sfn = crcInd->timingInfo.sfn;
   crcIndInfo.timingInfo.slot = crcInd->timingInfo.slot;
   crcIndInfo.numCrcInd = crcInd->crcInfo[0].numCb;
   crcIndInfo.crcInd[0] = crcInd->crcInfo[0].cbCrcStatus[0];

   return(sendCrcIndMacToSch(&crcIndInfo));
}

/*******************************************************************
 *
 * @brief Process Rx Data Ind at MAC
 *
 * @details
 *
 *    Function : fapiMacRxDataInd
 *
 *    Functionality:
 *       Process Rx Data Ind at MAC
 *
 * @params[in] Post structure
 *             Rx Data Indication
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t fapiMacRxDataInd(Pst *pst, RxDataInd *rxDataInd)
{
   uint16_t pduIdx;

   DU_LOG("\nMAC : Received Rx Data indication");

   /* TODO : compare the handle received in RxDataInd with handle send in PUSCH
    * PDU, which is stored in raCb */

   for(pduIdx = 0; pduIdx < rxDataInd->numPdus; pduIdx++)
   {
      unpackRxData(rxDataInd->cellId, rxDataInd->timingInfo, &rxDataInd->pdus[pduIdx]);
   }
   return ROK;
}

/*******************************************************************
 *
 * @brief Processes DL data from RLC
 *
 * @details
 *
 *    Function : MacRlcProcDlData 
 *
 *    Functionality:
 *      Processes DL data from RLC
 *
 * @params[in] Post structure
 *             DL data
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint16_t MacRlcProcDlData(Pst* pst, SpId spId, RlcMacData *dlData)
{
   return ROK;
}

/*******************************************************************
 *
 * @brief Builds and Sends UL Data to RLC
 *
 * @details
 *
 *    Function : macProcUlData
 *
 *    Functionality: Builds and Sends UL Data to RLC
 *
 * @params[in] CellId
 *             CRNTI
 *             Slot information
 *             LC Id on which payload was received
 *             Pointer to the payload
 *             Length of payload
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t macProcUlData(uint16_t cellId, uint16_t rnti, SlotIndInfo slotInfo, \
uint8_t lcId, uint16_t pduLen, uint8_t *pdu)
{
   Pst         pst;
   RlcMacData  *ulData;

   /* Filling RLC Ul Data*/
   MAC_ALLOC_SHRABL_BUF(ulData, sizeof(RlcMacData));
   if(!ulData)
   {
      DU_LOG("\nMAC : Memory allocation failed while sending UL data to RLC");
      return RFAILED;
   }
   memset(ulData, 0, sizeof(RlcMacData));
   ulData->cellId = cellId; 
   ulData->rnti = rnti;
   memcpy(&ulData->slotInfo, &slotInfo, sizeof(SlotIndInfo));
   ulData->slotInfo.cellId = cellId;

   /* Filling pdu info */
   if(lcId == SRB1_LCID || lcId == SRB2_LCID)
   {
      ulData->pduInfo[ulData->numPdu].commCh = true;
   }
   ulData->pduInfo[ulData->numPdu].lcId = lcId;
   ulData->pduInfo[ulData->numPdu].pduBuf = pdu;
   ulData->pduInfo[ulData->numPdu].pduLen = pduLen;
   ulData->numPdu++;

   /* Filling Post and send to RLC */
   memset(&pst, 0, sizeof(Pst));
   FILL_PST_MAC_TO_RLC(pst, 0, EVTRLCULDAT);
   MacSendUlDataToRlc(&pst, ulData);

   return ROK;
}


/*******************************************************************
 *
 * @brief Processes BO status from RLC
 *
 * @details
 *
 *    Function : MacRlcProcBOStatus
 *
 *    Functionality:
 *      Processes BO status from RLC
 *
 * @params[in] Post structure
 *             BO status
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint16_t MacRlcProcBOStatus(Pst* pst, SpId spId, RlcMacBOStatus*      boStatus)
{
   return ROK;
}


/*******************************************************************
 *
 * @brief Handles cell start reuqest from DU APP
 *
 * @details
 *
 *    Function : MacProcCellStartReq
 *
 *    Functionality:
 *      Handles cell start reuqest from DU APP
 *
 * @params[in] Post structure pointer
 *             Cell Start Request info pointer 
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t MacProcCellStartReq(Pst *pst, MacCellStartInfo  *cellStartInfo)
{
   DU_LOG("\nMAC : Handling cell start request");
   sendToLowerMac(START_REQUEST, 0, cellStartInfo);

   MAC_FREE_SHRABL_BUF(pst->region, pst->pool, cellStartInfo, \
	 sizeof(MacCellStartInfo));

   return ROK;
}

/*******************************************************************
 *
 * @brief Handles cell stop request from DU APP
 *
 * @details
 *
 *    Function : MacProcCellStartReq
 *
 *    Functionality:
 *        Handles cell stop request from DU APP
 *
 * @params[in] Post structure pointer
 *             Mac Cell stop information
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t MacProcCellStopReq(Pst *pst, MacCellStopInfo  *cellStopInfo)
{
#ifdef INTEL_FAPI
   DU_LOG("\nMAC : Sending cell stop request to Lower Mac");
   sendToLowerMac(FAPI_STOP_REQUEST, 0, cellStopInfo);
#endif

   MAC_FREE_SHRABL_BUF(pst->region, pst->pool, cellStopInfo, \
	 sizeof(MacCellStopInfo));

   return ROK;
}

/*******************************************************************
 *
 * @brief Handles DL CCCH Ind from DU APP
 *
 * @details
 *
 *    Function : MacProcDlCcchInd 
 *
 *    Functionality:
 *      Handles DL CCCH Ind from DU APP
 *
 * @params[in] Post structure pointer
 *             DL CCCH Ind pointer 
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t MacProcDlCcchInd(Pst *pst, DlCcchIndInfo *dlCcchIndInfo)
{
   uint16_t cellIdx;
   uint16_t idx;
   DlRlcBOInfo  dlBoInfo;
   memset(&dlBoInfo, 0, sizeof(DlRlcBOInfo));

   DU_LOG("\nMAC : Handling DL CCCH IND");

   GET_CELL_IDX(dlCcchIndInfo->cellId, cellIdx);

   dlBoInfo.cellId = dlCcchIndInfo->cellId;
   dlBoInfo.crnti = dlCcchIndInfo->crnti;
   dlBoInfo.numLc = 0;

   if(dlCcchIndInfo->msgType == RRC_SETUP)
   {
      dlBoInfo.boInfo[dlBoInfo.numLc].lcId = SRB_ID_0;    // SRB ID 0 for msg4
      dlBoInfo.boInfo[SRB_ID_0].dataVolume = \
         dlCcchIndInfo->dlCcchMsgLen;
      dlBoInfo.numLc++;

      /* storing Msg4 Pdu in raCb */
      if(macCb.macCell[cellIdx]->macRaCb[0].crnti == dlCcchIndInfo->crnti)
      {
	 macCb.macCell[cellIdx]->macRaCb[0].msg4PduLen = dlCcchIndInfo->dlCcchMsgLen;
	 MAC_ALLOC(macCb.macCell[cellIdx]->macRaCb[0].msg4Pdu, \
	    macCb.macCell[cellIdx]->macRaCb[0].msg4PduLen);
	 if(macCb.macCell[cellIdx]->macRaCb[0].msg4Pdu)
	 {
	    for(idx = 0; idx < dlCcchIndInfo->dlCcchMsgLen; idx++)
	    {
	       macCb.macCell[cellIdx]->macRaCb[0].msg4Pdu[idx] =\
	          dlCcchIndInfo->dlCcchMsg[idx];
	    }
	 }
      }
   }
   sendDlRlcBoInfoMacToSch(&dlBoInfo);

   MAC_FREE_SHRABL_BUF(pst->region, pst->pool, dlCcchIndInfo->dlCcchMsg, \
	 dlCcchIndInfo->dlCcchMsgLen);
   MAC_FREE_SHRABL_BUF(pst->region, pst->pool, dlCcchIndInfo, sizeof(DlCcchIndInfo));
   return ROK;

}

/*******************************************************************
 *
 * @brief Sends UL CCCH Ind to DU APP
 *
 * @details
 *
 *    Function : macProcUlCcchInd
 *
 *    Functionality:
 *        MAC sends UL CCCH Ind to DU APP
 *
 * @params[in] Post structure pointer
 *            
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t macProcUlCcchInd(uint16_t cellId, uint16_t crnti, uint16_t rrcContSize, uint8_t *rrcContainer)
{
   Pst pst;
   uint8_t ret = ROK;
   UlCcchIndInfo *ulCcchIndInfo = NULLP;

   MAC_ALLOC_SHRABL_BUF(ulCcchIndInfo, sizeof(UlCcchIndInfo));
   if(!ulCcchIndInfo)
   {
      DU_LOG("\nMAC: Memory failed in macProcUlCcchInd");
      return RFAILED;
   }

   ulCcchIndInfo->cellId = cellId;
   ulCcchIndInfo->crnti  = crnti;
   ulCcchIndInfo->ulCcchMsgLen = rrcContSize;
   ulCcchIndInfo->ulCcchMsg = rrcContainer;

   /* Fill Pst */
   FILL_PST_MAC_TO_DUAPP(pst, EVENT_MAC_UL_CCCH_IND);

   if(MacDuAppUlCcchInd(&pst, ulCcchIndInfo) != ROK)
   {
      DU_LOG("\nMAC: Failed to send UL CCCH Ind to DU APP");
      MAC_FREE_SHRABL_BUF(MAC_MEM_REGION, MAC_POOL, ulCcchIndInfo->ulCcchMsg, ulCcchIndInfo->ulCcchMsgLen);
      MAC_FREE_SHRABL_BUF(MAC_MEM_REGION, MAC_POOL, ulCcchIndInfo, sizeof(UlCcchIndInfo));
      ret = RFAILED;
   }
   return ret;
}

/*******************************************************************
 *
 * @brief Processes received short BSR
 *
 * @details
 *
 *    Function : macProcShortBsr
 *
 *    Functionality:
 *        MAC sends Short BSR to SCH
 *
 * @params[in] cell ID
 *             crnti
 *             lcg ID
 *             buffer size
 *
 *
 * ****************************************************************/
uint8_t macProcShortBsr(uint16_t cellId, uint16_t crnti, uint8_t lcgId, uint32_t bufferSize)
{
   Pst                  pst;
   UlBufferStatusRptInd bsrInd;

   memset(&pst, 0, sizeof(Pst));
   memset(&bsrInd, 0, sizeof(UlBufferStatusRptInd));

   bsrInd.cellId                 = cellId;
   bsrInd.crnti                  = crnti;
   bsrInd.bsrType                = SHORT_BSR;
   bsrInd.numLcg                 = 1; /* short bsr reports one lcg at a time */
   bsrInd.dataVolInfo[0].lcgId   = lcgId;
   bsrInd.dataVolInfo[0].dataVol = bufferSize;

   FILL_PST_MAC_TO_SCH(pst, EVENT_SHORT_BSR);
   return(*macSchBsrOpts[pst.selector])(&pst, &bsrInd);
}

/*******************************************************************
 *
 * @brief Builds and send SR UCI Indication to SCH
 *
 * @details
 *
 *    Function : buildAndSendSrInd
 *
 *    Functionality:
 *       Builds and send SR UCI Indication to SCH
 *
 * @params[in] SrUciIndInfo Pointer
 *             crnti value
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t buildAndSendSrInd(UciInd *macUciInd, uint8_t crnti)
{
   uint16_t cellIdx;
   Pst pst;
   SrUciIndInfo   srUciInd;
   memset(&pst, 0, sizeof(Pst));
   memset(&srUciInd, 0, sizeof(SrUciIndInfo));

   GET_CELL_IDX(macUciInd->cellId, cellIdx);
   srUciInd.cellId       = macCb.macCell[cellIdx]->cellId;;
   srUciInd.crnti        = crnti;
   srUciInd.slotInd.sfn  = macUciInd->slotInd.sfn;
   srUciInd.slotInd.slot = macUciInd->slotInd.slot;
   srUciInd.numSrBits++;
   memset(srUciInd.srPayload, 0, MAX_SR_BITS_IN_BYTES);

   /* Fill Pst */
   FILL_PST_MAC_TO_SCH(pst, EVENT_UCI_IND_TO_SCH);

   return(*macSchSrUciIndOpts[pst.selector])(&pst, &srUciInd);
}

/*******************************************************************
 *
 * @brief Processes UCI Indication from PHY
 *
 * @details
 *
 *    Function : fapiMacUciInd
 *
 *    Functionality:
 *       Processes UCI Indication from PHY
 *
 * @params[in] Post Structure Pointer
 *             UCI Indication Pointer
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/
uint8_t FapiMacUciInd(Pst *pst, UciInd *macUciInd)
{
   uint8_t     pduIdx = 0;
   uint8_t     ret = ROK;
   uint16_t    nPdus;
   uint16_t    crnti;

   if(macUciInd)
   {
      nPdus = macUciInd->numUcis;
      while(nPdus)
      {
         switch(macUciInd->pdus[pduIdx].pduType)
         {
            case UCI_IND_PUSCH:
               break;
            case UCI_IND_PUCCH_F0F1:
               if(macUciInd->pdus[pduIdx].uci.uciPucchF0F1.srInfo.srIndPres)
               {
                  DU_LOG("\nMAC : Received SR UCI indication");
		  crnti = macUciInd->pdus[pduIdx].uci.uciPucchF0F1.crnti; 
		  ret = buildAndSendSrInd(macUciInd, crnti);
               }
               break;
            case UCI_IND_PUCCH_F2F3F4:
               break;
            default:
               DU_LOG("\nMAC: Invalid Pdu Type %d at FapiMacUciInd", macUciInd->pdus[pduIdx].pduType);
               ret = RFAILED;
               break;
         }
         pduIdx++;
         nPdus--;
      }
   }
   else
   {
      DU_LOG("\nMAC: Received Uci Ind is NULL at FapiMacUciInd()");
      ret = RFAILED;
   }
   return ret;
}


/**********************************************************************
  End of file
 **********************************************************************/

