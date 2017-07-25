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
#include "preauth_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildPreAuthRequest(DinersTransaction& tx) {
	PreAuthRequest message;

    // DE 02 PAN
    if (tx.pos_entry_mode == types::PosEntryMode::MANUAL || tx.pos_entry_mode == types::PosEntryMode::FALLBACK_MANUAL ) {
    	if (tx.pan)
    		message.SetPan(*tx.pan);
    }

    // DE 03 PROCESSING CODE
    tx.processing_code = message.SetProcessingCode(tx.transaction_type, false);

    // DE 04 AMOUNT
    auto total_amount = tx.GetTotalPreauthAmount();
    if (total_amount) {
    	message.SetAmount(*total_amount);
    }

    // DE 11 STAN
    message.SetStan(tx.stan);

    // DE 14 EXPIRATION DATE
    if (tx.pos_entry_mode == types::PosEntryMode::MANUAL || tx.pos_entry_mode == types::PosEntryMode::FALLBACK_MANUAL ) {
    	if(!tx.expiration_date.empty())
    		message.SetExpirationDate(tx.expiration_date);
    }

    // DE 22 POS ENTRY MODE TODO: find a way to save this to batch
    tx.orig_pos_entry_mode = message.SetPosEntryMode(tx.pos_entry_mode);

    // DE 23 PAN SEQUENCE NUMBER
    if(tx.pan_sequence_number){
    	message.SetPanSequenceNumber(*tx.pan_sequence_number);
    }

    // DE 24 NII
    message.SetNii(tx.nii);

    // DE 25 POS CONDITION CODE
    message.SetPosConditionCode(tx.pos_condition_code);

    // DE 35 TRACK 2
    if (tx.track2) {
    	message.SetTrack2(*tx.track2);
    }

    // DE 41 TID
    message.SetTid(tx.tid);

    // DE 42 MID
    message.SetMid(tx.mid);

    // DE 48 ADDITIONAL DATA
    if(!tx.cvv.empty()){
        message.SetAdditionalData(tx.cvv);
    }

    // DE 52 PIN BLOCK
    if (tx.pin_block) {
        message.SetPinBlock(*tx.pin_block);
    }

    // DE 55 ICC DATA
    if (!tx.icc_data->empty()) {
    	message.SetEmvData(*tx.icc_data);
    }

    // DE 60 BATCH NUMBER
    message.SetBatchNumber(tx.batch_number);

    // DE 62 INVOICE NUMBER
    message.SetInvoiceNumber(tx.invoice_number);

    logger::debug(iso8583::Print(message.GetApdu()).c_str());

    return message.GetApdu();
}

bool ReadPreAuthResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
	PreAuthResponse response(data.data(), data.size());

    logger::debug(iso8583::Print(response.GetApdu()).c_str());

    if (!response.IsValid() ||
    		(response.GetProcessingCode() != tx.processing_code) ||
            (response.GetStan() != tx.stan) ||
            (response.GetNii() != tx.nii) ||
            (response.GetTid() != tx.tid))
    	return false;

    tx.tx_datetime = response.GetHostDatetime();  //DE-12/ DE-13
    tx.rrn = response.GetRrn();
    tx.response_code = response.GetResponseCode();       //DE-39

    auto auth_id_response = response.GetAuthIdResponse();   //DE-38
    if (auth_id_response)
    	tx.auth_id_response = *auth_id_response;

    auto emv_data = response.GetEmvData();                 //DE-55
    if (emv_data)
    	tx.issuer_emv_response = *emv_data;

    return true;
}

/**************************************
 * PREAUTH REQUEST
 **************************************/
PreAuthRequest::PreAuthRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(100);
}

void PreAuthRequest::SetPan(const types::Pan& pan) {
    apdu_.SetField(kFieldPan, pan.ToString());
}

std::string PreAuthRequest::SetProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
    std::string kSaleProcessingCode = GetDinersProcessingCode(trans_type, is_void_txn);
    apdu_.SetField(kFieldProcessingCode, kSaleProcessingCode);
    return kSaleProcessingCode;
}

void PreAuthRequest::SetAmount(const types::Amount& amount) {
    apdu_.SetField(kFieldAmount, amount.GetValue());
}

void PreAuthRequest::SetStan(uint32_t stan) {
    apdu_.SetField(kFieldStan, stan);
}

void PreAuthRequest::SetExpirationDate(const std::string& expiration_date) {
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

std::string PreAuthRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
    std::string PosEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, GetPosEntryMode(pos_entry_mode));
    return PosEntryMode;
}

void PreAuthRequest::SetPanSequenceNumber(const unsigned int pan_sequence) {
  apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void PreAuthRequest::SetNii(uint32_t nii) {
    apdu_.SetField(kFieldNii, nii);
}

void PreAuthRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {
    apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void PreAuthRequest::SetTrack2(const std::vector<uint8_t>& track2) {
    apdu_.SetField(kFieldTrack2Data, track2);
}

void PreAuthRequest::SetTid(const std::string& tid) {
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void PreAuthRequest::SetMid(const std::string& mid) {
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

void PreAuthRequest::SetAdditionalData(const std::string & cvv) {
    apdu_.SetField(kFieldAdditionalDataPrivate, cvv);
}

void PreAuthRequest::SetPinBlock(const std::vector<uint8_t>& pin_block) {
	apdu_.SetField(kFieldPinBlock, pin_block);
}

void PreAuthRequest::SetEmvData(const std::vector<uint8_t>& emv_data) {
    apdu_.SetField(kFieldIccData, emv_data);
}

void PreAuthRequest::SetBatchNumber(uint32_t batch_num){             //DE 60
	auto batch_str = utils::ToString(batch_num);
	std::string value = iso8583::RightAligned(batch_str, 6, '0');
	apdu_.SetField(kField60, value);
}

void PreAuthRequest::SetInvoiceNumber(uint32_t invoice) {  // field 62
    auto invoice_str = utils::ToString(invoice);
    std::string value = iso8583::RightAligned(invoice_str, 6, '0');
    apdu_.SetField(kField62, value);
}

iso8583::Apdu PreAuthRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * PREAUTH RESPONSE
 **************************************/
PreAuthResponse::PreAuthResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool PreAuthResponse::IsValid() const {
    bool output = true;

    if (!apdu_.HasMti()) {
    	return false;
    }
    int mti = apdu_.GetMti();
    if (mti != 110) {
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

iso8583::Apdu PreAuthResponse::GetApdu() const {
    return apdu_;
}

std::string PreAuthResponse::GetProcessingCode() const {
    return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t PreAuthResponse::GetStan() const {
    return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t PreAuthResponse::GetHostDatetime() const {
    std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
    std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
    return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t PreAuthResponse::GetNii() const {
    return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string PreAuthResponse::GetRrn() const {
    return apdu_.GetFieldAsString(kFieldRrn);
}

stdx::optional<std::string> PreAuthResponse::GetAuthIdResponse() const {
    if (apdu_.HasField(kFieldAuthorizationId)) {
    	return apdu_.GetFieldAsString(kFieldAuthorizationId);
    }
    return stdx::nullopt;
}

std::string PreAuthResponse::GetResponseCode() const {
    return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string PreAuthResponse::GetTid() const {
    return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

stdx::optional<std::vector<uint8_t>> PreAuthResponse::GetEmvData() const {
    if (apdu_.HasField(kFieldIccData)) {
    	return apdu_.GetFieldAsBytes(kFieldIccData);
    }
    return stdx::nullopt;
}

}
