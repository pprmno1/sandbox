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

#include "Key_exchange_message.h"
#include <diners/key_exchange.h>

#include "protocol.h"
#include "field_utils.h"
#include <iso8583/field_types.h>
#include <iso8583/encoder.h>
#include <utils/converter.h>
#include <utils/logger.h>
#include <iso8583/printer.h>
#include "apdu_utils.h"
#include <tpcore/calendar.h>

using namespace types;

namespace diners {

// PRIVATE DECLARATIONS
void GetEncryptedTidLocation(const iso8583::Apdu& apdu,
                             KeyExchange& key_exchange);
void GetEncryptedPinKey(const iso8583::Apdu& apdu,
                        KeyExchange& key_exchange);
void GetEncryptedTLEKey(const iso8583::Apdu& apdu,
                        KeyExchange& key_exchange);

// PRIVATE DATA
static const int kKeyExchangeRequestMti = 800;
static const std::string kKeyExchangeProcessingCode = "920000";

static const std::vector<int> kKeyExchangeRspMandatoryFields = {
    kFieldProcessingCode,
    kFieldStan, kFieldTimeLocalTransaction, kFieldDateLocalTransaction,
    kFieldNii,
    kFieldResponseCode, kFieldCardAcceptorTerminalId,
    kField62, kField63 };

// PUBLIC FUNCTIONS
iso8583::Apdu BuildKeyExchangeRequest(const KeyExchange& key_exchange) {
  iso8583::Apdu request_apdu(diners_spec());

  request_apdu.SetMti(kKeyExchangeRequestMti);

  SetProcessingCode(kKeyExchangeProcessingCode, request_apdu);
  SetStan(key_exchange.stan, request_apdu);
  SetNii(key_exchange.nii, request_apdu);
  SetTid(key_exchange.tid, request_apdu);
  SetMid(key_exchange.mid, request_apdu);

  return request_apdu;
}

bool ReadAndValidateKeyExchangeResponse(const iso8583::Apdu& request_apdu,
                                        const std::vector<uint8_t>& data,
                                        KeyExchange& key_exchange) {

  iso8583::Apdu response_apdu(diners_spec(), data.data(),
                              data.size());

  logger::debug(iso8583::Print(response_apdu).c_str());

  if (!CheckMandatoryFields(response_apdu, kKeyExchangeRspMandatoryFields)
      || !ValidateBasicFields(request_apdu, response_apdu))
    return false;

  key_exchange.tx_datetime = *GetHostDatetime(response_apdu);
  key_exchange.response_code = GetResponseCode(response_apdu);
  key_exchange.mid = GetMid(response_apdu);

  if (GetResponseCode(response_apdu) != "00")
    return false;

  if (HasDateTime(response_apdu)) {
    tpcore::SetSystemDatetime(key_exchange.tx_datetime);
  }

  if (!response_apdu.HasField(kField62) || !response_apdu.HasField(kField63))
    return false;

  //GetEncryptedTidLocation(response_apdu,  key_exchange);

  GetEncryptedPinKey(response_apdu, key_exchange);

  GetEncryptedTLEKey(response_apdu, key_exchange);

  return true;

}

// PRIVATE FUNCTIONS
void GetEncryptedTidLocation(const iso8583::Apdu& apdu,
                             KeyExchange& key_exchange) {
    std::vector<uint8_t> field_content = apdu.GetFieldAsBytes(kField62);
    std::vector<uint8_t> tid_location(field_content.begin() + 16, field_content.end());
    key_exchange.tid_location = tid_location;
}

void GetEncryptedPinKey(const iso8583::Apdu& apdu,
                        KeyExchange& key_exchange) {
    std::vector<uint8_t> field_content = apdu.GetFieldAsBytes(kField63);

    std::vector<uint8_t> pvt_key = ReadFieldFromTable("KP", field_content);
    std::vector<uint8_t> value(pvt_key.begin(), pvt_key.begin() + 16);
    key_exchange.pin_key = value;
}

void GetEncryptedTLEKey(const iso8583::Apdu& apdu,
                        KeyExchange& key_exchange) {
    std::vector<uint8_t> field_content = apdu.GetFieldAsBytes(kField63);

    std::vector<uint8_t> pvt_key = ReadFieldFromTable("KP", field_content);
    std::vector<uint8_t> value(pvt_key.begin() + 16, pvt_key.begin() + 32);
    key_exchange.tle_key = value;
}

}
