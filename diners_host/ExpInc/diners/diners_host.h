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
#ifndef DINERS__DINERS_HOST_H_
#define DINERS__DINERS_HOST_H_

#include <comms/client.h>
#include <diners/diners_transaction.h>
#include <diners/test_transaction.h>
#include <iso8583/apdu.h>

namespace diners {

template<typename T>
using BuildRequestFunc = iso8583::Apdu (*)(T& tx);

template<typename T>
using ReadAndValidateResponseFunc = bool (*)(const std::vector<uint8_t>& data, T& tx);

class DinersHost {
 public:
  enum Status {
    COMPLETED,
    TRANSIENT_FAILURE,
    PERM_FAILURE
  };

 public:
  DinersHost()
      : comms_(""){
  }

  ~DinersHost() {
  }

  bool PreConnect(const std::string& host_name);
  //Status PerformKeyExchange(KeyExchange& key_exchange);
  Status AuthorizeSale(DinersTransaction& tx);

  Status PerformTcUpload(DinersTransaction& tx);

  Status ReturnHostError(DinersTransaction& tx){
	  return Status::PERM_FAILURE;
  }
  Status PerformVoid(DinersTransaction& tx);

  Status PerformTMKDownload(DinersTransaction& tx);

  Status PerformBatchUpload(DinersTransaction& tx,
                            unsigned int batch_upload_stan);

  Status PerformSettlement(DinersSettlementData& settle_msg,
                           bool after_batch_upload);

  Status SendReversal(DinersTransaction& tx);

  Status AuthorizeRefund(DinersTransaction& tx);

  Status SendOfflineSale(DinersTransaction& tx);

  Status PerformSaleCompletion(DinersTransaction& tx);

  Status PerformDinersTestTransaction(TestTransaction& tx);

  Status AuthorizePreAuth(DinersTransaction& tx);

  Status SendTipAdjust(DinersTransaction& tx);

  bool WaitForConnection(uint32_t timeout);
  bool Disconnect();

 private:
  comms::Client comms_;

  Status SendMessage(const std::vector<std::uint8_t>& msg, const std::string tpdu);
  Status ReceiveMessage(std::vector<std::uint8_t>& msg);

  template<typename T>
  DinersHost::Status PerformOnline(BuildRequestFunc<T> request_func,
                                 ReadAndValidateResponseFunc<T> response_func,
                                 T& tx);

  template<typename T>
  DinersHost::Status PerformOnline(iso8583::Apdu request,
                                 ReadAndValidateResponseFunc<T> response_func,
                                 T& tx);
};

}

#endif
