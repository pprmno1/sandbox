/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2016 INGENICO S.A.
 28-32 boulevard de Grenelle 75015 Paris, France.
 All rights reserved.
 This source program is the property of the INGENICO Company mentioned above
 and may not be copied in any form or by any means, whether in part or in whole,
 except under license expressly granted by such INGENICO company.
 All copies of this source program, whether in part or in whole, and
 whether modified or not, must display this and all other
 embedded copyright and ownership notices in full.
 ------------------------------------------------------------------------------
 */
#include "acquirer.h"

#include <algorithm>
#include <tpcore/goal_handle.h>
#include <tpcore/telium_manager.h>
#include <types/pos_condition_code.h>
#include <fdms/ui.h>
#include <fdms/sale_workflow.h>
#include <fdms/clear_reversal_workflow.h>
#include <fdms/completion_online_workflow.h>
#include <fdms/completion_offline_workflow.h>
#include <fdms/preauth_workflow.h>
#include <fdms/key_exchange_workflow.h>
#include <fdms/offline_sale_workflow.h>
#include <fdms/receipts.h>
#include <fdms/refund_workflow.h>
#include <fdms/settlement_workflow.h>
#include <fdms/summary_report_workflow.h>
#include <fdms/detail_report_workflow.h>
#include <fdms/tip_adjust_workflow.h>
#include <fdms/transaction_type_name.h>
#include <fdms/void_workflow.h>
#include <fdms/preauth_cancellation_workflow.h>
#include <fdms/instalment_sale_workflow.h>
#include <fdms/host_test_workflow.h>
#include <fdms/reprint_last_settlement_workflow.h>
#include <fdms/reprint_transaction_receipt_workflow.h>
#include <fdms/password.h>
#include <fdms/batch_review_workflow.h>
#include <fdms/batch_total_workflow.h>
#include <fdms/tmk_download_workflow.h>
#include <fdms/quasi_cash_workflow.h>

