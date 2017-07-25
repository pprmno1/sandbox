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

#include "tc_upload_message.h"
#include "protocol.h"
#include "diners_utils.h"
#include <iso8583/field_types.h>
#include <iso8583/encoder.h>
#include <utils/converter.h>
#include <utils/logger.h>
#include <iso8583/printer.h>
#include "diners_utils.h"


namespace diners {

iso8583::Apdu BuildTcUploadRequest(DinersTransaction& tx) {
  TcUploadRequest message;

  message.SetPan(*tx.pan);

  // DE 03 PROCESSING CODE
  tx.processing_code = message.SetProcessingCode();

  // DE 04 AMOUNT
  auto total_amount = tx.GetTotalAmount();
  if (total_amount) {
	  message.SetAmount(*total_amount);
  }

  // DE 11 STAN
  message.SetStan(tx.stan);

  // DE 12-13 DATE & TIME
  message.SetHostDatetime(tx.tx_datetime);

  // DE 22 POS ENTRY MODE
  message.SetPosEntryMode(tx.pos_entry_mode); //TODO: use DinersTransaction::orig_pos_entry_mode storage?

  // DE 23 PAN SEQUENCE NUMBER
  if(tx.pan_sequence_number){
	  message.SetPanSequenceNumber(*tx.pan_sequence_number);
  }

  // DE 24 NII
  message.SetNii(tx.nii);

  // DE 25 POS CONDITION CODE
  message.SetPosConditionCode(tx.pos_condition_code);

  // DE 37 RRN
  message.SetRrn(tx.rrn);

  // DE 39 RESPONSE CODE
  message.SetResponseCode(tx.response_code);

  // DE 41 TID
  message.SetTid(tx.tid);

  // DE 42 MID
  message.SetMid(tx.mid);

  // DE 55 ICC DATA
  if (!tx.icc_data->empty()) {
      message.SetEmvData(*tx.icc_data);
  }

  message.SetInvoiceNumber(tx.invoice_number);

  logger::debug(iso8583::Print(message.GetApdu()).c_str());

  return message.GetApdu();
}

bool ReadTcUploadResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
  TcUploadResponse response(data.data(), data.size());
  logger::debug(iso8583::Print(response.GetApdu()).c_str());
  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetStan() != tx.stan) ||
      (response.GetNii() != tx.nii) ||
	  //(response.GetRrn() != tx.rrn)||
      (response.GetTid() != tx.tid))
    return false;

  tx.tx_datetime = response.GetHostDatetime();  //DE-12/ DE-13
  tx.response_code = response.GetResponseCode();       //DE-39
  return true;
}

/**************************************
 * TRANSACTION CERTIFICATE UPLOAD REQUEST
 **************************************/
TcUploadRequest::TcUploadRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(320);
}

void TcUploadRequest::SetPan(const types::Pan& pan) {
	apdu_.SetField(kFieldPan, pan.ToString());
}

std::string TcUploadRequest::SetProcessingCode() {
	std::string kTcUploadProcessingCode = "940000";
    apdu_.SetField(kFieldProcessingCode, kTcUploadProcessingCode);
    return kTcUploadProcessingCode;
}

void TcUploadRequest::SetAmount(const types::Amount& amount) {
	apdu_.SetField(kFieldAmount, amount.GetValue());
}

void TcUploadRequest::SetStan(uint32_t stan) {
	apdu_.SetField(kFieldStan, stan);
}

void TcUploadRequest::SetHostDatetime(time_t time_stamp) {
  apdu_.SetField(kFieldTimeLocalTransaction,
                 iso8583::IsoTimeFromTimestamp(time_stamp));
  apdu_.SetField(kFieldDateLocalTransaction,
                 iso8583::IsoDateFromTimestamp(time_stamp));
}

std::string TcUploadRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
	std::string PoseEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, PoseEntryMode);
    return PoseEntryMode;
}

void TcUploadRequest::SetPanSequenceNumber(const unsigned int pan_sequence) {
  apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void TcUploadRequest::SetNii(uint32_t nii) {
  apdu_.SetField(kFieldNii, nii);
}

void TcUploadRequest::SetPosConditionCode(
    types::PosConditionCode & pos_condition_code) {
  apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void TcUploadRequest::SetRrn(const std::string& retrival_response_code){        //DE 37
  apdu_.SetField(kFieldRrn, retrival_response_code);
}

void TcUploadRequest::SetResponseCode(
    const std::string& response_code) {
  apdu_.SetField(kFieldResponseCode, response_code);
}

void TcUploadRequest::SetTid(const std::string& tid) {
  apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void TcUploadRequest::SetMid(const std::string& mid) {
  apdu_.SetField(kFieldCardAcceptorId, mid);
}

void TcUploadRequest::SetEmvData(const std::vector<uint8_t>& emv_data) {
  apdu_.SetField(kFieldIccData, emv_data);
}

void TcUploadRequest::SetInvoiceNumber(uint32_t invoice) {  // field 62
  auto invoice_str = utils::ToString(invoice);
  std::string value = iso8583::RightAligned(invoice_str, 6, '0');
  apdu_.SetField(kField62, value);
}

iso8583::Apdu TcUploadRequest::GetApdu() const {
  return apdu_;
}

/**************************************
 * TRANSACTION CERTIFICATE UPLOAD RESPONSE
 **************************************/
TcUploadResponse::TcUploadResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool TcUploadResponse::IsValid() const {
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

iso8583::Apdu TcUploadResponse::GetApdu() const {
  return apdu_;
}

std::string TcUploadResponse::GetProcessingCode() const {
  return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t TcUploadResponse::GetStan() const {
  return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t TcUploadResponse::GetHostDatetime() const {
  std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
  std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
  return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t TcUploadResponse::GetNii() const {
  return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string TcUploadResponse::GetRrn() const {
  return apdu_.GetFieldAsString(kFieldRrn);
}

std::string TcUploadResponse::GetResponseCode() const {
  return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string TcUploadResponse::GetTid() const {
  return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
