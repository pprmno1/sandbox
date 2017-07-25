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
#include "settlement_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildSettlementRequest(DinersSettlementData& diners_settle_data, bool after_batch_upload) {
  SettlementRequest message;

  // DE 3 PROCESSING CODE
  diners_settle_data.processing_code = message.SetProcessingCode(after_batch_upload);

  // DE 11 STAN
  message.SetStan(diners_settle_data.stan);

  // DE 24 NII
  message.SetNii(diners_settle_data.nii);

  // DE 41 TID
  message.SetTid(diners_settle_data.tid);

  // DE 42 MID
  message.SetMid(diners_settle_data.mid);

  // DE 60
  message.SetBatchNumber(diners_settle_data.batch_number);

  // DE 63
  message.SetBatchTotal(diners_settle_data.batch_summary);

  logger::debug(iso8583::Print(message.GetApdu()).c_str());

  return message.GetApdu();
}

bool ReadSettlementResponse(const std::vector<uint8_t>& data, DinersSettlementData& settle_msg) {
  SettlementResponse response(data.data(), data.size());

  logger::debug(iso8583::Print(response.GetApdu()).c_str());

  if (!response.IsValid() ||
      (response.GetProcessingCode() != settle_msg.processing_code) ||
      (response.GetStan() != settle_msg.stan) ||
      (response.GetNii() != settle_msg.nii) ||
      (response.GetTid() != settle_msg.tid))
    return false;

  settle_msg.tx_datetime = response.GetHostDatetime();

  settle_msg.response_code = response.GetResponseCode();       //DE-39

  settle_msg.rrn = response.GetRrn();                           //DE-37

  return true;
}

/**************************************
 * SETTLEMENT REQUEST
 **************************************/
SettlementRequest::SettlementRequest()
    : apdu_(GetProtocolSpec()) {
  apdu_.SetMti(500);
}

std::string SettlementRequest::SetProcessingCode(bool after_batch_upload) {
  std::string processing_code;
  if (!after_batch_upload) {
    processing_code = "920000";
  } else {
    processing_code = "960000";
  }
  apdu_.SetField(kFieldProcessingCode, processing_code);
  return processing_code;
}

void SettlementRequest::SetStan(uint32_t stan) {
  apdu_.SetField(kFieldStan, stan);
}

void SettlementRequest::SetNii(uint32_t nii) {
  apdu_.SetField(kFieldNii, nii);
}

void SettlementRequest::SetTid(const std::string& tid) {
  apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void SettlementRequest::SetMid(const std::string& mid) {
  apdu_.SetField(kFieldCardAcceptorId, mid);
}

void SettlementRequest::SetBatchNumber(uint32_t batch_number) {
  std::stringstream batch_num;
  batch_num << batch_number;
  apdu_.SetField(kField60, iso8583::RightAligned(batch_num.str(), 6, '0'));
}

void SettlementRequest::SetBatchTotal(BatchTotalsForDinersHost & Diners_batch_totals) {  // field 63

    std::string batch_totals =
  	    utils::NumericPadded(Diners_batch_totals.sales_total.count, 3) +
        utils::NumericPadded(Diners_batch_totals.sales_total.total, 12) +
        utils::NumericPadded(Diners_batch_totals.refunds_total.count, 3) +
        utils::NumericPadded(Diners_batch_totals.refunds_total.total, 12);

    const std::string kEmptyBlock(30, '0');
    batch_totals += kEmptyBlock;
    batch_totals += kEmptyBlock;

    std::vector<uint8_t> batch_totals_in_bytes(batch_totals.c_str(), batch_totals.c_str() + batch_totals.length());

    apdu_.SetField(kField63, batch_totals_in_bytes);
}

iso8583::Apdu SettlementRequest::GetApdu() const {
  return apdu_;
}

/**************************************
 * SETTLEMENT RESPONSE
 **************************************/
SettlementResponse::SettlementResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool SettlementResponse::IsValid() const {
  bool output = true;

  if (!apdu_.HasMti()) {
    return false;
  }
  int mti = apdu_.GetMti();
  if (mti != 510) {
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

iso8583::Apdu SettlementResponse::GetApdu() const {
  return apdu_;
}

std::string SettlementResponse::GetProcessingCode() const {
  return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t SettlementResponse::GetStan() const {
  return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t SettlementResponse::GetHostDatetime() const {
  std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
  std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
  return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t SettlementResponse::GetNii() const {
  return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string SettlementResponse::GetRrn() const {
  return apdu_.GetFieldAsString(kFieldRrn);
}

std::string SettlementResponse::GetResponseCode() const {
  return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string SettlementResponse::GetTid() const {
  return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
