/*
 ------------------------------------------------------------------------------
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
#ifndef DINERS__ECHO_TEST_MESSAGE_H_
#define DINERS__ECHO_TEST_MESSAGE_H_

#include <diners/diners_transaction.h>
#include <diners/test_transaction.h>
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

class EchoTestRequest {

 public:
  EchoTestRequest();

  void SetProcessingCode(const std::string processing_code);
  void SetNii(uint32_t nii);
  void SetTid(const std::string tid);
  void SetMid(const std::string mid);

  iso8583::Apdu GetApdu() const;

 private:
  iso8583::Apdu apdu_;

};

class EchoTestResponse {
 public:
  EchoTestResponse(const uint8_t *data, size_t size);

  bool IsValid() const;

  iso8583::Apdu GetApdu() const;

  std::string GetProcessingCode() const;
  time_t GetHostDatetime() const;
  uint32_t GetNii() const;
  std::string GetTid() const;

 private:
  iso8583::Apdu apdu_;
};

iso8583::Apdu BuildEchoTestRequest(TestTransaction& tx);
bool ReadEchoTestResponse(const std::vector<uint8_t>& data, TestTransaction& tx);

}

#endif


