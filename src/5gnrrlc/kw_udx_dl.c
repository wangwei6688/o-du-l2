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

/**********************************************************************
 
    Name:   LTE RLC layer
 
    Type:   C include file
 
    Desc:   Defines required by LTE MAC
 
    File:   kw_udx_dl.c

**********************************************************************/
static const char* RLOG_MODULE_NAME="UDX";
static int RLOG_MODULE_ID=262144;
static int RLOG_FILE_ID=203;

/* header include files (.h) */
#include "common_def.h"
#include "lkw.h"           /* LKW defines */
#include "ckw.h"           /* CKW defines */
#include "kwu.h"           /* KWU defines */
#include "rgu.h"           /* RGU defines */
#include "kw_err.h"        /* Err defines */
#include "kw_env.h"        /* RLC environment options */

#include "kw.h"            /* RLC defines */
#include "kw_udx.h"
#include "kw_dl.h"

/* extern (.x) include files */
#include "lkw.x"           /* LKW */
#include "ckw.x"           /* CKW */
#include "kwu.x"           /* KWU */
#include "rgu.x"           /* RGU */

#include "kw.x"
#include "kw_udx.x"
#include "kw_dl.x"


#define KW_MODULE KW_DBGMASK_UDX
/* local defines */

EXTERN S16 rlcDlmHndlStaRsp ARGS (( RlcCb  *gCb,RlcDlRbCb  *rbCb,
                KwUdxStaPdu *pStaPdu, KwUdxBufLst  *rlsPduLst));




/**
  * @brief
  * UDX APIs
  */

/**
 *
 * @brief 
 *    Handler to bind the DL with UL. 
 *
 * @param[in] pst   Post structure  
 * @param[in] suId  Service user SAP ID 
 * @param[in] spId  Service provider ID
 *
 * @return  S16
 *    -# ROK 
 *    -# RFAILED
 */
#ifdef ANSI
PUBLIC S16 rlcDlUdxBndReq 
(
Pst    *pst,  
SuId   suId, 
SpId   spId 
)
#else
PUBLIC S16 rlcDlUdxBndReq (pst, suId, spId)
Pst    *pst;   
SuId   suId;  
SpId   spId; 
#endif
{
   KwUdxDlSapCb   *udxSap;            /* pointer to session SAP */
   RlcCb           *tRlcCb;

   TRC3(rlcDlUdxBndReq);

#if (ERRCLASS & ERRCLS_INT_PAR)
   if (pst->dstInst >= MAX_RLC_INSTANCES)
   {
      RETVALUE(RFAILED);
   }
#endif
   tRlcCb = RLC_GET_RLCCB(pst->dstInst);

   KWDBGP_BRIEF(tRlcCb, "rlcDlUdxBndReq(spId(%d), suId(%d))\n", 
                spId, suId);

   udxSap = (tRlcCb->u.dlCb->udxDlSap + spId);

   /* Verify CKW SAP State */
   switch(udxSap->state)
   {
      /* SAP is configured but not bound */
      case KW_SAP_CFG:
      case KW_SAP_UBND:
      {
         /* copy bind configuration parameters in SSAP sap */
         udxSap->suId = suId;
         udxSap->pst.dstProcId = pst->srcProcId;
         udxSap->pst.dstEnt = pst->srcEnt;
         udxSap->pst.dstInst = pst->srcInst;

         /* Update the State */
         udxSap->state = KW_SAP_BND;

         RLOG1(L_INFO, "UDX SAP state [%d]", udxSap->state);
         break;
      }
      /* SAP is already bound */
      case KW_SAP_BND:
      {
         /* 
          * Sap is already bound check source, destination Entity and 
          * Proc Id
          */
         if (udxSap->pst.dstProcId != pst->srcProcId 
               || udxSap->pst.dstEnt != pst->srcEnt
               || udxSap->pst.dstInst != pst->srcInst
               || udxSap->suId != suId)
         {
            KW_SEND_SAPID_ALARM(tRlcCb, spId, 
                                LKW_EVENT_UDX_BND_REQ, LCM_CAUSE_INV_PAR_VAL);

            RLOG0(L_ERROR, "UDX SAP already Bound");
            rlcDlUdxBndCfm(&(udxSap->pst), udxSap->suId, CM_BND_NOK);
         }
         break;
      }

     default:
      {
#if (ERRCLASS & ERRCLS_INT_PAR)
         KW_SEND_SAPID_ALARM(tRlcCb,spId, 
                             LKW_EVENT_CKW_BND_REQ, LCM_CAUSE_INV_STATE);
#endif /* ERRCLASS & ERRCLS_INT_PAR */
         RLOG0(L_ERROR, "Invalid UDX SAP State in Bind Req");
         rlcDlUdxBndCfm(&(udxSap->pst), udxSap->suId, CM_BND_NOK);
         break;
      }
   }
   rlcDlUdxBndCfm(&(udxSap->pst), udxSap->suId, CM_BND_OK);
   RETVALUE(ROK);
} 


