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

#include <protocol.h>
#include <iso8583/field_types.h>

using namespace iso8583;

namespace diners {

namespace {

ApduSpec& diners_spec() {
  static ApduSpec spec = ApduSpec().SetFieldSpecs(
      { { kFieldPan, Bcd2VarCompressedNumericRightPadded<19, 15>::spec() }, {
          kFieldProcessingCode, FixedCompressedNumeric<6>::spec() }, {
          kFieldAmount, FixedCompressedNumeric<12>::spec() }, {
          kFieldDateTimeTransmission, FixedCompressedNumeric<10>::spec() }, {
          kFieldStan, FixedCompressedNumeric<6>::spec() }, {
          kFieldTimeLocalTransaction, FixedCompressedNumeric<6>::spec() }, {
          kFieldDateLocalTransaction, FixedCompressedNumeric<4>::spec() }, {
          kFieldDateExpiration, FixedCompressedNumeric<4>::spec() }, {
          kFieldDateSettlement, FixedCompressedNumeric<4>::spec() }, {
          kFieldPosEntryMode, FixedCompressedNumeric<4>::spec() }, {
          kFieldPanSequenceNumber, FixedCompressedNumeric<4>::spec() }, {
          kFieldNii, FixedCompressedNumeric<4>::spec() }, {
          kFieldPosConditionCode, FixedCompressedNumeric<2>::spec() }, {
          kFieldTrack2Data, Track2Data<99>::spec() }, { // TODO: review max length
          kFieldRrn, FixedAnsRightPadded<12>::spec() }, {
          kFieldAuthorizationId, FixedAnsRightPadded<6, ' '>::spec() }, {
          kFieldResponseCode, FixedAnsRightPadded<2>::spec() }, {
          kFieldCardAcceptorTerminalId, FixedAnsRightPadded<8>::spec() }, {
          kFieldCardAcceptorId, FixedAnsRightPadded<15>::spec() }, {
          kFieldCardAcceptorNameLocation, FixedAnsRightPadded<40, ' '>::spec() }, {
          kFieldAdditionalResponseData, Bcd2VarAns<25>::spec() }, { // TODO: review max length
          kFieldTrack1Data, Bcd2VarAns<77>::spec() }, { // TODO: review max length
          kFieldAdditionalDataNational, Bcd3VarAns<304>::spec() }, { // TODO: review max length
          kFieldAdditionalDataPrivate, Bcd3VarAns<81>::spec() }, { // TODO: review max length
          kFieldCurrencyCode, FixedCompressedNumeric<4>::spec() }, {
          kFieldPinBlock, FixedBinary<64>::spec() }, {
          kFieldAdditionalAmount, Bcd3VarAns<12>::spec() },{
          kFieldIccData, Bcd3VarBytes<999>::spec() }, {  // TODO: review max length
          kFieldNationalUseData, Bcd3VarAns<999>::spec() }, {  // TODO: review max length
          kField60, Bcd3VarAns<999>::spec() }, {  // TODO: review max length
          kField61, Bcd3VarAns<999>::spec() }, {  // TODO: review max length
          kField62, Bcd3VarAns<999>::spec() }, {  // TODO: review max length
          kField63, Bcd3VarBytes<256>::spec() }, }); // TODO: review max length

  return spec;
}

}

iso8583::ApduSpec& GetProtocolSpec() {
  return diners_spec();
}

}