namespace fdms {
const uint32_t kOffline_Sale_timer_expiry = 30;

Acquirer::Acquirer(ApplicationSettings& app_setting, Batch& batch,
                   HostSwitch& host,
                   emv::EmvConfig& emv_config,
                   contactless::ClessConfig& cless_config,
                   tpcore::PinEntry& pin_entry,
                   Ui& ui,
                   Receipts& receipts,
                   Password& password)
    : app_setting_(app_setting),
      batch_(batch),
      host_(host),
      emv_config_(emv_config),
      cless_config_(cless_config),
      pin_entry_(pin_entry),
      ui_(ui),
      receipts_(receipts),
      password_(password),
      offline_sale_timer_(kOffline_Sale_timer_expiry){
}

bool Acquirer::StartSale() {
  if (!password_.PerformPasswordAccess(TransactionType::SALE))
    return true;

  SaleWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
               pin_entry_,
               ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartQuasiCash() {
  //TODO
  if (!password_.PerformPasswordAccess(TransactionType::QUASI_CASH))
    return true;

  QuasiCashWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
               pin_entry_,
               ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartSale(unsigned int amount_first_digit) {
  SaleWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
               pin_entry_,
               ui_, receipts_).Start(types::PosConditionCode::NORMAL,
                                     amount_first_digit);
  return true;
}

bool Acquirer::StartVoid() {
  if (!password_.PerformPasswordAccess(TransactionType::VOID))
    return true;

  VoidWorkflow(app_setting_, batch_, host_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartVoidPreAuth() {
  if (!password_.PerformPasswordAccess(TransactionType::PREAUTH_CANCELLATION))
    return true;

  PreAuthCancellationWorkflow(app_setting_, batch_, host_, emv_config_,
                              cless_config_,
                              pin_entry_, ui_, receipts_).Start(
      types::PosConditionCode::PREAUTH);
  return true;
}

bool Acquirer::StartPreauth() {
  if (!password_.PerformPasswordAccess(TransactionType::PREAUTH))
    return true;

  PreauthWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                  pin_entry_,
                  ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartCompletionOnline() {
  if (!password_.PerformPasswordAccess(
      TransactionType::PREAUTH_COMPLETION_ONLINE))
    return true;

  CompletionOnlineWorkflow(app_setting_, batch_, host_, emv_config_,
                           cless_config_,
                           ui_,
                           receipts_,
                           pin_entry_).Start(
                                             types::PosConditionCode::PREAUTH);
  return true;
}

bool Acquirer::StartCompletionOffline() {
  if (!password_.PerformPasswordAccess(
      TransactionType::PREAUTH_COMPLETION_OFFLINE))
    return true;

  CompletionOfflineWorkflow(app_setting_, batch_, host_, emv_config_,
                            cless_config_,
                            ui_,
                            receipts_,
                            pin_entry_).Start(
                                              types::PosConditionCode::PREAUTH);
  return true;
}

bool Acquirer::StartMotoPreauth() {
  if (!password_.PerformPasswordAccess(TransactionType::PREAUTH))
    return true;

  PreauthWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                  pin_entry_,
                  ui_, receipts_).Start(
                                        types::PosConditionCode::MOTO_PREAUTH);
  return true;
}

bool Acquirer::StartTipAdjust() {
  if (!password_.PerformPasswordAccess(TransactionType::OFFLINE_SALE))
    return false;

  TipAdjustWorkflow(app_setting_, batch_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartSettlement(bool pos_trigger) {
  if (!pos_trigger) {
    if (!password_.PerformSettlementPasswordAccess())
      return true;
  }

  if (!pos_trigger) {
    SettlementWorkflow(app_setting_, batch_, host_, ui_, receipts_).Start();
  }
  else {
    return SettlementWorkflow(app_setting_, batch_, host_, ui_, receipts_)
        .PosStart();
  }

  offline_sale_timer_ = kOffline_Sale_timer_expiry;
  return true;
}

bool Acquirer::StartRefund() {
  RefundWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                 pin_entry_,
                 ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartMOTOSale() {
  if (!password_.PerformPasswordAccess(TransactionType::SALE))
    return true;

  SaleWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
               pin_entry_,
               ui_, receipts_).Start(types::PosConditionCode::MOTO);
  return true;
}

bool Acquirer::StartInstalmentSale() {

  if (!password_.PerformPasswordAccess(TransactionType::INSTALMENT_SALE))
    return true;

  InstalmentSaleWorkflow(app_setting_, batch_, host_, emv_config_,
                         cless_config_,
                         pin_entry_,
                         ui_,
                         receipts_).Start();

  return true;
}

bool Acquirer::StartKeyExchange() {
  KeyExchangeWorkflow(app_setting_, host_, ui_).Start();
  return true;
}

bool Acquirer::StartTMKDownload() {
  TMKDownloadWorkflow(app_setting_, host_, ui_).Start();
  return true;
}

bool Acquirer::StartClearReversal() {
  if (!password_.PerformManagerPasswordAccess())
    return true;

  ClearReversalWorkFlow(app_setting_, batch_, ui_).Start();
  return true;
}

bool Acquirer::StartHostTest() {
  HostTestWorkfow(app_setting_, host_, ui_).Start();
  return true;
}

bool Acquirer::StartOfflineSale() {

  if(offline_sale_timer_ >= kOffline_Sale_timer_expiry)
  if (!password_.PerformPasswordAccess(TransactionType::OFFLINE_SALE))
    return true;

  OfflineSaleWorkflow(app_setting_, batch_, emv_config_, cless_config_, ui_,
                      receipts_).Start();
  ResetOfflineSaleTimer();
  return true;
}

bool Acquirer::StartClearBatch() {

  unsigned int host_index;

  if (!password_.PerformManagerPasswordAccess())
    return true;

  if (!(ui_.SelectAcquirer(host_index, false)))
    return false;

  if (!ui_.ConfirmDeleteBatch())
    return false;

  if (batch_.ClearBatch(host_index))
    ui_.MessageDeleteBatch(host_index);

  return true;
}

bool Acquirer::StartMOTORefund() {
  RefundWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                 pin_entry_,
                 ui_, receipts_).Start(
                                       types::PosConditionCode::MOTO);
  return true;
}

bool Acquirer::StartReprintLastTransaction(bool pos_trigger) {
  bool ret = ReprintTransactionReceiptWorkflow(app_setting_, batch_, ui_,
                                               receipts_)
      .Start(true);

  return !pos_trigger || ret;
}

bool Acquirer::StartReprintAnyTransaction() {
  ReprintTransactionReceiptWorkflow(app_setting_, batch_, ui_, receipts_)
      .Start(false);
  return true;
}

bool Acquirer::StartReprintLastSettlement() {
  ReprintLastSettlementWorkflow(app_setting_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartPrintSummaryReport() {

  SummaryReportWorkflow(app_setting_, batch_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::PrintDetailReport() {

  DetailReportWorkflow(app_setting_, batch_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartBatchReview() {
  BatchReviewWorkflow(app_setting_, batch_, ui_).Start();
  return true;
}

bool Acquirer::StartBatchTotal() {
  BatchTotalWorkflow(app_setting_, batch_, ui_, receipts_).Start();
  return true;
}

bool Acquirer::StartManagerMenu() {
  if(!password_.PerformManagerPasswordAccess()) {
    SetManagerSelection(false);
    return true;
  }

  #ifdef __TELIUM2__
    tpcore::OpenManagerMenu();
    return true;
  #else
    SetManagerSelection(true);
    return false;
  #endif

  return true;
}

std::unique_ptr<Transaction> Acquirer::StartSale(
    const types::Amount& amount) {
  return stdx::make_unique<Transaction>(
      SaleWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                   pin_entry_,
                   ui_, receipts_).Start(amount));
}

std::unique_ptr<Transaction> Acquirer::StartRefund(
    const types::Amount& amount) {
  return stdx::make_unique<Transaction>(
      RefundWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                     pin_entry_,
                     ui_, receipts_).Start(amount));
}

std::unique_ptr<Transaction> Acquirer::StartPreauth(
    const types::Amount& amount) {
  return stdx::make_unique<Transaction>(
      PreauthWorkflow(app_setting_, batch_, host_, emv_config_, cless_config_,
                      pin_entry_,
                      ui_, receipts_).Start(amount));
}

std::unique_ptr<Transaction> Acquirer::StartVoid(
    const std::string& invoice_num) {
    return stdx::make_unique<Transaction>(
      VoidWorkflow(app_setting_, batch_,
                   host_, ui_, receipts_).Start(invoice_num));
}

bool Acquirer::TODOFunction() {
  ui_.MessageTransactionNotAllowed();
  return true;
}

void Acquirer::IncrementOfflineSaleTimer() {
  if (app_setting_.managed_settings_->terminal_config.TxOfflineSaleEnabled()) {
    if (kOffline_Sale_timer_expiry > offline_sale_timer_)
      offline_sale_timer_++;
  }

}

void Acquirer::ResetOfflineSaleTimer() {
  offline_sale_timer_ = 1;
}

}

