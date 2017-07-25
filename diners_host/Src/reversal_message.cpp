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
#include "reversal_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildReversalRequest(DinersTransaction& tx) {
	ReversalRequest message;

    // DE 02 PAN
    if (tx.pan){
    	message.SetPan(*tx.pan);
    }

    // DE 03 PROCESSING CODE
    if(tx.in_progress_status == DinersInProgressStatus::IN_PROGRESS_VOID)
    	tx.processing_code = message.SetProcessingCode(tx.transaction_type, true);
    else
    	tx.processing_code = message.SetProcessingCode(tx.transaction_type, false);

    // DE 04 AMOUNT
      if(tx.transaction_type == DinersTransactionType::PREAUTH || tx.transaction_type == DinersTransactionType::AUTHORIZATION){
          if (tx.preauth_amount)
          	message.SetAmount(*tx.GetTotalPreauthAmount());
      }
      else {
          if (tx.amount) {
          	if(tx.previous_transaction_status == DinersTransactionStatus::APPROVED)
          		message.SetAmount(*tx.GetTotalAmount());
          	else if(tx.previous_transaction_status == DinersTransactionStatus::TO_ADVISE)
          		message.SetAmount(*tx.GetTotalOriginalAmount());
          }
      }

    // DE 11 STAN
    message.SetStan(tx.stan);

    // DE 12 & 13 DATETIME
    message.SetDatetime(tx.tx_datetime);

    // DE 14 EXPIRATION DATE
    if(!tx.expiration_date.empty()){
    	message.SetExpirationDate(tx.expiration_date);
    }

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
    if(!tx.rrn.empty()){
    	 message.SetRrn(tx.rrn);
    }

    // DE 41 TID
    message.SetTid(tx.tid);

    // DE 42 MID
    message.SetMid(tx.mid);

    // DE 52 PIN BLOCK
    if (tx.pin_block){
    	message.SetPinBlock(*tx.pin_block);
    }

    // DE 54 TIP AMOUNT
    if(tx.previous_transaction_status == DinersTransactionStatus::APPROVED && tx.is_adjusted){
    	message.SetAdditionalAmount(*tx.additional_amount);
    }

    // DE 55 ICC DATA
    if (!tx.icc_data->empty()) {
    	message.SetEmvData(*tx.icc_data);
    }

    // DE 60 ORIGINAL AMOUNT
    if(tx.previous_transaction_status == DinersTransactionStatus::APPROVED && tx.is_adjusted){
    	if(tx.additional_amount)
    		message.SetOriginalAmount(*tx.GetTotalOriginalAmount());
    }

    // DE 62 INVOICE NUMBER
    message.SetInvoiceNumber(tx.invoice_number);

    logger::debug(iso8583::Print(message.GetApdu()).c_str());

    return message.GetApdu();
}

bool ReadReversalResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
	ReversalResponse response(data.data(), data.size());

    logger::debug(iso8583::Print(response.GetApdu()).c_str());

    if (!response.IsValid() ||
    		(response.GetProcessingCode() != tx.processing_code) ||
            (response.GetStan() != tx.stan) ||
            (response.GetNii() != tx.nii) ||
	        //(response.GetRrn() != tx.rrn) ||
            (response.GetTid() != tx.tid))
    	return false;

    tx.tx_datetime = response.GetHostDatetime();  //DE-12/ DE-13

    auto auth_id_response = response.GetAuthIdResponse();   //DE-38
    if (auth_id_response)
    	tx.auth_id_response = *auth_id_response;

    tx.response_code = response.GetResponseCode();       //DE-39
    tx.rrn = response.GetRrn();

    return true;
}

/**************************************
 * REVERSAL REQUEST
 **************************************/
ReversalRequest::ReversalRequest() : apdu_(GetProtocolSpec()) {
	apdu_.SetMti(400);
}

void ReversalRequest::SetPan(const types::Pan& pan) {
	apdu_.SetField(kFieldPan, pan.ToString());
}