/**
 * @brief 
 *    Handler for unbinding the DL from UL. 
 *
 *  @param[in] pst     Post structure  
 *  @param[in] spId    Service provider SAP ID 
 *  @param[in] reason  Reason for Unbinding 
 *
 *  @return  S16
 *      -# ROK 
 */
#ifdef ANSI
PUBLIC S16 rlcDlUdxUbndReq
(
Pst      *pst,    
SpId     spId,   
Reason   reason 
)
#else
PUBLIC S16 rlcDlUdxUbndReq(pst, spId, reason)
Pst      *pst;   
SpId     spId;   
Reason   reason; 
#endif
{
   KwUdxDlSapCb   *udxSap; 
   RlcCb           *tRlcCb;

   TRC3(rlcDlUdxUbndReq)

#if (ERRCLASS & ERRCLS_INT_PAR)
   if (pst->dstInst >= MAX_RLC_INSTANCES)
   {
      RETVALUE(RFAILED);
   }
#endif

   tRlcCb = RLC_GET_RLCCB(pst->dstInst);

   RLOG2(L_DEBUG,"Unbind Req for spId[%d], reason[%d]", 
                spId, reason);
   UNUSED(reason);
   /* disable upper sap (CKW) */
   udxSap = (tRlcCb->u.dlCb->udxDlSap + spId);

#if (ERRCLASS & ERRCLS_INT_PAR)
   KW_GET_AND_VALIDATE_UDXSAP(tRlcCb,udxSap, EKW208, "KwUiDlUdxndReq");
#endif /* ERRCLASS & ERRCLS_INT_PAR */
   udxSap->state = KW_SAP_CFG;
   RETVALUE(ROK);
}


/**
 * @brief 
 *    Handler for configuring RLC entities.
 *
 * @details
 *    This function is used by RRC to configure(add/delete/modify)
 *    one or more RLC entities. 
 *        - CKW_CFG_ADD          => rlcCfgAddRb
 *        - CKW_CFG_MODIFY       => rlcCfgReCfgRb
 *        - CKW_CFG_DELETE       => rlcCfgDelRb
 *        - CKW_CFG_REESTABLISH  => rlcCfgReEstRb
 *        - CKW_CFG_DELETE_UE    => rlcCfgDelUe
 *
 * @param[in] pst   -  Post structure  
 * @param[in] spId  -  Serive Provider ID 
 * @param[in] cfg   -  Configuration information for one or more RLC entities. 
 *
 * @return  S16
 *    -# ROK 
 *    -# RFAILED
 */
