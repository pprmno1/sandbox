/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2015-2017 INGENICO S.A.
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
#include <diners/diners_host.h>
#include <stdx/string>
#include <utils/strings.h>
#include <utils/logger.h>
#include <iso8583/printer.h>

#include "preauth_message.h"
#include "test_transaction_message.h"
#include "key_request_message.h"
#include "sale_message.h"
#include "sale_completion_message.h"
#include "void_message.h"
#include "refund_message.h"
#include "tip_adjust_message.h"
#include "reversal_message.h"
#include "offline_sale_message.h"
#include "settlement_message.h"
#include "batch_upload_message.h"
#include "tc_upload_message.h"

using namespace diners;

namespace {
const size_t kTpduSize = 5;
const size_t kHeaderSize = 4;

std::vector<uint8_t> AddTpdu(const std::vector<uint8_t>& msg,
                             const std::string& tpdu) {

  std::vector<uint8_t> output = utils::HexStringToBytes(tpdu);
  output.insert(output.end(), msg.begin(), msg.end());
  return output;
}
}

DinersHost::Status DinersHost::AuthorizeSale(DinersTransaction& tx) {
	return PerformOnline(&BuildSaleRequest, &ReadSaleResponse, tx);
}

DinersHost::Status DinersHost::PerformVoid(DinersTransaction& tx) {
    return PerformOnline(&BuildVoidRequest, &ReadVoidResponse, tx);
}

DinersHost::Status DinersHost::PerformTcUpload(DinersTransaction& tx) {
  return PerformOnline(&BuildTcUploadRequest, &ReadTcUploadResponse, tx);
}

DinersHost::Status DinersHost::PerformTMKDownload(DinersTransaction& tx) {
    return PerformOnline(&BuildKeyDownloadRequest, &ReadKeyDownloadResponse, tx);
}

DinersHost::Status DinersHost::AuthorizeRefund(DinersTransaction& tx) {
    return PerformOnline(&BuildRefundRequest, &ReadRefundResponse, tx);
}

DinersHost::Status DinersHost::AuthorizePreAuth(DinersTransaction& tx) {
    return PerformOnline(&BuildPreAuthRequest, &ReadPreAuthResponse, tx);
}

DinersHost::Status DinersHost::SendTipAdjust(DinersTransaction& tx) {
    return PerformOnline(&BuildTipAdjustRequest, &ReadTipAdjustResponse, tx);
}

DinersHost::Status DinersHost::SendReversal(DinersTransaction& tx) {
    return PerformOnline(&BuildReversalRequest, &ReadReversalResponse, tx);
}

DinersHost::Status DinersHost::SendOfflineSale(DinersTransaction& tx) {
    return PerformOnline(&BuildOfflineSaleRequest, &ReadOfflineSaleResponse, tx);
}

DinersHost::Status DinersHost::PerformSaleCompletion(DinersTransaction& tx) {
    return PerformOnline(&BuildSaleCompletionRequest, &ReadSaleCompletionResponse, tx);
}

DinersHost::Status DinersHost::PerformDinersTestTransaction(TestTransaction& test_tx) {
    return PerformOnline(&BuildEchoTestRequest, &ReadEchoTestResponse, test_tx);
}

DinersHost::Status DinersHost::PerformSettlement(DinersSettlementData& settle_msg, bool after_batch_upload) {
	iso8583::Apdu settlement_request = BuildSettlementRequest(settle_msg, after_batch_upload);
    return PerformOnline(settlement_request, &ReadSettlementResponse, settle_msg);
}

DinersHost::Status DinersHost::PerformBatchUpload(DinersTransaction& tx, unsigned int batch_upload_stan) {
	iso8583::Apdu batch_upload_request = BuildBatchUploadRequest(tx, batch_upload_stan);
    return PerformOnline(batch_upload_request, &ReadBatchUploadResponse, tx);
}

bool DinersHost::PreConnect(const std::string& host_name) {
    comms_ = comms::Client(host_name.c_str());
    comms::CommsStatus status = comms_.PreConnect();
    if (status == comms::COMMS_OK)
    	return true;
    return false;
}

bool DinersHost::WaitForConnection(std::uint32_t timeout) {
    return comms_.WaitConnected(timeout) == comms::COMMS_CONNECTED;
}

bool DinersHost::Disconnect() {
    comms::CommsStatus status = comms_.Disconnect();
    if (status == comms::COMMS_OK)
    	return true;
    return false;
}

DinersHost::Status DinersHost::SendMessage(const utils::bytes& msg, const std::string tpdu) {
	uint64_t byte_sent;
    comms::CommsStatus comms_status;

    std::vector<uint8_t> msg_to_send = AddTpdu(msg, tpdu);
    logger::xdebug(msg_to_send.data(), msg_to_send.size());

    comms_status = comms_.Send(msg_to_send, &byte_sent);
    if (comms_status != comms::COMMS_OK) {
    	logger::error("DINERS - Error when sending message");
        comms_.Disconnect();
        return TRANSIENT_FAILURE;
    }

    if (byte_sent != msg_to_send.size()) {
    	logger::error("DINERS - Message not fully sent");
        comms_.Disconnect();
        return TRANSIENT_FAILURE;
    }

    return COMPLETED;
}

DinersHost::Status DinersHost::ReceiveMessage(utils::bytes& msg) {
    const size_t kMaxBytes = 99999;

    comms::CommsStatus comms_status = comms_.Receive(msg, kMaxBytes);
    if (comms_status != comms::COMMS_OK) {
    	comms_.Disconnect();
        return TRANSIENT_FAILURE;;
    }

    logger::xdebug(msg.data(), msg.size());
    if (msg.size() < kTpduSize) {
        comms_.Disconnect();
        return TRANSIENT_FAILURE;
    }

    // strip TPDU and HEADER
    msg.erase(msg.begin(), msg.begin() + kTpduSize);
    return COMPLETED;
}

template<typename T>
DinersHost::Status DinersHost::PerformOnline(BuildRequestFunc<T> request_func, ReadAndValidateResponseFunc<T> response_func, T& tx) {
	return PerformOnline(request_func(tx), response_func, tx);
}

template<typename T>
DinersHost::Status DinersHost::PerformOnline(iso8583::Apdu request, ReadAndValidateResponseFunc<T> response_func, T& tx) {
	std::uint32_t timeout = 30000;
    if (comms_.WaitConnected(timeout) != comms::COMMS_CONNECTED) {
    	comms_.Disconnect();
        return TRANSIENT_FAILURE;
    }

    if (SendMessage(request.text, tx.tpdu) != COMPLETED)
    	return TRANSIENT_FAILURE;

    utils::bytes msg_response_v;
    if (ReceiveMessage(msg_response_v) != COMPLETED)
        return TRANSIENT_FAILURE;

    if (!response_func(msg_response_v, tx))
    	return Status::PERM_FAILURE;

    return COMPLETED;
}


