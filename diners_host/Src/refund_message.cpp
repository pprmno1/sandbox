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
#include "refund_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildRefundRequest(DinersTransaction& tx) {
	RefundRequest message;

    // DE 02 PAN
	if(tx.pos_entry_mode == types::PosEntryMode::MANUAL || tx.pos_entry_mode == types::PosEntryMode::FALLBACK_MANUAL){
		if (tx.pan) {
			message.SetPan(*tx.pan);
		}
    }

    // DE 03 PROCESSING CODE
    tx.processing_code = message.SetProcessingCode(tx.transaction_type, false);

    // DE 04 AMOUNT
    auto total_amount = tx.GetTotalAmount();
    if (total_amount) {
    	message.SetAmount(*total_amount);
    }

    // DE 11 STAN
    message.SetStan(tx.stan);

    // DE 14 EXPIRATION DATE
    if(tx.pos_entry_mode == types::PosEntryMode::MANUAL || tx.pos_entry_mode == types::PosEntryMode::FALLBACK_MANUAL){
    	if(!tx.expiration_date.empty()){
    		message.SetExpirationDate(tx.expiration_date);
    	}
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

    logger::debug(iso8583::Print(message.GetApdu()).c_str());

    return message.GetApdu();
}

bool ReadRefundResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
	RefundResponse response(data.data(), data.size());

    logger::debug(iso8583::Print(response.GetApdu()).c_str());

    if (!response.IsValid() ||
       (response.GetProcessingCode() != tx.processing_code) ||
       (response.GetStan() != tx.stan) ||
       (response.GetNii() != tx.nii) ||
       (response.GetTid() != tx.tid))
    	return false;

    tx.tx_datetime = response.GetHostDatetime();  //DE-12/ DE-13
    tx.rrn = response.GetRrn();                           //DE-37

    auto auth_id_response = response.GetAuthIdResponse();   //DE-38
    if (auth_id_response)
    	tx.auth_id_response = *auth_id_response;

    tx.response_code = response.GetResponseCode();       //DE-39

    return true;
}

/**************************************
 * REFUND REQUEST
 **************************************/
RefundRequest::RefundRequest(): apdu_(GetProtocolSpec()) {
	apdu_.SetMti(200);
}

void RefundRequest::SetPan(const types::Pan& pan) {
    apdu_.SetField(kFieldPan, pan.ToString());
}

std::string RefundRequest::SetProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
    std::string kSaleProcessingCode = GetDinersProcessingCode(trans_type, is_void_txn);
    apdu_.SetField(kFieldProcessingCode, kSaleProcessingCode);
    return kSaleProcessingCode;
}

void RefundRequest::SetAmount(const types::Amount& amount) {
    apdu_.SetField(kFieldAmount, amount.GetValue());
}

void RefundRequest::SetStan(uint32_t stan) {
    apdu_.SetField(kFieldStan, stan);
}

void RefundRequest::SetExpirationDate(const std::string& expiration_date) {
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

std::string RefundRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
    std::string PoseEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, PoseEntryMode);
    return PoseEntryMode;
}

void RefundRequest::SetPanSequenceNumber(const unsigned int pan_sequence){
	apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void RefundRequest::SetNii(uint32_t nii) {
    apdu_.SetField(kFieldNii, nii);
}

void RefundRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {
    apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void RefundRequest::SetTrack2(const std::vector<uint8_t>& track2) {
    apdu_.SetField(kFieldTrack2Data, track2);
}

void RefundRequest::SetTid(const std::string& tid) {
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void RefundRequest::SetMid(const std::string& mid) {
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

iso8583::Apdu RefundRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * REFUND RESPONSE
 **************************************/
RefundResponse::RefundResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool RefundResponse::IsValid() const {
    bool output = true;
    if (!apdu_.HasMti()) {
    	return false;
    }
    int mti = apdu_.GetMti();
    if (mti != 210) {
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

iso8583::Apdu RefundResponse::GetApdu() const {
	return apdu_;
}

std::string RefundResponse::GetProcessingCode() const {
    return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t RefundResponse::GetStan() const {
    return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t RefundResponse::GetHostDatetime() const {
    std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
    std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
    return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t RefundResponse::GetNii() const {
    return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string RefundResponse::GetRrn() const {
    return apdu_.GetFieldAsString(kFieldRrn);
}

stdx::optional<std::string> RefundResponse::GetAuthIdResponse() const {
	if (apdu_.HasField(kFieldAuthorizationId)) {
		return apdu_.GetFieldAsString(kFieldAuthorizationId);
    }
    return stdx::nullopt;
}

std::string RefundResponse::GetResponseCode() const {
    return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string RefundResponse::GetTid() const {
    return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
