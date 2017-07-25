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
#include "void_message.h"
#include <utils/converter.h>
#include <iso8583/encoder.h>
#include <iso8583/printer.h>
#include <stdx/ctime>
#include <utils/logger.h>
#include "protocol.h"
#include "diners_utils.h"

namespace diners {

iso8583::Apdu BuildVoidRequest(DinersTransaction& tx) {
	VoidRequest message;

	// DE 02 PAN
    message.SetPan(*tx.pan);

    // DE 03 PROCESSING CODE
    tx.processing_code = message.SetProcessingCode(tx.transaction_type,true);

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

    // DE 41 TID
    message.SetTid(tx.tid);

    // DE 42 MID
    message.SetMid(tx.mid);

    // DE 52 PIN BLOCK
    if (tx.pin_block) {
    	message.SetPinBlock(*tx.pin_block);
    }

    // DE 54 TIP AMOUNT
    if(tx.previous_transaction_status == DinersTransactionStatus::APPROVED && tx.is_adjusted){
    	message.SetAdditionalAmount(*tx.additional_amount);
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

bool ReadVoidResponse(const std::vector<uint8_t>& data, DinersTransaction& tx) {
  VoidResponse response(data.data(), data.size());
  logger::debug(iso8583::Print(response.GetApdu()).c_str());
  if (!response.IsValid() ||
      (response.GetProcessingCode() != tx.processing_code) ||
      (response.GetStan() != tx.stan) ||
	  //(response.GetHostDatetime() != tx.tx_datetime)|| //To confirm whether date & time must be the same
      (response.GetNii() != tx.nii) ||
	  //(response.GetRrn() != tx.rrn)|| //To confirm whether rrn request = rrn response
      (response.GetTid() != tx.tid))
    return false;

    tx.tx_datetime = response.GetHostDatetime();  //DE-12/ DE-13
    tx.rrn = response.GetRrn();
    tx.response_code = response.GetResponseCode();       //DE-39

    auto auth_id_response = response.GetAuthIdResponse();   //DE-38
    if (auth_id_response)
    	tx.auth_id_response = *auth_id_response;

    return true;
}

/**************************************
 * VOID REQUEST
 **************************************/
VoidRequest::VoidRequest(): apdu_(GetProtocolSpec()) {
	apdu_.SetMti(200);
}

void VoidRequest::SetPan(const types::Pan& pan) {                           //DE 02
    apdu_.SetField(kFieldPan, pan.ToString());
}

std::string VoidRequest::SetProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
    std::string kVoidProcessingCode = GetDinersProcessingCode(trans_type, is_void_txn);
    apdu_.SetField(kFieldProcessingCode, kVoidProcessingCode);
    return kVoidProcessingCode;//DE 03
}

void VoidRequest::SetAmount(const types::Amount& amount) {                                             //DE 04
    apdu_.SetField(kFieldAmount, amount.GetValue());
}

void VoidRequest::SetStan(uint32_t stan) {                                  //DE 05
    apdu_.SetField(kFieldStan, stan);
}

void VoidRequest::SetExpirationDate(const std::string& expiration_date) {   //DE 14
    apdu_.SetField(kFieldDateExpiration, expiration_date);
}

void VoidRequest::SetHostDatetime(time_t time_stamp) {
    apdu_.SetField(kFieldTimeLocalTransaction, iso8583::IsoTimeFromTimestamp(time_stamp));
    apdu_.SetField(kFieldDateLocalTransaction, iso8583::IsoDateFromTimestamp(time_stamp));
}

void VoidRequest::SetPosEntryMode(types::PosEntryMode & pos_entry_mode) {   //DE 22
    //TODO: USE ORIGINAL TRANSACTIONS POS ENTRY MODE DATA?
    std::string PoseEntryMode = GetPosEntryMode(pos_entry_mode);
    apdu_.SetField(kFieldPosEntryMode, PoseEntryMode);
}

void VoidRequest::SetPanSequenceNumber(const unsigned int pan_sequence) {
    apdu_.SetField(kFieldPanSequenceNumber, pan_sequence);
}

void VoidRequest::SetNii(uint32_t nii) {                                    //DE 24
    apdu_.SetField(kFieldNii, nii);
}

void VoidRequest::SetPosConditionCode(types::PosConditionCode & pos_condition_code) {  //DE 25
    apdu_.SetField(kFieldPosConditionCode,GetDinersConditionCode(pos_condition_code));
}

void VoidRequest::SetRrn(const std::string& retrival_response_code){        //DE 37
    apdu_.SetField(kFieldRrn, retrival_response_code);
}

void VoidRequest::SetTrack2(const std::vector<uint8_t>& track2) {
    apdu_.SetField(kFieldTrack2Data, track2);
}

void VoidRequest::SetTid(const std::string& tid) {                          //DE 41
    apdu_.SetField(kFieldCardAcceptorTerminalId, tid);
}

void VoidRequest::SetMid(const std::string& mid) {                          //DE 42
    apdu_.SetField(kFieldCardAcceptorId, mid);
}

void VoidRequest::SetPinBlock(const std::vector<uint8_t>& pin_block) {
	apdu_.SetField(kFieldPinBlock, pin_block);
}

void VoidRequest::SetAdditionalAmount(const types::Amount& tip_amount){
	auto tip_amount_str = utils::ToString(tip_amount.GetValue());
	auto value = iso8583::RightAligned(tip_amount_str, 12, '0');
	apdu_.SetField(kFieldAdditionalAmount, value);
}

void VoidRequest::SetOriginalAmount(const types::Amount& original_amount){
	auto original_amount_str = utils::ToString(original_amount.GetValue());
	auto value = iso8583::RightAligned(original_amount_str, 12, '0');
	apdu_.SetField(kField60, value);
}

void VoidRequest::SetInvoiceNumber(uint32_t invoice) {                      //DE 62
    auto invoice_str = utils::ToString(invoice);
    std::string value = iso8583::RightAligned(invoice_str, 6, '0');
    apdu_.SetField(kField62, value);
}

iso8583::Apdu VoidRequest::GetApdu() const {
    return apdu_;
}

/**************************************
 * VOID RESPONSE
 **************************************/
VoidResponse::VoidResponse(const uint8_t *data, size_t size)
    : apdu_(GetProtocolSpec(), data, size) {
}

bool VoidResponse::IsValid() const {
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

iso8583::Apdu VoidResponse::GetApdu() const {
	return apdu_;
}

std::string VoidResponse::GetProcessingCode() const {
    return apdu_.GetFieldAsString(kFieldProcessingCode);
}

uint32_t VoidResponse::GetStan() const {
    return apdu_.GetFieldAsInteger(kFieldStan);
}

time_t VoidResponse::GetHostDatetime() const {
    std::string date = apdu_.GetFieldAsString(kFieldDateLocalTransaction);
    std::string time = apdu_.GetFieldAsString(kFieldTimeLocalTransaction);
    return utils::GetDatetimefromIso8583Format(date, time);
}

uint32_t VoidResponse::GetNii() const {
    return apdu_.GetFieldAsInteger(kFieldNii);
}

std::string VoidResponse::GetRrn() const {
    return apdu_.GetFieldAsString(kFieldRrn);
}

stdx::optional<std::string> VoidResponse::GetAuthIdResponse() const {
    if (apdu_.HasField(kFieldAuthorizationId)) {
    	return apdu_.GetFieldAsString(kFieldAuthorizationId);
    }
    return stdx::nullopt;
}

std::string VoidResponse::GetResponseCode() const {
    return apdu_.GetFieldAsString(kFieldResponseCode);
}

std::string VoidResponse::GetTid() const {
    return apdu_.GetFieldAsString(kFieldCardAcceptorTerminalId);
}

}
