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
#ifndef DINERS__SALE_COMPLETION_MESSAGE_H_
#define DINERS__SALE_COMPLETION_MESSAGE_H_

#include <vector>
#include <cstdint>
#include <memory>
#include <ctime>

#include <stdx/optional>
#include <iso8583/apdu.h>

#include <types/pan.h>
#include <types/amount.h>
#include <types/pos_entry_mode.h>
#include <types/pos_condition_code.h>
#include <emv/track2_equivalent_data.h>

#include "diners/diners_transaction.h"

namespace diners {

class SaleCompletionRequest {
 public:
  SaleCompletionRequest();

  void SetPan(const types::Pan& pan);
  std::string SetProcessingCode(DinersTransactionType & trans_type,bool is_void_txn);
  void SetAmount(const types::Amount& amount);
  void SetStan(std::uint32_t stan);
  void SetExpirationDate(const std::string& expiration_date);
  std::string SetPosEntryMode(types::PosEntryMode & pos_entry_mode);
  void SetPanSequenceNumber(const unsigned int pan_sequence);
  void SetNii(std::uint32_t nii);
  void SetPosConditionCode(types::PosConditionCode & pos_condition_code);
  void SetRrn(const std::string& retrival_response_code);
  void SetAuthorizationCode(const std::string& authorization_code);
  void SetResponseCode(const std::string& response_code);
  void SetTid(const std::string& tid);
  void SetMid(const std::string& mid);
  void SetBatchNumber(std::uint32_t batch_number);
  void SetInvoiceNumber(std::uint32_t invoice);  // field 62
  void SetHostDatetime(time_t time_stamp);

  iso8583::Apdu GetApdu() const;

 private:
  iso8583::Apdu apdu_;
};

class SaleCompletionResponse {
 public:
  SaleCompletionResponse(const std::uint8_t* data, size_t size);

  bool IsValid() const;

  iso8583::Apdu GetApdu() const;

  std::string GetProcessingCode() const;
  std::uint32_t GetStan() const;
  time_t GetHostDatetime() const;
  std::uint32_t GetNii() const;
  std::string GetRrn() const;
  stdx::optional<std::string> GetAuthIdResponse() const;
  std::string GetResponseCode() const;
  std::string GetTid() const;

 private:
  iso8583::Apdu apdu_;
};

iso8583::Apdu BuildSaleCompletionRequest(DinersTransaction& tx);
bool ReadSaleCompletionResponse(const utils::bytes& data, DinersTransaction& tx);

}

#endif