#ifdef ANSI
PUBLIC S16 rlcDlUdxCfgReq
(
Pst          *pst,
SpId         spId,
RlcCfgInfo   *cfg
)
#else
PUBLIC S16 rlcDlUdxCfgReq(pst, spId, cfg)
Pst          *pst;
SpId         spId;
RlcCfgInfo   *cfg;
#endif
{
   RlcCfgCfmInfo   *cfgCfm; 
   U8              idx;    
   RlcCb            *tRlcCb;
   Pst             *pstUdxCfm;

   TRC3(rlcDlUdxCfgReq)

#if (ERRCLASS & ERRCLS_INT_PAR)
   if (pst->dstInst >= MAX_RLC_INSTANCES)
   {
      RETVALUE(RFAILED);
   }
#endif

   tRlcCb = RLC_GET_RLCCB(pst->dstInst);

   pstUdxCfm = &(tRlcCb->u.dlCb->udxDlSap[spId].pst);
   KWDBGP_BRIEF(tRlcCb,"spId(%d)\n", spId);
   /* Allocate memory and memset to 0 for cfmInfo */
   RLC_ALLOC_SHRABL_BUF_WC(pstUdxCfm->region,
                          pstUdxCfm->pool,
                          cfgCfm,
                          sizeof(RlcCfgCfmInfo));

#if (ERRCLASS & ERRCLS_ADD_RES)
   if (cfgCfm == NULLP)
   {
      RLOG0(L_FATAL,"Memory Allocation Failed.");
      /* kw002.201 Freeing from proper region */
      /* RLC_PST_FREE(pst->region, pst->pool, cfg, sizeof(RlcCfgInfo)); */
      RETVALUE(RFAILED);
   }
#endif /* ERRCLASS & ERRCLS_ADD_RES */

   /* For every entity configuration process by cfgType */
   for (idx = 0; idx < cfg->numEnt; idx++)
   {
      RlcEntCfgCfmInfo   *entCfgCfm;
      RlcEntCfgInfo      *entCfg;

      entCfg  = (RlcEntCfgInfo *)&(cfg->entCfg[idx]);
      entCfgCfm   = (RlcEntCfgCfmInfo *)&(cfgCfm->entCfgCfm[idx]);

      switch (entCfg->cfgType)
      {
         case CKW_CFG_ADD:
            {
               if (entCfg->dir & KW_DIR_DL)
               { 
                  /* Add a new RB entity configuration */
                  if (rlcCfgAddDlRb(tRlcCb,cfg->ueId, cfg->cellId,\
                              entCfg, entCfgCfm) != ROK)
                  {
                     RLOG_ARG1(L_ERROR,DBG_RBID,cfg->entCfg[idx].rbId,"Addition Failed due to[%d]",
                           entCfgCfm->status.reason);
                  }
               }
               break;
            }
         case CKW_CFG_MODIFY:
            {
               if (entCfg->dir & KW_DIR_DL)
               {
                  /* Re-configure the existing RB entity configuration */
                  if (rlcCfgReCfgDlRb(tRlcCb,cfg->ueId, cfg->cellId,\
                           entCfg, entCfgCfm) != ROK)
                  {
                     RLOG_ARG1(L_ERROR,DBG_RBID,cfg->entCfg[idx].rbId,"ReCfg Failed due to[%d]",
                           entCfgCfm->status.reason);
                  }
               }
               break;
            }

         case CKW_CFG_DELETE:
            {
               if (entCfg->dir & KW_DIR_DL)
               {
                  /* Delete the existing RB entity configuration */
                  if (rlcCfgDelDlRb(tRlcCb,cfg->ueId, cfg->cellId,\
                        entCfg, entCfgCfm) != ROK)
                  {
                     RLOG_ARG1(L_ERROR,DBG_RBID,cfg->entCfg[idx].rbId,"Deletion Failed due to[%d]",
                           entCfgCfm->status.reason);
                  } 
               }
               break;
            }

         case CKW_CFG_REESTABLISH:
            {
               if (entCfg->dir & KW_DIR_DL)
               {
                  /*if direction is both then, re-establishment end indication
                   * should be sent only from the UL instance, only if DIR is
                   * DL only then DL instance will send indication.*/
                  Bool sndReEst = TRUE;
                  if (entCfg->dir & KW_DIR_UL)
                  {
                     sndReEst = FALSE;
                  }
                  /* Re-establish the existing RB entity configuration */
                  if (rlcCfgReEstDlRb(tRlcCb,cfg->ueId, cfg->cellId,
                                     sndReEst,entCfg, entCfgCfm) != ROK)
                  {
                     RLOG_ARG1(L_ERROR,DBG_RBID,cfg->entCfg[idx].rbId,"Reest Failed due to[%d]",
                           entCfgCfm->status.reason);
                  }
               }
               break;
            }

         case CKW_CFG_DELETE_UE:
            {
               /* Delete all RB entity configuration under UE */
               if (rlcCfgDelDlUe(tRlcCb,cfg->ueId, cfg->cellId,
                                entCfg, entCfgCfm) != ROK)
               {
                  RLOG_ARG1(L_ERROR,DBG_UEID,cfg->ueId,"deletion Failed due to[%d]",
                           entCfgCfm->status.reason);
               }
               break;
            }
         case CKW_CFG_DELETE_CELL:
            {
               if (rlcCfgDelDlCell(tRlcCb,cfg->cellId,entCfg,entCfgCfm) 
                                                                != ROK )
               {
                  RLOG_ARG1(L_ERROR,DBG_CELLID,cfg->cellId,"deletion Failed due to[%d]",
                           entCfgCfm->status.reason);
               } 
               break;
            }

         default:
            {
               KW_CFG_FILL_CFG_CFM(entCfgCfm, entCfg->rbId, entCfg->rbType,\
                                   CKW_CFG_CFM_NOK, CKW_CFG_REAS_INVALID_CFG);
               RLOG0(L_ERROR, "Invalid CfgType");
            }
      }
   }

   /* Assign number of entity configuraitons and suId */
   cfgCfm->transId = cfg->transId;
   cfgCfm->ueId = cfg->ueId;
   cfgCfm->cellId = cfg->cellId;
   cfgCfm->numEnt = cfg->numEnt;

   /* kw002.201 Freeing from proper region */
   /* RLC_PST_FREE(pst->region, pst->pool, cfg, sizeof(RlcCfgInfo)); */
   /* Send Configuration confirm primitive */
   rlcDlUdxCfgCfm(&(tRlcCb->u.dlCb->udxDlSap[spId].pst),
                 tRlcCb->u.dlCb->udxDlSap[spId].suId, 
                 cfgCfm);

   RETVALUE(ROK);
} 

