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
#ifndef DINERS__PROTOCOL_H_
#define DINERS__PROTOCOL_H_

#include <iso8583/apdu.h>

namespace diners {

const int kFieldPan = 2;  // Primary account number
const int kFieldProcessingCode = 3;  // Processing Code
const int kFieldAmount = 4;  // Amount
const int kFieldDateTimeTransmission = 7;  // Date Time transmission
const int kFieldStan = 11;  // Systems trace audit number
const int kFieldTimeLocalTransaction = 12;  // Time, local transaction
const int kFieldDateLocalTransaction = 13;  // Date, local transaction
const int kFieldDateExpiration = 14;  // Date, expiration
const int kFieldDateSettlement = 15;  // Date, settlement
const int kFieldPosEntryMode = 22;  // POS entry mode
const int kFieldPanSequenceNumber = 23; //PAN sequence number
const int kFieldNii = 24;  // Network Intl ID
const int kFieldPosConditionCode = 25;  // POS condition code
const int kFieldTrack2Data = 35;  // Track 2 data
const int kFieldRrn = 37;  // Retrieval reference number
const int kFieldAuthorizationId = 38;  // Authorisation ID
const int kFieldResponseCode = 39;  // Response code
const int kFieldCardAcceptorTerminalId = 41;  // Card acceptor terminal ID
const int kFieldCardAcceptorId = 42;  // Card acceptor ID
const int kFieldCardAcceptorNameLocation = 43;  // Card acceptor Name/Location
const int kFieldAdditionalResponseData = 44;  // Additional Response Data
const int kFieldTrack1Data = 45;  // Track 1 data
const int kFieldAdditionalDataNational = 47;  // Additional Data National
const int kFieldAdditionalDataPrivate = 48;  // Additional Data Private
const int kFieldCurrencyCode = 49;  // Currency
const int kFieldPinBlock = 52;  // PIN data
const int kFieldAdditionalAmount = 54;  // Additional amount
const int kFieldIccData = 55;  // ICC data
const int kFieldNationalUseData = 59;  // National Use Data
const int kField60 = 60;  // Private Use
const int kField61 = 61;  // Private Use
const int kField62 = 62;  // Private Use
const int kField63 = 63;  // Private Use

iso8583::ApduSpec& GetProtocolSpec();

}

#endif
