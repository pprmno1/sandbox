/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2016 INGENICO S.A.
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

namespace diners {

stdx::optional<types::Amount> DinersTransaction::GetTotalAmount() const {
	stdx::optional<types::Amount> total_amount;
	if (amount.has_value())
		total_amount = GetTotalOriginalAmount();
	else if(preauth_amount.has_value())
		total_amount = preauth_amount;
	else
		return stdx::nullopt;

    if (additional_amount.has_value())
    	total_amount = *total_amount + *additional_amount;

    return total_amount;
}

stdx::optional<types::Amount> DinersTransaction::GetTotalPreauthAmount() const {
	if (!preauth_amount.has_value())
		return stdx::nullopt;

    stdx::optional<types::Amount> total_amount = preauth_amount;

    if (additional_amount.has_value())
    	total_amount = *total_amount + *additional_amount;

    return total_amount;
}

stdx::optional<types::Amount> DinersTransaction::GetTotalOriginalAmount() const {
	if (!amount.has_value())
		return stdx::nullopt;

	stdx::optional<types::Amount> total_amount = amount;

	if (original_additional_amount.has_value())
		total_amount = *total_amount + *original_additional_amount;
    return total_amount;
}

}
