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
#include "diners_utils.h"

namespace diners {

std::string GetDinersProcessingCode(DinersTransactionType & trans_type, bool is_void_txn) {
	std::string kProcessingCode;
	if (is_void_txn){
		if (trans_type == DinersTransactionType::SALE
		    || trans_type == DinersTransactionType::OFFLINE_SALE
			|| trans_type == DinersTransactionType::SALE_COMPLETION)
			kProcessingCode = "02";
        else if (trans_type == DinersTransactionType::REFUND)
        	kProcessingCode = "22";
		//TODO: check processing code for void of other transactions e.g. pre-authorization
	}

	else{ //sale/refund/preauth/offline/completion
		if (trans_type == DinersTransactionType::AUTHORIZATION ||
		    trans_type == DinersTransactionType::SALE ||
            trans_type == DinersTransactionType::OFFLINE_SALE ||
            trans_type == DinersTransactionType::SALE_COMPLETION)

			kProcessingCode = "00";
		else if (trans_type == DinersTransactionType::REFUND)
			kProcessingCode = "20";
		else if (trans_type == DinersTransactionType::PREAUTH)
			kProcessingCode = "30";
        else if (trans_type == DinersTransactionType::TC_UPLOAD)
        	kProcessingCode = "94";
	}

	kProcessingCode += "0000";  //TODO: Flow Digit Control
	return kProcessingCode;
}

std::string GetPosEntryMode(types::PosEntryMode & pos_entry_mode) {
	std::string kPosEntryMode;
	if (pos_entry_mode == types::PosEntryMode::FALLBACK_MANUAL)
		kPosEntryMode = "8";  // TODO: to check
	else if (pos_entry_mode == types::PosEntryMode::FALLBACK_MAGSTRIPE)
	    kPosEntryMode = "9";  // TODO: to check
    else
    	kPosEntryMode = "0";  // TODO: to check

    switch (pos_entry_mode) {
    case types::PosEntryMode::CHIP:
    	kPosEntryMode += "5";
        break;
    case types::PosEntryMode::MAGSTRIPE:
    case types::PosEntryMode::FALLBACK_MANUAL:
        kPosEntryMode += "2";
        break;
    case types::PosEntryMode::FALLBACK_MAGSTRIPE:
        kPosEntryMode += "0";
        break;
    case types::PosEntryMode::CONTACTLESS:  //TODO: value of position 2 for contactless magstripe
        kPosEntryMode += "9";
        break;
    case types::PosEntryMode::MANUAL:
        kPosEntryMode += "1";
        break;
    default:
        kPosEntryMode += "0";
    }

    if(pos_entry_mode == types::PosEntryMode::CHIP || pos_entry_mode == types::PosEntryMode::CONTACTLESS )
    	kPosEntryMode += "1";
    else
		kPosEntryMode += "2"; // To check with spec
    return kPosEntryMode;
}

std::string GetDinersConditionCode(types::PosConditionCode & pos_condition_code) {
	std::string kConditionCode;
	switch (pos_condition_code){
	case types::PosConditionCode::PREAUTH:
		return kConditionCode = "00"; //TODO: other PREAUTH Types, sale completion ...
	case types::PosConditionCode::NORMAL:
    default:
    	return kConditionCode = "00";
	}
}

types::Amount GetAmountRequired(DinersTransaction & tx){
	types::Amount amount;
    if (tx.pos_condition_code == types::PosConditionCode::NORMAL)
    	amount.SetValue(tx.GetTotalAmount()->GetValue());
    else
    	amount.SetValue(tx.preauth_amount->GetValue());
    return amount;
}

}

