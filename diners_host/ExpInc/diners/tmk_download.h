/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2015-2016 INGENICO S.A.
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
#ifndef DINERS__TMK_DOWNLOAD_H_
#define DINERS__TMK_DOWNLOAD_H_

#include <string>
#include <vector>
#include <ctime>

namespace diners {

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

#endif
