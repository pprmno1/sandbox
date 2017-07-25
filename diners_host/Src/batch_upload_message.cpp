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
#include "batch_upload_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildBatchUploadRequest(DinersTransaction& tx,
                                      std::uint32_t batch_upload_stan) {
  BatchUploadRequest message;

  // DE 02 PAN
  message.SetPan(*tx.pan);

  // DE 03 PROCESSING CODE
  tx.processing_code = message.SetProcessingCode(tx.transaction_type, false);

  // DE 04 AMOUNT
     if(tx.transaction_type == DinersTransactionType::PREAUTH || tx.transaction_type == DinersTransactionType::AUTHORIZATION){
         if (tx.preauth_amount)
         	message.SetAmount(*tx.GetTotalPreauthAmount());
     }
     else {
         if (tx.amount)
            message.SetAmount(*tx.GetTotalAmount());
     }

  // DE 11 STAN
  message.SetStan(batch_upload_stan);

  // DE 12/13 TIME/DATE
  message.SetHostDatetime(tx.tx_datetime);

  // DE 14 EXPIRATION DATE
  message.SetExpirationDate(tx.expiration_date);

  // DE 22 POS ENTRY MODE
  message.SetPosEntryMode(tx.pos_entry_mode);

  // DE 24 NII
  message.SetNii(tx.nii);

  // DE 25 POS CONDITION CODE
  message.SetPosConditionCode(tx.pos_condition_code);

  //DE 37 RRN
  message.SetRrn(tx.rrn);

  //DE 38 Auth Code
  message.SetAuthorizationCode(tx.auth_id_response);

  // DE 41 TID
  message.SetTid(tx.tid);

  // DE 42 MID
  message.SetMid(tx.mid);

  //DE60
  message.SetField60(tx.transaction_type, tx.stan);

  //DE 62
  message.SetInvoiceNumber(tx.invoice_number);

  logger::debug(iso8583::Print(message.GetApdu()).c_str());

  return message.GetApdu();
}

bool ReadBatchUploadResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
  BatchUploadResponse response(data.data(), data.size());

  logger::debug(iso8583::Print(response.GetApdu()).c_str());

  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetNii() != tx.nii) ||
	  //(response.GetRrn() != tx.rrn) ||
      (response.GetTid() != tx.tid))
    return false;

  if (response.GetResponseCode() != "00")
    return false;

  return true;
}

/**************************************
 * BTACH UPLOAD REQUEST
 **************************************/
BatchUploadRequest::BatchUploadRequest()
    : apdu_(GetProtocolSpec()) {
  apdu_.SetMti(320);
}

void BatchUploadRequest::SetPan(const types::Pan& pan) {
  apdu_.SetField(kFieldPan, pan.ToString());
}

std::string BatchUploadRequest::SetProcessingCode(DinersTransactionType & trans_type,
                                           bool is_void_txn) {
  apdu_.SetField(kFieldProcessingCode, GetDinersProcessingCode(trans_type, is_void_txn));
  return GetDinersProcessingCode(trans_type, is_void_txn);
}

void BatchUploadRequest::SetAmount(const types::Amount& amount) {
  apdu_.SetField(kFieldAmount, amount.GetValue());
}

void BatchUploadRequest::SetStan(uint32_t batch_upload_stan) {
  apdu_.SetField(kFieldStan, batch_upload_stan);
}

void BatchUploadRequest::SetHostDatetime(time_t & time_stamp) {
  apdu_.SetField(kFieldTimeLocalTransaction,
                 iso8583::IsoTimeFromTimestamp(time_stamp));
  apdu_.SetField(kFieldDateLocalTransaction,
                 iso8583::IsoDateFromTimestamp(time_stamp));
}

void BatchUploadRequest::SetExpirationDate(const std::string& expiration_date) {
  apdu_.SetField(kFieldDateExpiration, expiration_date);
}

void BatchUploadRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
  apdu_.SetField(kFieldPosEntryMode,
                 GetPosEntryMode(pos_entry_mode));
}

void BatchUploadRequest::SetNii(std::uint32_t nii) {
  apdu_.SetField(kFieldNii, nii);
}

void BatchUploadRequest::SetPosConditionCode(
    types::PosConditionCode & pos_condition_code) {
  apdu_.SetField(kFieldPosConditionCode,
                 GetDinersConditionCode(pos_condition_code));
}

void BatchUploadRequest::SetRrn(const std::string& retrival_response_code) {
  apdu_.SetField(kFieldRrn, retrival_response_code);
}

void BatchUploadRequest::SetAuthorizationCode(
    const std::string& authorization_code) {
  apdu_.SetField(kFieldAuthorizationId, authorization_code);
}

void BatchUploadRequest::SetTid(const std::string& tid) {
  apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void BatchUploadRequest::SetMid(const std::string& mid) {
  apdu_.SetField(kFieldCardAcceptorId, mid);
}

void BatchUploadRequest::SetInvoiceNumber(std::uint32_t invoice) {  // field 62
  auto invoice_str = utils::ToString(invoice);
  std::string value = iso8583::RightAligned(invoice_str, 6, '0');
  apdu_.SetField(kField62, value);
}

void BatchUploadRequest::SetField60(const DinersTransactionType &transaction_type,
                                    const std::uint32_t stan) {

  std::stringstream transaction_data;
  std::stringstream padded_stan;
  std::string reserve_data = "            ";  //Reserved subfield (12 character spaces)

  padded_stan << stan;

  //TODO: other types
  switch (transaction_type) {
    case DinersTransactionType::SALE:
      case DinersTransactionType::REFUND:
      transaction_data << "0200"
                       << iso8583::RightAligned(padded_stan.str(), 6, '0')
                       << reserve_data;
      break;
    case DinersTransactionType::SALE_COMPLETION:
      case DinersTransactionType::OFFLINE_SALE:
      transaction_data << "0220"
                       << iso8583::RightAligned(padded_stan.str(), 6, '0')
                       << reserve_data;
    default:
      break;
  }

  apdu_.SetField(kField60, transaction_data.str());
}

iso8583::Apdu BatchUploadRequest::GetApdu() const {
  return apdu_;
}

/**************************************
 * BATCH UPLOAD RESPONSE
 **************************************/
BatchUploadResponse::BatchUploadResponse(const std::uint8_t* data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool BatchUploadResponse::IsValid() const {
  bool output = true;

  if (!apdu_.HasMti()) {
    return false;
  }
  int mti = apdu_.GetMti();
  if (mti != 330) {
    return false;
  }
  output &= apdu_.HasField(kFieldProcessingCode);
  output &= apdu_.HasField(kFieldStan);
  output &= apdu_.HasField(kFieldTimeLocalTransaction);
  output &= apdu_.HasField(kFieldDateLocalTransaction);
  output &= apdu_.HasField(kFieldNii);
  output &= apdu_.HasField(kFieldRrn);
  output &= apdu_.HasField(kFieldResponseCode);
  output &= apdu_.HasField(kFieldCardAcceptorTerminalId);

  return output;
}

iso8583::Apdu BatchUploadResponse::GetApdu() const {
  return apdu_;
}

std::string BatchUploadResponse::GetProcessingCode() const {
  return apdu_.GetFieldAsString(kFieldProcessingCode);
}

std::uint32_t BatchUploadResponse::GetStan() const {
  return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t BatchUploadResponse::GetHostDatetime() const {
  std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
  std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
  return utils::GetDatetimefromIso8583Format(date, time);
}

std::uint32_t BatchUploadResponse::GetNii() const {
  return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string BatchUploadResponse::GetRrn() const {
  return apdu_.GetFieldAsString(kFieldRrn);
}

std::string BatchUploadResponse::GetResponseCode() const {
  return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string BatchUploadResponse::GetTid() const {
  return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}
}
