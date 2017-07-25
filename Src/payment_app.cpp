/*
 ------------------------------------------------------------------------------
 INGENICO Technical Software Department
 ------------------------------------------------------------------------------
 Copyright (c) 2014 INGENICO S.A.
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
#include "payment_app.h"

#include <stdx/memory>
#include <limits>
#include <tpcore/telium_printer.h>
extern "C" {
#include "SEC_interface.h"
}
#include <fdms/sqlite_batch.h>
#include <fdms/goal_ui.h>
#include <fdms/goal_receipts.h>
#include <fdms/emv_config.h>
#include <fdms/contactless_config.h>
#include <menu_manager.h>
#include <app_ssl_profile.h>
#include <logger/ldbg.h>

namespace fdms {

const std::string keyBatchPath = "/HOST/Batch.db";

PaymentApp::PaymentApp(int app_no, int app_type)
    : TeliumApp(app_no, app_type) {
  RegisterService(IsNameService(this, kServiceHighPriority));
  RegisterService(AfterResetService(this, kServiceHighPriority));
  RegisterService(MoreFunctionService(this, kServiceHighPriority));
  RegisterService(MessageReceivedService(this, kServiceHighPriority));
  RegisterService(KeyboardEventService(this, kServiceHighPriority));
  RegisterService(IdleMessageService(this, kServiceHighPriority));
  RegisterService(IsTimeFunctionService(this, kServiceHighPriority));
}

/*
 * Accessors for interfaces.
 */

const char *PaymentApp::AppName() const {
  return TeliumApp::AppName();
}

unsigned char PaymentApp::AppNo() const {
  return TeliumApp::AppNo();
}

/*
 * TeliumApp interface.
 */

void PaymentApp::State() {
  tpcore::TeliumPrinter().
      Bold().
      PrintLn(AppName()).
      DefaultStyle().
      Print("File name : ").PrintLn(FileName()).
      WaitPrinted();

  version_.PrintVersion();

  tpcore::TeliumPrinter().
      DefaultStyle().
      Print("CRC     : ").PrintHex4(Crc()).PrintLn().
      Print("RAM Usage : ").Print(RamSizeKb()).PrintLn("kb").
      Print("HEAP Free : ").Print(HeapFreeKb()).PrintLn("kb").
      PrintLn().
      WaitPrinted();
}

bool PaymentApp::IsDeleteAllowed() {

  return true;
}

bool PaymentApp::IsEvolAllowed() {

  // TODO: may want to check batch empty here to allow remote update
  return true;
}

/*
 * WithAfterReset interface
 */

void PaymentApp::FirstReset(bool is_remote_download) {
  InitializeApplication(true);
}

void PaymentApp::NormalReset() {
  InitializeApplication(false);
}

/*
 * WithMoreFunction interface
 */

void PaymentApp::More() {
  int initial_header = IsHeaderDisplayed();
  DisplayHeader(OFF);

  menu_manager_->OpenMainMenu();

  if (initial_header == 1)
    DisplayHeader(ON);
}

void PaymentApp::Message(const S_MESSAGE_IAM& message) {
  ecr_manager_->MessageReceived();
}

/*
 * WithIdleMessage interface
 */

void PaymentApp::Idle() {
  ui_->MessageIdleScreen();
}

void PaymentApp::InitializeApplication(bool migrate_batch) {
  SEClib_Open();

  using stdx::make_unique;
  const std::string kFdmsPartitionName = "fdms";
  disk_manager_ = stdx::make_unique<tpcore::DiskManager>(
      kFdmsPartitionName.c_str());
  pin_entry_ = make_unique<tpcore::PinEntry>(registry::Reader());  // TODO: give proper reader
  app_setting_ = make_unique<ApplicationSettings>(kFdmsPartitionName,
                                                  version_.GetVersion(),
                                                  version_.GetBuildDate());
  emv_config_ = stdx::make_unique<emvconfig::EmvConfigPredefined>(
      BuildEmvConfig(*app_setting_));
  cless_config_ = BuildClessConfig(*app_setting_);
  ui_ = make_unique<GoalUi>(gl_);
  receipts_ = make_unique<GoalReceipts>(gl_, 1000);
  host_ = make_unique<HostSwitch>(*app_setting_);
  batch_ = make_unique<SqliteBatch>(*app_setting_, keyBatchPath, migrate_batch);
  password_ = make_unique<Password>(*app_setting_,*ui_);
  acquirer_ = make_unique<Acquirer>(*app_setting_,
                                         *batch_,
                                         *host_,
                                         *emv_config_,
                                         *cless_config_,
                                         *pin_entry_,
                                         *ui_,
                                         *receipts_,
                                         *password_);
  menu_manager_ = make_unique<MenuManager>(*app_setting_,
                                           *ui_,
                                           *receipts_,
                                           *acquirer_,
                                           *batch_,
                                           *password_);
  ecr_manager_ = make_unique<EcrManager>(*app_setting_,
                                         *ui_,
                                         *receipts_,
                                         *acquirer_,
                                         *batch_);

  LoadSslProfiles();
  logger::LoggerLdbg(2);
}

int PaymentApp::KeyIn(int key_code) {
  DisplayHeader(OFF);

  if (app_setting_->local_settings_->IsTerminalLocked()) {
    if(!password_->PerformUnlockTerminalPasswordAccess()) {
      key_code = 0;

      DisplayHeader(ON);
      return key_code;
    }

  }

  if (key_code >= T_NUM0 && key_code <= T_NUM9) {
    acquirer_->StartSale(key_code - '0');
    key_code = 0;

  } else {
    switch (key_code) {
      case T_F:  // F
        menu_manager_->OpenMainMenu();
        key_code = acquirer_->IsManagerSelection() ? T_F : 0;
        acquirer_->SetManagerSelection(false);
        break;
      case T_VAL:
        acquirer_->StartSale();
        key_code = 0;
        break;
      default:  // inhibit all other keys
        key_code = 0;
        break;
    }
  }

  DisplayHeader(ON);
  return key_code;
}

bool PaymentApp::IsTime() {
  acquirer_->IncrementOfflineSaleTimer();
  return true;
}

}
