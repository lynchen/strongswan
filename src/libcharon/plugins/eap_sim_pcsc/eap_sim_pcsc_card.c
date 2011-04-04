/*
 * Copyright (C) 2011 Duncan Salerno
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "eap_sim_pcsc_card.h"

#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#include <daemon.h>

typedef struct private_eap_sim_pcsc_card_t private_eap_sim_pcsc_card_t;

/**
 * Private data of an eap_sim_pcsc_card_t object.
 */
struct private_eap_sim_pcsc_card_t {

	/**
	 * Public eap_sim_pcsc_card_t interface.
	 */
	eap_sim_pcsc_card_t public;
};

/**
 * Maximum length for an IMSI.
 */
#define SIM_IMSI_MAX_LEN 15

/**
 * Length of the status at the end of response APDUs.
 */
#define APDU_STATUS_LEN 2

/**
 * First byte of status word indicating success.
 */
#define APDU_SW1_SUCCESS 0x90

/**
 * First byte of status word indicating there is response data to be read.
 */
#define APDU_SW1_RESPONSE_DATA 0x9f

/**
 * Decode IMSI EF (Elementary File) into an ASCII string
 */
static bool decode_imsi_ef(unsigned char *input, int input_len, char *output)
{
	/* Only digits 0-9 valid in IMSIs */
	static const char bcd_num_digits[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', '\0', '\0', '\0', '\0', '\0', '\0'
	};
	int i;

	/* Check length byte matches how many bytes we have, and that input is correct length for an IMSI */
	if (input[0] != input_len-1 || input_len < 2 || input_len > 9)
	{
		return FALSE;
	}

	/* Check type byte is IMSI */
	if ((input[1] & 0xf) != 0x9)
	{
		return FALSE;
	}
	*output++ = bcd_num_digits[input[1] >> 4];

	for (i = 2; i < input_len; i++)
	{
		*output++ = bcd_num_digits[input[i] & 0xf];
		*output++ = bcd_num_digits[input[i] >> 4];
	}

	*output++ = '\0';
	return TRUE;
}

/**
 * Implementation of sim_card_t.get_triplet
 */
