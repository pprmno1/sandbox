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

#include <diners/test_transaction.h>
#include "test_transaction_message.h"
#include "protocol.h"
#include <iso8583/field_types.h>
#include <iso8583/encoder.h>
#include <utils/converter.h>
#include <utils/logger.h>
#include <stdx/ctime>
#include <iso8583/printer.h>
#include "diners_utils.h"


namespace diners {

iso8583::Apdu BuildEchoTestRequest(TestTransaction& tx) {
  EchoTestRequest message;

  // DE 03 PROCESSING CODE
  message.SetProcessingCode(tx.processing_code);

  // DE 24 NII
  message.SetNii(tx.nii);

  // DE 41 TID
  message.SetTid(tx.tid);

  // DE 42 MID
  message.SetMid(tx.mid);

  logger::debug(iso8583::Print(message.GetApdu()).c_str());

  return message.GetApdu();
}

bool ReadEchoTestResponse(const std::vector<uint8_t>& data, TestTransaction& tx) {
  EchoTestResponse response(data.data(), data.size());
  logger::debug(iso8583::Print(response.GetApdu()).c_str());
  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetNii() != tx.nii)||
      (response.GetTid() != tx.tid))
    return false;

  tx.host_datetime = response.GetHostDatetime();  //DE-12/ DE-13
  return true;
}

/**************************************
 * ECHO TEST REQUEST
 **************************************/
EchoTestRequest::EchoTestRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(800);
}

void EchoTestRequest::SetProcessingCode(const std::string processing_code) {
    apdu_.SetField(kFieldProcessingCode, processing_code);
}

void EchoTestRequest::SetNii(uint32_t nii) {
  apdu_.SetField(kFieldNii, nii);
}

void EchoTestRequest::SetTid(const std::string tid) {
  apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void EchoTestRequest::SetMid(const std::string mid) {
  apdu_.SetField(kFieldCardAcceptorId, mid);
}

iso8583::Apdu EchoTestRequest::GetApdu() const {
  return apdu_;
}

/**************************************
 * ECHO TEST RESPONSE
 **************************************/
EchoTestResponse::EchoTestResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool EchoTestResponse::IsValid() const {
  bool output = true;

  if (!apdu_.HasMti()) {
    return false;
  }
  int mti = apdu_.GetMti();
  if (mti != 810) {
    return false;
  }
  output &= apdu_.HasField(kFieldProcessingCode);
  output &= apdu_.HasField(kFieldTimeLocalTransaction);
  output &= apdu_.HasField(kFieldDateLocalTransaction);
  output &= apdu_.HasField(kFieldNii);
  output &= apdu_.HasField(kFieldCardAcceptorTerminalId);

  return output;
}

iso8583::Apdu EchoTestResponse::GetApdu() const {
  return apdu_;
}

std::string EchoTestResponse::GetProcessingCode() const {
  return apdu_.GetFieldAsString(kFieldProcessingCode);
}

time_t EchoTestResponse::GetHostDatetime() const {
  std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
  std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
  return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t EchoTestResponse::GetNii() const {
  return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string EchoTestResponse::GetTid() const {
  return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
