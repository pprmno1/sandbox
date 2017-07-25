/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2015-2017 INGENICO S.A.
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
#include "offline_sale_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildOfflineSaleRequest(DinersTransaction& tx) {
	OfflineSaleRequest message;

    // DE 02 PAN
    message.SetPan(*tx.pan);

    // DE 03 PROCESSING CODE
    tx.processing_code = message.SetProcessingCode(tx.transaction_type, false);

    // DE 04 AMOUNT
    message.SetAmount(*tx.GetTotalAmount());

    // DE 11 STAN
    message.SetStan(tx.stan);

    // DE 12-13 DATE & TIME
    message.SetHostDatetime(tx.tx_datetime);

    // DE 14 EXPIRATION DATE
    message.SetExpirationDate(tx.expiration_date);

    // DE 22 POS ENTRY MODE
    tx.orig_pos_entry_mode = message.SetPosEntryMode(tx.pos_entry_mode);

    // DE 23 PAN SEQUENCE NUMBER
    if(tx.pan_sequence_number){
    	message.SetPanSequenceNumber(*tx.pan_sequence_number);
    }

    // DE 24 NII
    message.SetNii(tx.nii);

    // DE 25 POS CONDITION CODE
    message.SetPosConditionCode(tx.pos_condition_code);

    // DE 37 RETRIEVAL REFERENCE NUMBER
    if(tx.transaction_type == SALE_COMPLETION){
    	message.SetRrn(tx.rrn);
    }

	// DE 38 Auth Code
    message.SetAuthorizationCode(tx.auth_id_response);

    //DE 39 Response Code
    if(tx.transaction_type == SALE_COMPLETION){
    	message.SetResponseCode(tx.response_code);
    }

    // DE 41 TID
    message.SetTid(tx.tid);

    // DE 42 MID
    message.SetMid(tx.mid);

    // DE 60 BATCH NUMBER
    message.SetBatchNumber(tx.batch_number);

    // DE 62 INVOICE NUMBER
    message.SetInvoiceNumber(tx.invoice_number);

    logger::debug(iso8583::Print(message.GetApdu()));

    return message.GetApdu();
}

bool ReadOfflineSaleResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
	OfflineSaleResponse response(data.data(), data.size());

    logger::debug(iso8583::Print(response.GetApdu()));

  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetStan() != tx.stan) ||
      (response.GetNii() != tx.nii) ||
      (response.GetTid() != tx.tid))
    return false;

  //tx.tx_datetime = response.GetHostDatetime();
  tx.rrn = response.GetRrn();
  tx.response_code = response.GetResponseCode();       //DE-39

  return true;
}

/**************************************
 * OFFLINE SALE REQUEST
 **************************************/
OfflineSaleRequest::OfflineSaleRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(220);
}

void OfflineSaleRequest::SetPan(const types::Pan& pan) {
    apdu_.SetField(kFieldPan, pan.ToString());
}

std::string OfflineSaleRequest::SetProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
    std::string kSaleProcessingCode = GetDinersProcessingCode(trans_type, is_void_txn);
    apdu_.SetField(kFieldProcessingCode, kSaleProcessingCode);
    return kSaleProcessingCode;
}

void OfflineSaleRequest::SetAmount(const types::Amount& amount) {
    apdu_.SetField(kFieldAmount, amount.GetValue());
}

void OfflineSaleRequest::SetStan(std::uint32_t stan) {
    apdu_.SetField(kFieldStan, stan);
}

void OfflineSaleRequest::SetExpirationDate(const std::string& expiration_date) {
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

void OfflineSaleRequest::SetHostDatetime(time_t time_stamp) {
    apdu_.SetField(kFieldTimeLocalTransaction, iso8583::IsoTimeFromTimestamp(time_stamp));
    apdu_.SetField(kFieldDateLocalTransaction, iso8583::IsoDateFromTimestamp(time_stamp));
}

std::string OfflineSaleRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
    std::string PosEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, GetPosEntryMode(pos_entry_mode));
    return PosEntryMode;
}

void OfflineSaleRequest::SetPanSequenceNumber(const unsigned int pan_sequence){
	apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void OfflineSaleRequest::SetNii(std::uint32_t nii) {
    apdu_.SetField(kFieldNii, nii);
}

void OfflineSaleRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {
    apdu_.SetField(kFieldPosConditionCode, GetDinersConditionCode(pos_condition_code));
}

void OfflineSaleRequest::SetRrn(const std::string& retrival_response_code){        //DE 37
    apdu_.SetField(kFieldRrn, retrival_response_code);
}

void OfflineSaleRequest::SetAuthorizationCode(const std::string& authorization_code) {
    apdu_.SetField(kFieldAuthorizationId, authorization_code);
}

void OfflineSaleRequest::SetResponseCode(const std::string& response_code) {
    apdu_.SetField(kFieldResponseCode, response_code);
}

void OfflineSaleRequest::SetTid(const std::string& tid) {
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void OfflineSaleRequest::SetMid(const std::string& mid) {
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

void OfflineSaleRequest::SetBatchNumber(uint32_t batch_num){             //DE 60
	auto batch_str = utils::ToString(batch_num);
	std::string value = iso8583::RightAligned(batch_str, 6, '0');
	apdu_.SetField(kField60, value);
}

void OfflineSaleRequest::SetInvoiceNumber(std::uint32_t invoice) {  // field 62
    auto invoice_str = utils::ToString(invoice);
    std::string value = iso8583::RightAligned(invoice_str, 6, '0');
    apdu_.SetField(kField62, value);
}

iso8583::Apdu OfflineSaleRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * OFFLINE SALE RESPONSE
 **************************************/
OfflineSaleResponse::OfflineSaleResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool OfflineSaleResponse::IsValid() const {
	bool output = true;

    if (!apdu_.HasMti()) {
    	return false;
    }
    int mti = apdu_.GetMti();
    if (mti != 230) {
    	return false;
    }

    output &= apdu_.HasField(kFieldProcessingCode);
    output &= apdu_.HasField(kFieldStan);
    output &= apdu_.HasField(kFieldNii);
    output &= apdu_.HasField(kFieldRrn);
    output &= apdu_.HasField(kFieldResponseCode);
    output &= apdu_.HasField(kFieldCardAcceptorTerminalId);
    return output;
}

iso8583::Apdu OfflineSaleResponse::GetApdu() const {
	return apdu_;
}

std::string OfflineSaleResponse::GetProcessingCode() const {
    return apdu_.GetFieldAsString(kFieldProcessingCode);
}

time_t OfflineSaleResponse::GetHostDatetime() const {
    std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
    std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
    return utils::GetDatetimefromIso8583Format(date, time);
}

std::uint32_t OfflineSaleResponse::GetStan() const {
    return apdu_.GetFieldAsInteger(kFieldStan);
}

std::uint32_t OfflineSaleResponse::GetNii() const {
    return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string OfflineSaleResponse::GetRrn() const {
    return apdu_.GetFieldAsString(kFieldRrn);
}

std::string OfflineSaleResponse::GetResponseCode() const {
    return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string OfflineSaleResponse::GetTid() const {
    return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