std::string ReversalRequest::SetProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
	std::string kReversalProcessingCode = GetDinersProcessingCode(trans_type, is_void_txn);
    apdu_.SetField(kFieldProcessingCode, kReversalProcessingCode);
    return kReversalProcessingCode;
}

void ReversalRequest::SetAmount(const types::Amount& amount) {
	apdu_.SetField(kFieldAmount, amount.GetValue());
}

void ReversalRequest::SetStan(uint32_t stan) {
    apdu_.SetField(kFieldStan, stan);
}

void ReversalRequest::SetDatetime(time_t & time_stamp) {
    apdu_.SetField(kFieldTimeLocalTransaction, iso8583::IsoTimeFromTimestamp(time_stamp));
    apdu_.SetField(kFieldDateLocalTransaction, iso8583::IsoDateFromTimestamp(time_stamp));
}

void ReversalRequest::SetExpirationDate(const std::string& expiration_date) {
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

std::string ReversalRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
    std::string PoseEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, PoseEntryMode);
    return PoseEntryMode;
}

void ReversalRequest::SetPanSequenceNumber(const unsigned int pan_sequence) {
    apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void ReversalRequest::SetNii(uint32_t nii) {
    apdu_.SetField(kFieldNii, nii);
}

void ReversalRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {
    apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void ReversalRequest::SetTrack2(const std::vector<uint8_t>& track2) {
    apdu_.SetField(kFieldTrack2Data, track2);
}

void ReversalRequest::SetRrn(const std::string& retrival_response_code) {
    apdu_.SetField(kFieldRrn, retrival_response_code);
}

void ReversalRequest::SetTid(const std::string& tid) {
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void ReversalRequest::SetMid(const std::string& mid) {
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

void ReversalRequest::SetPinBlock(const std::vector<uint8_t>& pin_block) {
	apdu_.SetField(kFieldPinBlock, pin_block);
}

void ReversalRequest::SetEmvData(const std::vector<uint8_t>& emv_data) {
    apdu_.SetField(kFieldIccData, emv_data);
}

void ReversalRequest::SetAdditionalAmount(const types::Amount& tip_amount){
	auto tip_amount_str = utils::ToString(tip_amount.GetValue());
	auto value = iso8583::RightAligned(tip_amount_str, 12, '0');
	apdu_.SetField(kFieldAdditionalAmount, value);
}

void ReversalRequest::SetOriginalAmount(const types::Amount& original_amount){
	auto original_amount_str = utils::ToString(original_amount.GetValue());
	auto value = iso8583::RightAligned(original_amount_str, 12, '0');
	apdu_.SetField(kField60, value);
}

void ReversalRequest::SetInvoiceNumber(uint32_t invoice) {  // field 62
    auto invoice_str = utils::ToString(invoice);
    std::string value = iso8583::RightAligned(invoice_str, 6, '0');
    apdu_.SetField(kField62, value);
}

iso8583::Apdu ReversalRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * REVERSAL RESPONSE
 **************************************/
ReversalResponse::ReversalResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool ReversalResponse::IsValid() const {
	bool output = true;

    if (!apdu_.HasMti()) {
    	return false;
    }

    int mti = apdu_.GetMti();
    if (mti != 410) {
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

iso8583::Apdu ReversalResponse::GetApdu() const {
    return apdu_;
}

std::string ReversalResponse::GetProcessingCode() const {
    return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t ReversalResponse::GetStan() const {
    return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t ReversalResponse::GetHostDatetime() const {
    std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
    std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
    return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t ReversalResponse::GetNii() const {
    return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string ReversalResponse::GetRrn() const {
    return apdu_.GetFieldAsString(kFieldRrn);
}

stdx::optional<std::string> ReversalResponse::GetAuthIdResponse() const {
    if (apdu_.HasField(kFieldAuthorizationId)) {
    	return apdu_.GetFieldAsString(kFieldAuthorizationId);
    }
    return stdx::nullopt;
}

std::string ReversalResponse::GetResponseCode() const {
    return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string ReversalResponse::GetTid() const {
    return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
