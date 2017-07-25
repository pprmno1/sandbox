/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2015 INGENICO S.A.
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
#include <fdms/host_switch.h>
#include <diners/diners_host.h>
#include <functional>
#include <utils/get_default.h>
#include <amex/amex_host.h>
#include "app_counter.h"

namespace fdms {

namespace {

HostSwitch::Status ConvertStatus(Host::Status status) {
  static std::map<Host::Status, HostSwitch::Status> map =
      {
          { Host::Status::COMPLETED, HostSwitch::Status::COMPLETED },
          { Host::Status::TRANSIENT_FAILURE, HostSwitch::Status::TRANSIENT_FAILURE },
          { Host::Status::PERM_FAILURE, HostSwitch::Status::PERM_FAILURE },
      };
  return utils::GetDefault(map, status, HostSwitch::Status::PERM_FAILURE);
}

HostSwitch::Status ConvertAmexStatus(amex::AmexHost::Status status) {
  static std::map<amex::AmexHost::Status, HostSwitch::Status> map =
      {
          { amex::AmexHost::Status::COMPLETED, HostSwitch::Status::COMPLETED },
          { amex::AmexHost::Status::TRANSIENT_FAILURE, HostSwitch::Status::TRANSIENT_FAILURE },
          { amex::AmexHost::Status::PERM_FAILURE, HostSwitch::Status::PERM_FAILURE },
      };
  return utils::GetDefault(map, status, HostSwitch::Status::PERM_FAILURE);
}

amex::AmexTransactionType ConvertTxToAmexTxType(TransactionType tx_type) {
  amex::AmexTransactionType amex_tx_type = amex::AmexTransactionType::SALE;
  switch (tx_type) {
    case TransactionType::REFUND:
      amex_tx_type = amex::AmexTransactionType::REFUND;
      break;

    case TransactionType::PREAUTH:
      amex_tx_type = amex::AmexTransactionType::PREAUTH;
      break;

    case TransactionType::PREAUTH_COMPLETION_ONLINE:
      amex_tx_type = amex::AmexTransactionType::PREAUTH_COMPLETION_ONLINE;
      break;

    case TransactionType::OFFLINE_SALE:
      amex_tx_type = amex::AmexTransactionType::OFFLINE_SALE;
      break;

    case TransactionType::SALE:
      default:     // TODO: other types
      amex_tx_type = amex::AmexTransactionType::SALE;
      break;
  }

  return amex_tx_type;
}

amex::AmexTransaction BuildAmexTransactionFromTransaction(const Transaction tx) {
  amex::AmexTransaction amex_tx;

  amex_tx.pan = tx.pan;
  amex_tx.processing_code = tx.processing_code;
  amex_tx.amount = tx.amount;

  amex_tx.stan = tx.stan;
  amex_tx.expiration_date = tx.expiration_date;
  amex_tx.pos_entry_mode = tx.pos_entry_mode;
  amex_tx.nii = tx.nii;
  amex_tx.pos_condition_code = tx.pos_condition_code;

  if (tx.track2_equivalent_data) {
    amex_tx.track2 = tx.track2_equivalent_data->GetData();
  }

  amex_tx.tid = tx.tid;
  amex_tx.mid = tx.mid;

  if (!tx.track1_data.empty()) {
    amex_tx.track1_data = tx.track1_data;
  }

  if (!tx.pin_data.empty()) {
    amex_tx.pin_block = tx.pin_data;
  }

  amex_tx.additional_amount = tx.secondary_amount;
  amex_tx.icc_data = tx.icc_data;
  amex_tx.transaction_type = ConvertTxToAmexTxType(tx.transaction_type);
  amex_tx.tpdu = tx.tpdu;

  amex_tx.tx_datetime = tx.tx_datetime;
  amex_tx.rrn = tx.rrn;
  amex_tx.auth_id_response = tx.auth_id_response;
  amex_tx.response_code = tx.response_code;
  amex_tx.issuer_emv_response = tx.issuer_emv_response;

  amex_tx.invoice_number = tx.invoice_num;
  amex_tx.amex_4dbc = tx.cvv;
  amex_tx.preauth_amount = tx.preauth_amount;

  return amex_tx;
}

amex::AmexSettlementData BuildAmexSettlementData(const SettlementData & settle_data){
  amex::AmexSettlementData amex_settle;
  amex_settle.tx_datetime = settle_data.tx_datetime;
  amex_settle.batch_number = settle_data.batch_number;
  amex_settle.nii = settle_data.nii;
  amex_settle.tpdu = settle_data.tpdu;
  amex_settle.tid = settle_data.tid;
  amex_settle.mid = settle_data.mid;
  amex_settle.rrn = settle_data.rrn;
  amex_settle.stan = settle_data.stan;

  amex_settle.batch_summary.credit_total.count = settle_data.batch_summary.refunds_total.count;
  amex_settle.batch_summary.credit_total.total = settle_data.batch_summary.refunds_total.total;
  amex_settle.batch_summary.debit_total.count = settle_data.batch_summary.sales_total.count;
  amex_settle.batch_summary.debit_total.total = settle_data.batch_summary.sales_total.total;
  amex_settle.batch_summary.currency = settle_data.batch_summary.currency;

  amex_settle.invoice_num = GetNextInvoiceNo();  //TODO: check / clarify amex requirement

  return amex_settle;
}

void FillSettlementDataWithAmexSettlementData(SettlementData & settle_data, const amex::AmexSettlementData amex_settle_data) {
  settle_data.processing_code = amex_settle_data.processing_code;
  settle_data.tx_datetime = amex_settle_data.tx_datetime;
  settle_data.rrn = amex_settle_data.rrn;
  settle_data.response_code = amex_settle_data.response_code;
  settle_data.stan = amex_settle_data.stan;
  settle_data.nii = amex_settle_data.nii;
  settle_data.tid = amex_settle_data.tid;
}

void FillTransactionWithAmexTransactionData(Transaction& tx, const amex::AmexTransaction amex_tx) {

  tx.processing_code = amex_tx.processing_code;
  tx.tx_datetime = amex_tx.tx_datetime;  // TODO: check that. Shall we have a host datetime somewhere?
  tx.rrn = amex_tx.rrn;
  tx.auth_id_response = amex_tx.auth_id_response;
  tx.response_code = amex_tx.response_code;
  tx.issuer_emv_response = amex_tx.issuer_emv_response;
}

HostSwitch::Status ConvertDinersStatus(diners::DinersHost::Status status) {
  static std::map<diners::DinersHost::Status, HostSwitch::Status> map =
      {
          { diners::DinersHost::Status::COMPLETED, HostSwitch::Status::COMPLETED },
          { diners::DinersHost::Status::TRANSIENT_FAILURE, HostSwitch::Status::TRANSIENT_FAILURE },
          { diners::DinersHost::Status::PERM_FAILURE, HostSwitch::Status::PERM_FAILURE },
      };
  return utils::GetDefault(map, status, HostSwitch::Status::PERM_FAILURE);
}

diners::DinersTransactionType ConvertTxToDinersTxType(TransactionType tx_type) {
  diners::DinersTransactionType diners_tx_type = diners::DinersTransactionType::SALE;
  switch (tx_type) {
    case TransactionType::REFUND:
      diners_tx_type = diners::DinersTransactionType::REFUND;
      break;

    case TransactionType::PREAUTH:
      diners_tx_type = diners::DinersTransactionType::PREAUTH;
      break;

    case TransactionType::AUTHORIZATION:
      diners_tx_type = diners::DinersTransactionType::AUTHORIZATION;
      break;

    case TransactionType::OFFLINE_SALE:
      diners_tx_type = diners::DinersTransactionType::OFFLINE_SALE;
      break;

    case TransactionType::PREAUTH_COMPLETION_OFFLINE:
      diners_tx_type = diners::DinersTransactionType::PREAUTH_COMPLETION_OFFLINE;
      break;

    default:     // TODO: other types
      diners_tx_type = diners::DinersTransactionType::SALE;
      break;
  }

  return diners_tx_type;
}

diners::DinersTransactionStatus ConvertTxToDinersTxStatus(TransactionStatus tx_status) {
  diners::DinersTransactionStatus diners_tx_status = diners::DinersTransactionStatus::IN_PROGRESS;
  switch (tx_status) {
    case TransactionStatus::IN_PROGRESS:
      diners_tx_status = diners::DinersTransactionStatus::IN_PROGRESS;
      break;

    case TransactionStatus::APPROVED:
      diners_tx_status = diners::DinersTransactionStatus::APPROVED;
      break;

    case TransactionStatus::DECLINED:
      diners_tx_status = diners::DinersTransactionStatus::DECLINED;
      break;

    case TransactionStatus::NOT_ALLOWED:
      diners_tx_status = diners::DinersTransactionStatus::NOT_ALLOWED;
      break;

    case TransactionStatus::TO_ADVISE:
      diners_tx_status = diners::DinersTransactionStatus::TO_ADVISE;
      break;

    case TransactionStatus::CANCELLED:
    default:     // TODO: other transaction status
      diners_tx_status = diners::DinersTransactionStatus::DECLINED;
      break;
  }

  return diners_tx_status;
}

diners::DinersInProgressStatus ConvertTxToDinersInProgressStatus(InProgressStatus in_progress_status){
  diners::DinersInProgressStatus diners_in_status;
  switch(in_progress_status){
  case InProgressStatus::IN_PROGRESS_NONE:
    diners_in_status = diners::DinersInProgressStatus::IN_PROGRESS_NONE;
    break;

  case InProgressStatus::IN_PROGRESS_VOID:
    diners_in_status = diners::DinersInProgressStatus::IN_PROGRESS_VOID;
    break;

  case InProgressStatus::IN_PROGRESS_PREAUTH_COMPLETION:
    diners_in_status = diners::DinersInProgressStatus::IN_PROGRESS_SALE_COMPLETION;
    break;

  default:     // TODO: other status
    diners_in_status = diners::DinersInProgressStatus::IN_PROGRESS_NONE;
    break;
  }

  return diners_in_status;
}

diners::DinersTransaction BuildDinersTransactionFromTransaction(const Transaction tx) {
	diners::DinersTransaction diners_tx;

    diners_tx.pan = tx.pan;
    diners_tx.processing_code = tx.processing_code;
    diners_tx.amount = tx.amount;

    diners_tx.stan = tx.stan;
    diners_tx.expiration_date = tx.expiration_date;
    diners_tx.pos_entry_mode = tx.pos_entry_mode;
    diners_tx.nii = tx.nii;
    diners_tx.pos_condition_code = tx.pos_condition_code;

    if (tx.track2_equivalent_data) {
    	diners_tx.track2 = tx.track2_equivalent_data->GetData();
    }

    if(tx.aid){
    	diners_tx.aid = tx.aid;
    }

    diners_tx.tid = tx.tid;
    diners_tx.mid = tx.mid;
    diners_tx.cvv = tx.cvv;

    if (!tx.track1_data.empty()) {
    	diners_tx.track1_data = tx.track1_data;
    }

    if (!tx.pin_data.empty()) {
    	diners_tx.pin_block = tx.pin_data;
    }

    diners_tx.additional_amount = tx.secondary_amount;
    diners_tx.original_additional_amount = tx.original_secondary_amount;
    diners_tx.icc_data = tx.icc_data;
    diners_tx.transaction_type = ConvertTxToDinersTxType(tx.transaction_type);
    diners_tx.in_progress_status = ConvertTxToDinersInProgressStatus(tx.in_progress_status);
    diners_tx.transaction_status = ConvertTxToDinersTxStatus(tx.transaction_status);
    diners_tx.previous_transaction_status = ConvertTxToDinersTxStatus(tx.previous_transaction_status);
    diners_tx.tpdu = tx.tpdu;

    diners_tx.tx_datetime = tx.tx_datetime;
    diners_tx.rrn = tx.rrn;
    diners_tx.auth_id_response = tx.auth_id_response;
    diners_tx.response_code = tx.response_code;
    diners_tx.issuer_emv_response = tx.issuer_emv_response;

    diners_tx.invoice_number = tx.invoice_num;
    diners_tx.batch_number = tx.batch_num;
    diners_tx.preauth_amount = tx.preauth_amount;
    diners_tx.cardholder_name = tx.card_holder_name;
    if(tx.pan_sequence_number){
    	diners_tx.pan_sequence_number = tx.pan_sequence_number;
    }

    diners_tx.is_preauth_completed = tx.is_preauth_completed;
    diners_tx.is_adjusted = tx.is_adjusted;

    return diners_tx;
}

diners::DinersSettlementData BuildDinersSettlementData(const SettlementData & settle_data){
	diners::DinersSettlementData diners_settle;
    diners_settle.tx_datetime = settle_data.tx_datetime; //To check
    diners_settle.batch_number = settle_data.batch_number;
    diners_settle.nii = settle_data.nii;
    diners_settle.tpdu = settle_data.tpdu;
    diners_settle.tid = settle_data.tid;
    diners_settle.mid = settle_data.mid;
    diners_settle.rrn = settle_data.rrn;
    diners_settle.stan = settle_data.stan;

    diners_settle.batch_summary.refunds_total.count = settle_data.batch_summary.refunds_total.count;
    diners_settle.batch_summary.refunds_total.total = settle_data.batch_summary.refunds_total.total;
    diners_settle.batch_summary.sales_total.count = settle_data.batch_summary.sales_total.count;
    diners_settle.batch_summary.sales_total.total = settle_data.batch_summary.sales_total.total;
    diners_settle.batch_summary.currency = settle_data.batch_summary.currency;

    diners_settle.invoice_num = GetNextInvoiceNo();  //TODO: check
    return diners_settle;
}

void FillSettlementDataWithDinersSettlementData(SettlementData & settle_data, const diners::DinersSettlementData diners_settle_data) {
	settle_data.processing_code = diners_settle_data.processing_code;
    settle_data.tx_datetime = diners_settle_data.tx_datetime;
    settle_data.rrn = diners_settle_data.rrn;
    settle_data.response_code = diners_settle_data.response_code;
    settle_data.stan = diners_settle_data.stan;
    settle_data.nii = diners_settle_data.nii;
    settle_data.tid = diners_settle_data.tid;
}

void FillTransactionWithDinersTransactionData(Transaction& tx, const diners::DinersTransaction diners_tx) {
	tx.processing_code = diners_tx.processing_code;
    tx.tx_datetime = diners_tx.tx_datetime;  // TODO: check that. Shall we have a host datetime somewhere?
    tx.rrn = diners_tx.rrn;
    tx.auth_id_response = diners_tx.auth_id_response;
    tx.response_code = diners_tx.response_code;
    tx.issuer_emv_response = diners_tx.issuer_emv_response;
}

}

amex::AmexHost& HostSwitch::GetAmexHost() {
  if (!host_amex_) {
    host_amex_ = stdx::make_unique<amex::AmexHost>(
        app_settings_.managed_settings_->amex_config.Origin(),
        app_settings_.managed_settings_->amex_config.CountryCode(),
        app_settings_.managed_settings_->amex_config.Region(),
        app_settings_.managed_settings_->amex_config.RoutingIndicator());
  }

  return *host_amex_;
}

diners::DinersHost& HostSwitch::GetDinersHost() {
  if (!host_diners_) {
	  host_diners_ = stdx::make_unique<diners::DinersHost>();
  }

  return *host_diners_;
}

template<typename T1, typename T2, typename T3>
HostSwitch::Status HostSwitch::PerformActionWithTx(T1 f1, T2 f2, T3 f3, Transaction& tx) {
  using namespace std::placeholders;

  if (!current_host_protocol_) {
    return HostSwitch::Status::PERM_FAILURE;
  }

  if (*current_host_protocol_ == HostProtocol::FDMS_BASE24) {
    auto f = std::bind < Host::Status > (f1, host_fdms_, _1);
    Host::Status status = f(tx);
    return ConvertStatus(status);

  }

  else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT) {
    auto f = std::bind < amex::AmexHost::Status > (f2, GetAmexHost(), _1);
    amex::AmexTransaction amex_tx = BuildAmexTransactionFromTransaction(tx);
    amex::AmexHost::Status status = f(amex_tx);
    FillTransactionWithAmexTransactionData(tx, amex_tx);
    return ConvertAmexStatus(status);

  }