/**
 *@brief 
 *   This primitive is used by RRC to change the UeId for the existing UE
 *   context.
 *
 * @param pst     -  Pointer to the pst structure
 * @param spId    -  The ID of the service provider SAP in the RLC layer 
 * @param transId -  Transaction ID. This field uniquily identifies
 *                   transaction between RRC and RLC
 * @param ueInfo    -  Old UE Id Info for which the change request has come 
 * @param newUeInfo -  New UE Id Info for existing UE context 
 * 
 * @return 
 *    -# ROK
 *    -# RFAILED
 */
#ifdef ANSI
PUBLIC S16 rlcDlUdxUeIdChgReq
(
Pst         *pst, 
SpId        spId, 
U32         transId, 
CkwUeInfo   *ueInfo,
CkwUeInfo   *newUeInfo
)
#else
PUBLIC S16 rlcDlUdxUeIdChgReq(pst,spId,transId,ueInfo,newUeInfo)
Pst         *pst;
SpId        spId;
U32         transId;
CkwUeInfo   *ueInfo;
CkwUeInfo   *newUeInfo;
#endif
{
   CmStatus       status;
   RlcCb           *tRlcCb;

   TRC3(rlcDlUdxUeIdChgReq)

#if (ERRCLASS & ERRCLS_INT_PAR)
   if (pst->dstInst >= MAX_RLC_INSTANCES)
   {
      RETVALUE(RFAILED);
   }
#endif

   tRlcCb = RLC_GET_RLCCB(pst->dstInst);
#ifndef ALIGN_64BIT
   KWDBGP_BRIEF(tRlcCb, "(spId(%d), transId(%ld))\n", 
                spId, transId);
#else
   KWDBGP_BRIEF(tRlcCb, "(spId(%d), transId(%d))\n", 
                spId, transId);
#endif

   status.reason = CKW_CFG_REAS_NONE;
   status.status = CKW_CFG_CFM_OK;
   
   if (rlcCfgDlUeIdChng(tRlcCb, ueInfo, newUeInfo, &status) != ROK)
   {
      RLOG_ARG1(L_ERROR,DBG_CELLID,newUeInfo->cellId,"Failure due to[%d]",
             status.reason);
   }
   rlcDlUdxUeIdChgCfm(&(tRlcCb->u.dlCb->udxDlSap[spId].pst),
                     tRlcCb->u.dlCb->udxDlSap[spId].suId, 
                     transId, 
                     status);

   RETVALUE(ROK);
} 