static bool get_triplet(private_eap_sim_pcsc_card_t *this,
						identification_t *id, char *rand, char *sres, char *kc)
{
	status_t found = FALSE;
	LONG rv;
	SCARDCONTEXT hContext;
	DWORD dwReaders;
	LPSTR mszReaders;
	char *cur_reader;
	char full_nai[128];

	snprintf(full_nai, sizeof(full_nai), "%Y", id);

	DBG2(DBG_IKE, "looking for triplet: %Y rand %b", id, rand, SIM_RAND_LEN);

	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		DBG1(DBG_IKE, "SCardEstablishContext: %s", pcsc_stringify_error(rv));
		return FALSE;
	}
	
	rv = SCardListReaders(hContext, NULL, NULL, &dwReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		DBG1(DBG_IKE, "SCardListReaders: %s", pcsc_stringify_error(rv));
		return FALSE;
	}
	mszReaders = malloc(sizeof(char)*dwReaders);

	rv = SCardListReaders(hContext, NULL, mszReaders, &dwReaders);
	if (rv != SCARD_S_SUCCESS)
	{
		DBG1(DBG_IKE, "SCardListReaders: %s", pcsc_stringify_error(rv));
		return FALSE;
	}		

	/* mszReaders is a multi-string of readers, separated by '\0' and terminated by an additional '\0' */
	for (cur_reader = mszReaders; *cur_reader != '\0' && found == FALSE; cur_reader += strlen(cur_reader)+1)
	{
		DWORD dwActiveProtocol = -1;
		SCARDHANDLE hCard;
		SCARD_IO_REQUEST *pioSendPci;
		SCARD_IO_REQUEST pioRecvPci;
		BYTE pbRecvBuffer[64];
		DWORD dwRecvLength;
		char imsi[SIM_IMSI_MAX_LEN+1];

		/* See GSM 11.11 for SIM APDUs */
		BYTE pbSelectMF[] = { 0xa0, 0xa4, 0x00, 0x00, 0x02, 0x3f, 0x00 };
		BYTE pbSelectDFGSM[] = { 0xa0, 0xa4, 0x00, 0x00, 0x02, 0x7f, 0x20 };
		BYTE pbSelectIMSI[] = { 0xa0, 0xa4, 0x00, 0x00, 0x02, 0x6f, 0x07 };
		BYTE pbReadBinary[] = { 0xa0, 0xb0, 0x00, 0x00, 0x09 };
		BYTE pbRunGSMAlgorithm[] = { 0xa0, 0x88, 0x00, 0x00, 0x10, /*RAND*/ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
		BYTE pbGetResponse[] = { 0xa0, 0xc0, 0x00, 0x00, 0x0c };

		/* Copy RAND into APDU */
		memcpy(pbRunGSMAlgorithm+5, rand, SIM_RAND_LEN);

		rv = SCardConnect(hContext, cur_reader, SCARD_SHARE_SHARED,
			SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardConnect: %s", pcsc_stringify_error(rv));
			continue;
		}

		switch(dwActiveProtocol)
		{
			case SCARD_PROTOCOL_T0:
				pioSendPci = SCARD_PCI_T0;
				break;
			case SCARD_PROTOCOL_T1:
				pioSendPci = SCARD_PCI_T1;
				break;
			default:
				DBG1(DBG_IKE, "Unknown SCARD_PROTOCOL");
				continue;
		}

		/* APDU: Select MF */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbSelectMF, sizeof(pbSelectMF),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}
		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_RESPONSE_DATA )
		{
			DBG1(DBG_IKE, "Select MF failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* APDU: Select DF GSM */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbSelectDFGSM, sizeof(pbSelectDFGSM),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}
		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_RESPONSE_DATA )
		{
			DBG1(DBG_IKE, "Select DF GSM failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* APDU: Select IMSI */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbSelectIMSI, sizeof(pbSelectIMSI),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}
		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_RESPONSE_DATA )
		{
			DBG1(DBG_IKE, "Select IMSI failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* APDU: Read Binary (of IMSI) */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbReadBinary, sizeof(pbReadBinary),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}
		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_SUCCESS )
		{
			DBG1(DBG_IKE, "Select IMSI failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		if (!decode_imsi_ef(pbRecvBuffer, dwRecvLength-APDU_STATUS_LEN, imsi) )
		{
			DBG1(DBG_IKE, "Couldn't decode IMSI EF: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* The IMSI could be post/prefixed in the full NAI, so just make sure it's in there */
		if( !(strlen(full_nai) && strstr(full_nai, imsi)) )
		{
			DBG1(DBG_IKE, "Not the SIM we're looking for, IMSI: %s", imsi);
			continue;
		}

		/* APDU: Run GSM Algorithm */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbRunGSMAlgorithm, sizeof(pbRunGSMAlgorithm),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}
		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_RESPONSE_DATA )
		{
			DBG1(DBG_IKE, "Run GSM Algorithm failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* APDU: Get Response (of Run GSM Algorithm) */
		dwRecvLength = sizeof(pbRecvBuffer);
		rv = SCardTransmit(hCard, pioSendPci, pbGetResponse, sizeof(pbGetResponse),
			&pioRecvPci, pbRecvBuffer, &dwRecvLength);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardTransmit: %s", pcsc_stringify_error(rv));
			continue;
		}

		if( dwRecvLength < APDU_STATUS_LEN || pbRecvBuffer[dwRecvLength-APDU_STATUS_LEN] != APDU_SW1_SUCCESS )
		{
			DBG1(DBG_IKE, "Get Response failed: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		/* Extract out Kc and SRES from response */
		if( dwRecvLength == SIM_SRES_LEN + SIM_KC_LEN + APDU_STATUS_LEN )
		{
			memcpy(sres, pbRecvBuffer, SIM_SRES_LEN);
			memcpy(kc, pbRecvBuffer+4, SIM_KC_LEN);
			/* This will also cause the loop to exit */
			found = TRUE;
		}
		else
		{
			DBG1(DBG_IKE, "Get Response incorrect length: %b", pbRecvBuffer, dwRecvLength);
			continue;
		}

		rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
		if (rv != SCARD_S_SUCCESS)
		{
			DBG1(DBG_IKE, "SCardDisconnect: %s", pcsc_stringify_error(rv));
			continue;
		}
	}

	rv = SCardReleaseContext(hContext);
	if (rv != SCARD_S_SUCCESS)
	{
		DBG1(DBG_IKE, "SCardReleaseContext: %s", pcsc_stringify_error(rv));
	}

	free(mszReaders);
	return found;
}

/**
 * Implementation of sim_card_t.get_quintuplet
 */
static status_t get_quintuplet()
{
	return NOT_SUPPORTED;
}

/**
 * Implementation of eap_sim_pcsc_card_t.destroy.
 */
static void destroy(private_eap_sim_pcsc_card_t *this)
{
	free(this);
}

/**
 * See header
 */
eap_sim_pcsc_card_t *eap_sim_pcsc_card_create()
{
	private_eap_sim_pcsc_card_t *this = malloc_thing(private_eap_sim_pcsc_card_t);

	this->public.card.get_triplet = (bool(*)(sim_card_t*, identification_t *id, char rand[SIM_RAND_LEN], char sres[SIM_SRES_LEN], char kc[SIM_KC_LEN]))get_triplet;
	this->public.card.get_quintuplet = (status_t(*)(sim_card_t*, identification_t *id, char rand[AKA_RAND_LEN], char autn[AKA_AUTN_LEN], char ck[AKA_CK_LEN], char ik[AKA_IK_LEN], char res[AKA_RES_MAX], int *res_len))get_quintuplet;
	this->public.card.resync = (bool(*)(sim_card_t*, identification_t *id, char rand[AKA_RAND_LEN], char auts[AKA_AUTS_LEN]))return_false;
	this->public.card.get_pseudonym = (identification_t*(*)(sim_card_t*, identification_t *perm))return_null;
	this->public.card.set_pseudonym = (void(*)(sim_card_t*, identification_t *id, identification_t *pseudonym))nop;
	this->public.card.get_reauth = (identification_t*(*)(sim_card_t*, identification_t *id, char mk[HASH_SIZE_SHA1], u_int16_t *counter))return_null;
	this->public.card.set_reauth = (void(*)(sim_card_t*, identification_t *id, identification_t* next, char mk[HASH_SIZE_SHA1], u_int16_t counter))nop;
	this->public.destroy = (void(*)(eap_sim_pcsc_card_t*))destroy;

	return &this->public;
}