  else if(*current_host_protocol_ == HostProtocol::DINERS_DIRECT){
	auto f = std::bind < diners::DinersHost::Status > (f3, GetDinersHost(), _1);
	diners::DinersTransaction diners_tx = BuildDinersTransactionFromTransaction(tx);
	diners::DinersHost::Status status = f(diners_tx);
	FillTransactionWithDinersTransactionData(tx, diners_tx);
	return ConvertDinersStatus(status);
  }

  current_host_protocol_ = stdx::nullopt;
  return HostSwitch::Status::PERM_FAILURE;
}

bool HostSwitch::PreConnect(unsigned int host_index) {
	stdx::optional<HostDefinition> host_config = app_settings_.managed_settings_->GetHostDefinition(host_index);
    if (!host_config)
    	return false;

    bool ret = false;
    current_host_protocol_ = host_config->host_protocol;
    if (*current_host_protocol_ == HostProtocol::FDMS_BASE24)
    	ret = host_fdms_.PreConnect(host_config->comms_host_name);
    else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT)
    	ret = GetAmexHost().PreConnect(host_config->comms_host_name);
    else if (*current_host_protocol_ == HostProtocol::DINERS_DIRECT)
    	ret = GetDinersHost().PreConnect(host_config->comms_host_name);

    if (!ret)
    	current_host_protocol_ = stdx::nullopt;
    return ret;
}