/**
* @brief 
*    Request for status PDU from  ULM to DLM.
*
* @param[in]   pst   -  Post Structure
* @param[in]   spId  -  Service Provider Id
* @param[in]   rlcId -  Rlc Information Id
* @param[in]   pStaPdu - Status PDU 
*  
* @return   S16
*    -# ROK
*    -# RFAILED
**/
#ifdef ANSI
PUBLIC S16  rlcDlUdxStaPduReq
(
Pst             *pst,
SpId            spId,
CmLteRlcId      *rlcId,
KwUdxDlStaPdu   *pStaPdu
)
#else
PUBLIC S16  rlcDlUdxStaPduReq(pst, spId, rlcId, pStaPdu)
Pst             *pst;
SpId            spId;
CmLteRlcId      *rlcId;
KwUdxDlStaPdu   *pStaPdu;
#endif
{
   RlcDlRbCb   *rbCb;
   RlcCb       *tRlcCb;

   tRlcCb =  RLC_GET_RLCCB (pst->dstInst);

   kwDbmFetchDlRbCbByRbId(tRlcCb, rlcId, &rbCb); /* Fetch DBM RbCb */
   if (!rbCb)
   {
      RLOG_ARG2(L_ERROR, DBG_UEID,rlcId->ueId, "CellId [%u]:RbId[%d] not found",
            rlcId->cellId,rlcId->rbId);
      RLC_FREE_SHRABL_BUF(pst->region, 
			 pst->pool, 
			 pStaPdu, 
			 sizeof(KwUdxDlStaPdu));
      RETVALUE(RFAILED);
   }

   AMDL.cntrlBo = pStaPdu->controlBo;
   /* If there already exists a STAUS PDU, free it and take the new one
      into account */
   if(AMDL.pStaPdu)
   {
      RLC_FREE_SHRABL_BUF(pst->region, 
			 pst->pool, 
			 AMDL.pStaPdu, 
			 sizeof(KwUdxDlStaPdu));
   }
   
   AMDL.pStaPdu = pStaPdu;
   kwAmmSendDStaRsp(tRlcCb, rbCb, &AMDL);             

   RETVALUE (ROK);
}

/**
* @brief 
*    It handles the status update recieved from ULM.
*
* @param[in]   pst   -  Post Structure
* @param[in]   spId  -  Service Provider Id
* @param[in]   rlcId -  Rlc Information Id
* @param[in]   pStaPdu - Status PDU 
*  
* @return   S16
*    -# ROK
*    -# RFAILED
**/
#ifdef ANSI
PUBLIC S16  rlcDlUdxStaUpdReq
(
Pst*          pst,
SpId          spId,
CmLteRlcId    *rlcId,
KwUdxStaPdu   *pStaPdu
)
#else
PUBLIC S16 rlcDlUdxStaUpdReq(pst, spId, rlcId,pStaPdu)
Pst*          pst;
SpId          spId;
CmLteRlcId    *rlcId;
KwUdxStaPdu   *pStaPdu;
#endif
{
   RlcCb          *tRlcCb;
   RlcDlRbCb      *rbCb;

   tRlcCb = RLC_GET_RLCCB(pst->dstInst);

   kwDbmFetchDlRbCbByRbId(tRlcCb, rlcId, &rbCb);
   if (!rbCb)
   {
      RLOG_ARG2(L_ERROR, DBG_UEID,rlcId->ueId, "CellId [%u]:RbId[%d] not found",
            rlcId->cellId,rlcId->rbId);
      RETVALUE(RFAILED);
   }

   kwAmmDlHndlStatusPdu(tRlcCb, rbCb, pStaPdu);

   RLC_FREE_SHRABL_BUF(pst->region,
		      pst->pool, 
		      pStaPdu, 
		      sizeof(KwUdxStaPdu));

   RETVALUE(ROK);
}

#ifdef LTE_L2_MEAS
/**
*/
#ifdef ANSI
PUBLIC S16 rlcDlUdxL2MeasReq 
(
Pst            *pst, 
KwL2MeasReqEvt *measReqEvt 
)
#else
PUBLIC S16 rlcDlUdxL2MeasReq (pst, measReqEvt)
Pst            *pst; 
KwL2MeasReqEvt *measReqEvt;
#endif
{
   U32            cntr;
   U8             measType;
   VOLATILE U32     startTime = 0;
   RlcCb     *tRlcCb;

   TRC3(rlcDlUdxL2MeasReq);

   /*starting Task*/
   SStartTask(&startTime, PID_RLC_MEAS_START);

   tRlcCb =  RLC_GET_RLCCB(pst->dstInst);

   /* Initialize measCfmEvt */

  /* validate the received measReqEvt */
 /*LTE_L2_MEAS_PHASE2*/

   measType = measReqEvt->measReq.measType;

   if(measType & LKW_L2MEAS_DL_IP) 
   {
      /* if measurement is for DL IP enable for all QCI */
      for(cntr = 0; cntr < LKW_MAX_QCI; cntr++)
      {
         tRlcCb->u.dlCb->kwL2Cb.measOn[cntr] |= LKW_L2MEAS_DL_IP;
      }
   }
   else
   {
      /* for nonIpThroughput meas, enable only for the sent QCIs */
      U32 i;
      for(i = 0; i < LKW_MAX_QCI; i++)
      {
         tRlcCb->u.dlCb->kwL2Cb.measOn[i] |= measType;
      }
   }

   /* We need to copy the transId for sending back confirms later */
   for(cntr = 0; cntr < LKW_MAX_L2MEAS; cntr++)
   {
      KwL2MeasEvtCb* measEvtCb = &(tRlcCb->u.dlCb->kwL2Cb.kwL2EvtCb[cntr]);
      if(measEvtCb->measCb.measType & measType)
      {
         measEvtCb->transId= measReqEvt->transId;
      }
   }

   /*stopping Task*/
   SStopTask(startTime, PID_RLC_MEAS_START);
   RETVALUE(ROK);
} /* rlcDlUdxMeasReq */

