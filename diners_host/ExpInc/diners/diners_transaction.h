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
#ifndef DINERS__DINERS_TRANSACTION_H_
#define DINERS__DINERS_TRANSACTION_H_

#include <stdx/optional>
#include <types/amount.h>
#include <types/pan.h>
#include <types/aid.h>
#include <types/pos_entry_mode.h>
#include <types/pos_condition_code.h>

namespace diners {

enum DinersTransactionType {
  AUTHORIZATION,
  SALE,
  OFFLINE_SALE,
  REFUND,
  PREAUTH,
  SALE_COMPLETION,
  TC_UPLOAD
};

enum DinersInProgressStatus {
  IN_PROGRESS_NONE,
  IN_PROGRESS_VOID,
  IN_PROGRESS_SALE_COMPLETION
};

enum DinersTransactionStatus {
	IN_PROGRESS,
	APPROVED,
	DECLINED,
	NOT_ALLOWED,
	CANCELLED,
	CARD_BLOCKED,
	CARD_REMOVED_FROM_READER,
	CARD_ERROR,
	TERMINAL_ERROR,
	REVERSED,
	TO_REVERSE,
	TO_ADVISE,
	UNKNOWN_ERROR
};

struct DinersTransaction {
  DinersTransaction()
  : stan(0),
    nii(0),
    invoice_number(0),
    tx_datetime(0) {
  }

  stdx::optional<types::Pan> pan;
  std::string processing_code;
  std::string orig_pos_entry_mode; //TODO: find a way to also save this to batch
  stdx::optional<types::Amount> amount;
  stdx::optional<types::Amount> preauth_amount;  //amount, pre-authorization

  bool is_preauth_completed;
  bool is_adjusted;

  unsigned int stan;
  std::string expiration_date;
  types::PosEntryMode pos_entry_mode;
  stdx::optional<unsigned int> pan_sequence_number;
  unsigned int nii;
  types::PosConditionCode pos_condition_code;
  stdx::optional<std::vector<uint8_t>> track2;
  std::string tid;
  std::string mid;
  std::string cvv;
  stdx::optional<std::vector<uint8_t>> pin_block;
  stdx::optional<types::Amount> additional_amount;
  stdx::optional<types::Amount> original_additional_amount;
  stdx::optional<std::vector<uint8_t>> icc_data;
  stdx::optional<types::Aid> aid;
  DinersTransactionType transaction_type;
  DinersTransactionStatus transaction_status;
  DinersInProgressStatus in_progress_status;
  DinersTransactionStatus previous_transaction_status;
  std::string tpdu;
  unsigned int invoice_number;
  unsigned int batch_number;

  time_t tx_datetime;
  std::string rrn;
  std::string auth_id_response;
  std::string response_code;
  std::vector<uint8_t> issuer_emv_response;
  std::string track1_data;
  std::string cardholder_name;

  stdx::optional<types::Amount> GetTotalAmount() const;
  stdx::optional<types::Amount> GetTotalPreauthAmount() const;
  stdx::optional<types::Amount> GetTotalOriginalAmount() const;
};


struct BatchTotal {
  unsigned int count = 0;
  std::uint64_t total = 0;

  BatchTotal(unsigned int count_, std::uint64_t total_)
      : count(count_),
        total(total_) {
  }

  BatchTotal()
      : count(0),
        total(0) {
  }

  BatchTotal& operator+=(const BatchTotal& rhs) {
    count += rhs.count;
    total += rhs.total;
    return *this;
  }

};

struct BatchTotalsForDinersHost {
  types::Currency currency;
  BatchTotal sales_total;
  BatchTotal refunds_total;
};

struct DinersSettlementData {
  std::string processing_code;
  uint32_t stan;
  time_t tx_datetime;
  std::string tpdu;
  uint32_t nii;
  std::string tid;
  std::string mid;
  std::string rrn;
  std::string response_code;
  uint32_t batch_number;
  BatchTotalsForDinersHost batch_summary;
  std::string response_msg;
  uint32_t invoice_num;
};

struct TMKDownloadMessage {
  std::string processing_code;
  uint32_t stan;
  time_t tx_datetime;
  std::string tpdu;
  uint32_t nii;
  std::string tid;
  std::string mid;
  std::string response_code;
  std::string field62;
  std::string field63;
  std::vector<uint8_t> tmk;
  std::vector<uint8_t> tle_key;
  std::vector<uint8_t> tid_location;
};

}
;

#endif