HostSwitch::Status HostSwitch::AuthorizeSale(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizeSale,
                               &amex::AmexHost::AuthorizeSale,
                               &diners::DinersHost::AuthorizeSale,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizeSaleWithDccEnquiry(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizeSaleWithDccEnquiry,
                               &amex::AmexHost::AuthorizeSale,
							   &diners::DinersHost::AuthorizeSale,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizeSaleWithDccAllowed(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizeSaleWithDccAllowed,
                               &amex::AmexHost::AuthorizeSale,
							   &diners::DinersHost::AuthorizeSale,
                               tx);
}

HostSwitch::Status HostSwitch::PerformTcUpload(Transaction& tx) {
    return PerformActionWithTx(&Host::PerformTcUpload,
                               &amex::AmexHost::PerformTcUpload,
							   &diners::DinersHost::PerformTcUpload,
                               tx);
}

HostSwitch::Status HostSwitch::PerformVoid(Transaction& tx) {
    return PerformActionWithTx(&Host::PerformVoid,
                               &amex::AmexHost::PerformVoid,
							   &diners::DinersHost::PerformVoid,
                               tx);
}

HostSwitch::Status HostSwitch::PerformAmexBatchUpload(std::vector<Transaction>& transaction_list){
	HostSwitch::Status output = HostSwitch::Status::PERM_FAILURE;
    std::vector<amex::AmexTransaction> amex_tx_list;
    std::transform(transaction_list.begin(), transaction_list.end(), std::back_inserter(amex_tx_list), BuildAmexTransactionFromTransaction);

    amex::AmexHost::Status status;
    for (auto tx : amex_tx_list){
    	status = GetAmexHost().PerformBatchUpload(tx, GetNextStanNo());
        if (status != amex::AmexHost::Status::COMPLETED)
        	break;
    }

    output = ConvertAmexStatus(status);
    return output;
}

HostSwitch::Status HostSwitch::PerformBatchUpload(std::vector<Transaction>& transaction_list) {
	HostSwitch::Status output = HostSwitch::Status::PERM_FAILURE;
	if (!current_host_protocol_)
		return output;

	if (*current_host_protocol_ == HostProtocol::FDMS_BASE24) {
		Host::Status status = host_fdms_.PerformBatchUpload(transaction_list);
        output = ConvertStatus(status);
	}
	else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT) {
		output = PerformAmexBatchUpload(transaction_list);
	}
	else if (*current_host_protocol_ == HostProtocol::DINERS_DIRECT){
		output = PerformDinersBatchUpload(transaction_list);
    }

	return output;
}

