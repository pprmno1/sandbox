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

#include <diners/diners_transaction.h>
#include "key_request_message.h"
#include "protocol.h"
#include <iso8583/field_types.h>
#include <iso8583/encoder.h>
#include <utils/converter.h>
#include <utils/logger.h>
#include <iso8583/printer.h>
#include "diners_utils.h"


namespace diners {

iso8583::Apdu BuildKeyDownloadRequest(DinersTransaction& tx) {
  KeyDownloadRequest message;

  // DE 03 PROCESSING CODE
  message.SetProcessingCodeForKeyDownload();

  // DE 24 NII
  message.SetNii(tx.nii);

  // DE 41 TID
  message.SetTid(tx.tid);

  // DE 42 MID
  message.SetMid(tx.mid);

  logger::debug(iso8583::Print(message.GetApdu()).c_str());

  return message.GetApdu();
}

bool ReadKeyDownloadResponse(const utils::bytes& data, DinersTransaction& tx) {
  KeyDownloadResponse response(data.data(), data.size());

  logger::debug(iso8583::Print(response.GetApdu()).c_str());

  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetNii() != tx.nii) ||
      (response.GetTid() != tx.tid))
    return false;

  if(response.GetResponseCode()!="00")
	  return false;

  tx.tx_datetime = response.GetHostDatetime();
  tx.response_code = response.GetResponseCode();

  TMKDownloadMessage tmk_download_message;
  tmk_download_message.tmk = response.GetEncryptedTMK();

  return true;
}

/**************************************
 * KEY DOWNLOAD REQUEST
 **************************************/
KeyDownloadRequest::KeyDownloadRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(800);
}

std::string KeyDownloadRequest::SetProcessingCodeForKeyDownload() {
	std::string kKeyDownloadProcessingCode = "920000";
    apdu_.SetField(kFieldProcessingCode, kKeyDownloadProcessingCode);
    return kKeyDownloadProcessingCode;
}

void KeyDownloadRequest::SetNii(uint32_t nii) {
  apdu_.SetField(kFieldNii, nii);
}

void KeyDownloadRequest::SetTid(const std::string& tid) {
  apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void KeyDownloadRequest::SetMid(const std::string& mid) {
  apdu_.SetField(kFieldCardAcceptorId, mid);
}

iso8583::Apdu KeyDownloadRequest::GetApdu() const {
  return apdu_;
}

/**************************************
 * KEY DOWNLOAD RESPONSE
 **************************************/
KeyDownloadResponse::KeyDownloadResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool KeyDownloadResponse::IsValid() const {
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
  output &= apdu_.HasField(kFieldResponseCode);
  output &= apdu_.HasField(kFieldCardAcceptorTerminalId);
  output &= apdu_.HasField(kField62);

  return output;
}

iso8583::Apdu KeyDownloadResponse::GetApdu() const {
  return apdu_;
}

std::string KeyDownloadResponse::GetProcessingCode() const {
  return apdu_.GetFieldAsString(kFieldProcessingCode);
}

time_t KeyDownloadResponse::GetHostDatetime() const {
  std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
  std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
  return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t KeyDownloadResponse::GetNii() const {
  return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string KeyDownloadResponse::GetResponseCode() const {
  return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string KeyDownloadResponse::GetTid() const {
  return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

std::vector<uint8_t> KeyDownloadResponse::GetEncryptedTMK() const {
  std::vector<uint8_t> field_content = apdu_.GetFieldAsBytes(kField62);
  std::vector<uint8_t> tmk(field_content.begin() + 2, field_content.begin() + 18);
  return tmk;
}

}

