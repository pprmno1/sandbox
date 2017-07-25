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
#include "tip_adjust_message.h"
#include <utils/converter.h>
#include <stdx/ctime>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

using namespace types;
namespace diners {

iso8583::Apdu BuildTipAdjustRequest(DinersTransaction& tx) {
	TipAdjustRequest message;

    // DE 02 PAN
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
    message.SetHostDatetime(stdx::time(nullptr));

    // DE 14 EXPIRATION DATE
    message.SetExpirationDate(tx.expiration_date);

    // DE 22 POS ENTRY MODE
    message.SetPosEntryMode(tx.pos_entry_mode);

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

    // DE 38 Auth Code
    message.SetAuthorizationCode(tx.auth_id_response);

    // DE 39 Response Code
    message.SetResponseCode(tx.response_code);

    // DE 41 TID
    message.SetTid(tx.tid);

    // DE 42 MID
    message.SetMid(tx.mid);

    // DE 54 TIP AMOUNT
    auto add_amount = tx.additional_amount;
    if (add_amount) {
    	message.SetAdditionalAmount(*add_amount);
    }

    // DE 60 ORIGINAL AMOUNT
    if(tx.amount){
    	message.SetOriginalAmount(*tx.GetTotalOriginalAmount());
    }

    // DE 62 INVOICE NUMBER
    message.SetInvoiceNumber(tx.invoice_number);

    logger::debug(iso8583::Print(message.GetApdu()).c_str());

    return message.GetApdu();
}

bool ReadTipAdjustResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
	TipAdjustResponse response(data.data(), data.size());

    logger::debug(iso8583::Print(response.GetApdu()).c_str());

    if (!response.IsValid() ||
    		(response.GetProcessingCode()!=tx.processing_code)||
            (response.GetStan() != tx.stan) ||
            (response.GetNii() != tx.nii) ||
	        //(response.GetRrn() != tx.rrn)||
            (response.GetTid() != tx.tid))
    	return false;

    tx.response_code = response.GetResponseCode();       //DE-39
    tx.rrn = response.GetRrn();

    auto original_amount = response.GetOriginalAmount(); //DE-60
    if(original_amount){
    	types::Amount amount("SGD", original_amount);
    	tx.amount = amount;
    }

    return true;
}

/**************************************
 * TIP ADJUST REQUEST
 **************************************/
TipAdjustRequest::TipAdjustRequest(): apdu_(GetProtocolSpec()) {
	apdu_.SetMti(220);
}

void TipAdjustRequest::SetPan(const types::Pan& pan) {
	apdu_.SetField(kFieldPan, pan.ToString());
}

std::string TipAdjustRequest::SetProcessingCode() {
	std::string kSaleProcessingCode = "020000";
    apdu_.SetField(kFieldProcessingCode, kSaleProcessingCode);
    return kSaleProcessingCode;
}

void TipAdjustRequest::SetAmount(const types::Amount& amount) {
    apdu_.SetField(kFieldAmount, amount.GetValue());
}

void TipAdjustRequest::SetStan(uint32_t stan) {
	apdu_.SetField(kFieldStan, stan);
}

void TipAdjustRequest::SetExpirationDate(const std::string& expiration_date) {
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

void TipAdjustRequest::SetHostDatetime(time_t time_stamp) {
    apdu_.SetField(kFieldTimeLocalTransaction, iso8583::IsoTimeFromTimestamp(time_stamp));
    apdu_.SetField(kFieldDateLocalTransaction, iso8583::IsoDateFromTimestamp(time_stamp));
}

void TipAdjustRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
	std::string PosEntryMode = GetPosEntryMode(pos_entry_mode);
	apdu_.SetField(kFieldPosEntryMode, PosEntryMode);
}

void TipAdjustRequest::SetPanSequenceNumber(const unsigned int pan_sequence) {
    apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void TipAdjustRequest::SetNii(uint32_t nii) {
    apdu_.SetField(kFieldNii, nii);
}

void TipAdjustRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {
    apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void TipAdjustRequest::SetRrn(const std::string& rrn) {
    apdu_.SetField(kFieldRrn, rrn);
}

void TipAdjustRequest::SetAuthorizationCode(const std::string& authorization_code) {
    apdu_.SetField(kFieldAuthorizationId, authorization_code);
}

void TipAdjustRequest::SetResponseCode(const std::string& response_code) {
    apdu_.SetField(kFieldResponseCode, response_code);
}

void TipAdjustRequest::SetTid(const std::string& tid) {
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void TipAdjustRequest::SetMid(const std::string& mid) {
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

void TipAdjustRequest::SetAdditionalAmount(const types::Amount& tip_amount){
	auto tip_amount_str = utils::ToString(tip_amount.GetValue());
	auto value = iso8583::RightAligned(tip_amount_str, 12, '0');
	apdu_.SetField(kFieldAdditionalAmount, value);
}

void TipAdjustRequest::SetOriginalAmount(const types::Amount& original_amount){
	auto original_amount_str = utils::ToString(original_amount.GetValue());
	auto value = iso8583::RightAligned(original_amount_str, 12, '0');
	apdu_.SetField(kField60, value);
}

void TipAdjustRequest::SetInvoiceNumber(uint32_t invoice) {  // field 62
    auto invoice_str = utils::ToString(invoice);
    std::string value = iso8583::RightAligned(invoice_str, 6, '0');
    apdu_.SetField(kField62, value);
}

iso8583::Apdu TipAdjustRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * TIP ADJUST RESPONSE
 **************************************/
TipAdjustResponse::TipAdjustResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool TipAdjustResponse::IsValid() const {
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

iso8583::Apdu TipAdjustResponse::GetApdu() const {
	return apdu_;
}

std::string TipAdjustResponse::GetProcessingCode() const {
	return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t TipAdjustResponse::GetStan() const {
	return apdu_.GetFieldAsInteger(kFieldStan);
}

uint32_t TipAdjustResponse::GetNii() const {
	return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string TipAdjustResponse::GetRrn() const {
	return apdu_.GetFieldAsString(kFieldRrn);
}

std::string TipAdjustResponse::GetResponseCode() const {
	return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string TipAdjustResponse::GetTid() const {
	return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

int64_t TipAdjustResponse::GetOriginalAmount() const{
	return apdu_.GetFieldAsInteger(kField60);
}

}
