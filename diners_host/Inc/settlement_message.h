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
#ifndef DINERS__SETTLEMENT_MESSAGE_H_
#define DINERS__SETTLEMENT_MESSAGE_H_

#include <diners/diners_transaction.h>
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



namespace diners {

class SettlementRequest {
 public:
  SettlementRequest();

  std::string SetProcessingCode(bool after_batch_upload);
  void SetStan(uint32_t stan);
  void SetNii(uint32_t nii);
  void SetTid(const std::string& tid);
  void SetMid(const std::string& mid);
  void SetHostDatetime(time_t time_stamp);
  void SetBatchNumber(uint32_t batch_number); //field 60
  void SetBatchTotal(BatchTotalsForDinersHost & Diners_batch_totals); //field 63 reconciliation totals //TODO: compute for batch totals

  iso8583::Apdu GetApdu() const;

 private:
  iso8583::Apdu apdu_;
};

class SettlementResponse {
 public:
  SettlementResponse(const uint8_t *data, size_t size);

  bool IsValid() const;

  iso8583::Apdu GetApdu() const;

  std::string GetProcessingCode() const;
  uint32_t GetStan() const;
  time_t GetHostDatetime() const;
  uint32_t GetNii() const;
  std::string GetRrn() const;
  std::string GetResponseCode() const;
  std::string GetTid() const;

 private:
  iso8583::Apdu apdu_;
};

iso8583::Apdu BuildSettlementRequest(diners::DinersSettlementData & settle_msg,bool after_batch_upload);
bool ReadSettlementResponse(const std::vector<uint8_t>& data, diners::DinersSettlementData & settle_msg);

}

#endif