HostSwitch::Status HostSwitch::PerformDinersBatchUpload(std::vector<Transaction>& transaction_list){
    std::vector<diners::DinersTransaction> diners_tx_list;
    std::transform(transaction_list.begin(), transaction_list.end(), std::back_inserter(diners_tx_list), BuildDinersTransactionFromTransaction);

    diners::DinersHost::Status status;
    for (auto diners_tx : diners_tx_list){
    	status = GetDinersHost().PerformBatchUpload(diners_tx, GetNextStanNo());
        if (status != diners::DinersHost::Status::COMPLETED)
        	break;
    }

    return ConvertDinersStatus(status);
}

HostSwitch::Status HostSwitch::SendReversal(Transaction& tx) {
    return PerformActionWithTx(&Host::SendReversal,
                               &amex::AmexHost::SendReversal,
							   &diners::DinersHost::SendReversal,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizeRefund(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizeRefund,
                               &amex::AmexHost::AuthorizeRefund,
							   &diners::DinersHost::AuthorizeRefund,
                               tx);
}

HostSwitch::Status HostSwitch::PerformOfflineSale(Transaction& tx) {
    return PerformActionWithTx(&Host::PerformOfflineSale,
                               &amex::AmexHost::SendOfflineSale,
							   &diners::DinersHost::SendOfflineSale,
                               tx);
}

HostSwitch::Status HostSwitch::PerformOfflineWithDccEnquiry(Transaction& tx) {
    return PerformActionWithTx(&Host::PerformOfflineWithDccEnquiry,
                               &amex::AmexHost::SendOfflineSale,
							   &diners::DinersHost::SendOfflineSale,
                               tx);
}

HostSwitch::Status HostSwitch::PerformOfflineWithDccAllowed(Transaction& tx) {
    return PerformActionWithTx(&Host::PerformOfflineWithDccAllowed,
                               &amex::AmexHost::SendOfflineSale,
							   &diners::DinersHost::SendOfflineSale,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizePreAuth(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizePreAuth,
                               &amex::AmexHost::AuthorizePreAuth,
							   &diners::DinersHost::AuthorizePreAuth,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizePreAuthWithDccEnquiry(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizePreAuthWithDccEnquiry,
                               &amex::AmexHost::AuthorizePreAuth,
							   &diners::DinersHost::AuthorizePreAuth,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizePreAuthWithDccAllowed(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizePreAuthWithDccAllowed,
                               &amex::AmexHost::AuthorizePreAuth,
							   &diners::DinersHost::AuthorizePreAuth,
                               tx);
}

HostSwitch::Status HostSwitch::PerformTipAdjust(Transaction& tx) {
    return PerformActionWithTx(&Host::SendTipAdjust,
                               &amex::AmexHost::SendTipAdjust,
							   &diners::DinersHost::SendTipAdjust,
                               tx);
}

HostSwitch::Status HostSwitch::PerformFdmsTMKDownload(TMKDownloadMessage& tmk_download_msg) {
	if (!current_host_protocol_ || *current_host_protocol_ != HostProtocol::FDMS_BASE24) {
		return HostSwitch::Status::PERM_FAILURE;
    }

    Host::Status status = host_fdms_.PerformTMKDownload(tmk_download_msg);
    return ConvertStatus(status);
}

HostSwitch::Status HostSwitch::AuthorizePreAuthCompletion(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizePreAuthCompletion,
                               &amex::AmexHost::AuthorizePreAuthCompletion,
							   &diners::DinersHost::PerformSaleCompletion,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizePreAuthCompletionWithDccEnquiry(Transaction& tx) {
    return PerformActionWithTx(&Host::AuthorizePreAuthCompletionWithDccEnquiry,
                               &amex::AmexHost::AuthorizePreAuthCompletion,
							   &diners::DinersHost::PerformSaleCompletion,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizePreAuthCompletionWithDccAllowed(Transaction& tx) {
	return PerformActionWithTx(&Host::AuthorizePreAuthCompletionWithDccAllowed,
                               &amex::AmexHost::AuthorizePreAuthCompletion,
							   &diners::DinersHost::PerformSaleCompletion,
                               tx);
}

HostSwitch::Status HostSwitch::AuthorizeQuasiCash(Transaction& tx) {
	return PerformActionWithTx(&Host::AuthorizeQuasiCash,
                               &amex::AmexHost::AuthorizeSale,
							   &diners::DinersHost::AuthorizeSale,
                               tx);
}

HostSwitch::Status HostSwitch::PerformFdmsPreAuthCancellation(Transaction& tx) {
	if (!current_host_protocol_ || *current_host_protocol_ != HostProtocol::FDMS_BASE24) {
		return HostSwitch::Status::PERM_FAILURE;
    }

    Host::Status status = host_fdms_.PerformPreAuthCancellation(tx);
    return ConvertStatus(status);
}

HostSwitch::Status HostSwitch::AuthorizeFdmsInstalmentSale(Transaction& tx) {
	if (!current_host_protocol_ || *current_host_protocol_ != HostProtocol::FDMS_BASE24) {
		return HostSwitch::Status::PERM_FAILURE;
    }

    Host::Status status = host_fdms_.AuthorizeInstalmentSale(tx);
    return ConvertStatus(status);
}

HostSwitch::Status HostSwitch::PerformFdmsKeyExchange(KeyExchange& key_exchange) {
	if (!current_host_protocol_ || *current_host_protocol_ != HostProtocol::FDMS_BASE24) {
		return HostSwitch::Status::PERM_FAILURE;
    }

    Host::Status status = host_fdms_.PerformKeyExchange(key_exchange);
    return ConvertStatus(status);
}

HostSwitch::Status HostSwitch::PerformFdmsSettlement(SettlementData& settle_msg, bool after_batch_upload) {
	if (!current_host_protocol_ || *current_host_protocol_ != HostProtocol::FDMS_BASE24)
		return HostSwitch::Status::PERM_FAILURE;

	Host::Status status = host_fdms_.PerformSettlement(settle_msg, after_batch_upload);
    return ConvertStatus(status);
}

HostSwitch::Status HostSwitch::PerformSettlement(SettlementData& settle_msg, bool after_batch_upload) {
	HostSwitch::Status output = HostSwitch::Status::PERM_FAILURE;
	if (!current_host_protocol_)
		return output;

	if (*current_host_protocol_ == HostProtocol::FDMS_BASE24){
		Host::Status status = host_fdms_.PerformSettlement(settle_msg, after_batch_upload);
        output = ConvertStatus(status);
	}
    else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT){
    	amex::AmexSettlementData amex_settle;
        amex_settle = BuildAmexSettlementData(settle_msg);
        amex::AmexHost::Status status_amex = GetAmexHost().PerformSettlement(amex_settle, after_batch_upload);
        FillSettlementDataWithAmexSettlementData(settle_msg, amex_settle);
        output = ConvertAmexStatus(status_amex);
    }
    else if(*current_host_protocol_ == HostProtocol::DINERS_DIRECT){
    	diners::DinersSettlementData diners_settle;
        diners_settle = BuildDinersSettlementData(settle_msg);
        diners::DinersHost::Status status_diners = GetDinersHost().PerformSettlement(diners_settle, after_batch_upload);
        FillSettlementDataWithDinersSettlementData(settle_msg, diners_settle);
        return ConvertDinersStatus(status_diners);
    }

	return output;
}

HostSwitch::Status HostSwitch::PerformFdmsTestTransaction(TestTransaction & test_tx) {
	if (!current_host_protocol_)
		return HostSwitch::Status::PERM_FAILURE;
    else if(*current_host_protocol_ == HostProtocol::FDMS_BASE24){
    	Host::Status status = host_fdms_.PerformTestTransaction(test_tx);
        return ConvertStatus(status);
    }
    else if(*current_host_protocol_ == HostProtocol::DINERS_DIRECT){
    	diners::TestTransaction diners_tx;
	    diners_tx.processing_code = "990000";
	    diners_tx.tpdu = test_tx.tpdu;
	    diners_tx.nii = test_tx.nii;
	    diners_tx.tid = test_tx.tid;
	    diners_tx.mid = test_tx.mid;
	    diners::DinersHost::Status status = GetDinersHost().PerformDinersTestTransaction(diners_tx);
	    return ConvertDinersStatus(status);
    }

	return HostSwitch::Status::PERM_FAILURE;
}

bool HostSwitch::WaitForConnection() {
	if (!current_host_protocol_)
		return false;

    uint32_t timeout = 30000;  // TODO: don't hardcode
    bool ret = true;
    if (*current_host_protocol_ == HostProtocol::FDMS_BASE24)
    	ret = host_fdms_.WaitForConnection(timeout);
    else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT)
    	ret = GetAmexHost().WaitForConnection(timeout);
    else if (*current_host_protocol_ == HostProtocol::DINERS_DIRECT)
    	ret = GetDinersHost().WaitForConnection(timeout);

    if (!ret)
    	current_host_protocol_ = stdx::nullopt;
    return ret;
}

bool HostSwitch::Disconnect() {
	if (!current_host_protocol_)
		return true;

	bool ret = true;
    if (*current_host_protocol_ == HostProtocol::FDMS_BASE24)
    	ret = host_fdms_.Disconnect();
    else if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT)
    	ret = GetAmexHost().Disconnect();
    else if (*current_host_protocol_ == HostProtocol::DINERS_DIRECT)
    	ret = GetDinersHost().Disconnect();

    current_host_protocol_ = stdx::nullopt;
    return ret;
}

bool HostSwitch::isAmex() {
	if (*current_host_protocol_ == HostProtocol::AMEX_DIRECT)
		return true;
    else
    	return false;
}

}

