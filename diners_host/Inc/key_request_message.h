/*
 ------------------------------------------------------------------------------
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
#ifndef DINERS__KEY_DOWNLOAD_MESSAGE_H_
#define DINERS__KEY_DOWNLOAD_MESSAGE_H_

#include <diners/diners_transaction.h>
#include <vector>
#include <cstdint>
#include <memory>
#include <ctime>

#include <stdx/optional>
#include <iso8583/apdu.h>

namespace diners {

class KeyDownloadRequest {

 public:
  KeyDownloadRequest();

  std::string SetProcessingCodeForKeyDownload();
  void SetNii(uint32_t nii);
  void SetTid(const std::string& tid);
  void SetMid(const std::string& mid);

  iso8583::Apdu GetApdu() const;

 private:
  iso8583::Apdu apdu_;

};

class KeyDownloadResponse {
 public:
  KeyDownloadResponse(const uint8_t *data, size_t size);

  bool IsValid() const;

  iso8583::Apdu GetApdu() const;

  std::string GetProcessingCode() const;
  time_t GetHostDatetime() const;
  uint32_t GetNii() const;
  std::string GetResponseCode() const;
  std::string GetTid() const;
  std::vector<uint8_t> GetEncryptedTMK() const;


 private:
  iso8583::Apdu apdu_;
};

iso8583::Apdu BuildKeyDownloadRequest(DinersTransaction& tx);
bool ReadKeyDownloadResponse(const std::vector<uint8_t>& data, DinersTransaction& tx);

}

#endif