/**
@brief 
This function processes L2 Measurement stop request received from the layer manager.
After receving this request, RLC stops L2 Measurement
 *  @param[in] pst      post structure
 *  @param[in] measType meas Type 
 *  @return S16
 *      -# Success : ROK
 *      -# Failure : RFAILED
*/

#ifdef ANSI
PUBLIC S16 rlcDlUdxL2MeasStopReq
(
Pst            *pst,
U8             measType
)
#else
PUBLIC S16 rlcDlUdxL2MeasStopReq (pst, measType)
Pst            *pst;
U8             measType;
#endif
{
  /* S16 ret = ROK;*/
   KwL2MeasEvtCb *measEvtCb = NULLP;
   U16            cntr;
   U8             status = ROK;
/*   KwL2MeasCfmEvt          measCfmEvt;  */
   VOLATILE U32     startTime = 0;
   RlcCb     *tRlcCb=NULLP;
   TRC3(rlcDlUdxMeasStopReq);
   
   /*starting Task*/
   SStartTask(&startTime, PID_RLC_MEAS_STOP);

   tRlcCb =  RLC_GET_RLCCB(pst->dstInst);
/*   cmMemset((U8*)&measCfmEvt, 0, sizeof(KwL2MeasCfmEvt)); */
   /* reset the counters for the measurement type passed */
   for(cntr = 0; cntr < LKW_MAX_L2MEAS; cntr++)
   {
      measEvtCb = &(tRlcCb->u.dlCb->kwL2Cb.kwL2EvtCb[cntr]);
      if(measEvtCb->measCb.measType & measType)
      {
         kwUtlResetDlL2MeasInKwRb(tRlcCb, &measEvtCb->measCb, measType);

      }
   }

   /* switch off the measurements for the type passed */
   for(cntr = 0; cntr < LKW_MAX_QCI; cntr++)
   {
      tRlcCb->u.dlCb->kwL2Cb.measOn[cntr] &= ~measType;
   }
   
   status = LCM_PRIM_OK;
   /* Stop confirm is removed as UL thread is already sending it */
   
   /*stopping Task*/
   SStopTask(startTime, PID_RLC_MEAS_STOP);

   RETVALUE(ROK);
}
/**
@brief 
This function processes L2 Measurement Send request received from the layer manager.
After receving this request, RLC sends L2 Measurement
 *  @param[in] pst      post structure
 *  @param[in] measType meas Type 
 *  @return S16
 *      -# Success : ROK
 *      -# Failure : RFAILED
*/

#ifdef ANSI
PUBLIC S16 rlcDlUdxL2MeasSendReq
(
Pst            *pst,
U8             measType
)
#else
PUBLIC S16 rlcDlUdxL2MeasSendReq (pst, measType)
Pst            *pst;
U8             measType;
#endif
{
   KwL2MeasEvtCb *measEvtCb;
   U16            cntr;
   
   VOLATILE U32     startTime = 0;
   RlcCb     *tRlcCb;
   TRC3(rlcDlUdxMeasSendReq);

   tRlcCb =  RLC_GET_RLCCB(pst->dstInst);
   for(cntr = 0; cntr < LKW_MAX_L2MEAS; cntr++)
   {
      measEvtCb = &(tRlcCb->u.dlCb->kwL2Cb.kwL2EvtCb[cntr]);
      if(measEvtCb->measCb.measType & measType)
      {
         /*starting Task*/
         SStartTask(&startTime, PID_RLC_MEAS_REPORT);

         kwUtlSndDlL2MeasCfm(tRlcCb, measEvtCb);

         /*stopping Task*/
         SStopTask(startTime, PID_RLC_MEAS_REPORT);
      }
   }

   RETVALUE(ROK);
}
#endif /* LTE_L2_MEAS */

#ifdef __cplusplus
}
#endif /* __cplusplus */


/**********************************************************************
         End of file
**********************************************************************/
