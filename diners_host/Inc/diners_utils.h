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
#ifndef DINERS__UTILS_MESSAGE_H_
#define DINERS__UTILS_MESSAGE_H_

#include <diners/diners_transaction.h>
#include <types/pos_entry_mode.h>

namespace diners {

std::string GetDinersProcessingCode(DinersTransactionType & trans_type, bool is_void);
std::string GetPosEntryMode(types::PosEntryMode & pos_entry_mode);
std::string GetDinersConditionCode(types::PosConditionCode & pos_condition_code);
types::Amount GetAmountRequired(DinersTransaction & tx);

}
#endif
