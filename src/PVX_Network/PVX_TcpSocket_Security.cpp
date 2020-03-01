// Compiles with Visual Studio 2008 for Windows

// This C example is designed as more of a guide than a library to be plugged into an application
// That module required a couple of major re-writes and is available upon request
// The Basic example has tips to the direction you should take
// This will work with connections on port 587 that upgrade a plain text session to an encrypted session with STARTTLS as covered here.

// TLSclient.c - SSPI Schannel gmail TLS connection example

#define SECURITY_WIN32
#define IO_BUFFER_SIZE  0x10000
#define NT4_DLL_NAME TEXT("Security.dll")

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>
#include <string>
#include <array>
#include <vector>

#pragma comment(lib, "WSock32.Lib")
#pragma comment(lib, "Crypt32.Lib")
// #pragma comment(lib, "user32.lib")
// #pragma comment(lib, "MSVCRTD.lib")

// Globals.

LPSTR	pszUser = 0; // if specified, a certificate in "MY" store is searched for

DWORD	dwProtocol = SP_PROT_TLS1; // SP_PROT_TLS1; // SP_PROT_PCT1; SP_PROT_SSL2; SP_PROT_SSL3; 0=default
ALG_ID	aiKeyExch = 0; // = default; CALG_DH_EPHEM; CALG_RSA_KEYX;

HCERTSTORE hMyCertStore = NULL;

SCHANNEL_CRED SchannelCred;

namespace PVX {
	namespace Network {

		HMODULE Secur32 = 0;
		PSecurityFunctionTableW SChannel = 0;
		struct __Secure__ {
			void Init() {
				if (!Secur32) {
					Secur32 = LoadLibraryA("Secur32.dll");
					SChannel = ((INIT_SECURITY_INTERFACE_W)GetProcAddress(Secur32, "InitSecurityInterfaceW"))();
				}
			}
			~__Secure__() {
				if (Secur32) FreeLibrary(Secur32);
				Secur32 = NULL;
			}
		} _Secturity_;

		/*****************************************************************************/
		static DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, std::wstring ServerName, DWORD dwCertFlags) {
			HTTPSPolicyCallbackData  polHttps;
			CERT_CHAIN_POLICY_PARA   PolicyPara;
			CERT_CHAIN_POLICY_STATUS PolicyStatus;
			CERT_CHAIN_PARA          ChainPara{};
			PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
			DWORD                    Status;

			
			LPSTR rgszUsages[] = { 
				(LPSTR)szOID_PKIX_KP_SERVER_AUTH,
				(LPSTR)szOID_SERVER_GATED_CRYPTO,
				(LPSTR)szOID_SGC_NETSCAPE
			};

			DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

			if (pServerCert == NULL) { Status = SEC_E_WRONG_PRINCIPAL; goto cleanup; }

			// Build certificate chain.
			ChainPara.cbSize = sizeof(ChainPara);
			ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
			ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
			ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

			if (!CertGetCertificateChain(NULL,
				pServerCert,
				NULL,
				pServerCert->hCertStore,
				&ChainPara,
				0,
				NULL,
				&pChainContext)) {
				Status = GetLastError();
				printf("Error 0x%x returned by CertGetCertificateChain!\n", Status);
				goto cleanup;
			}


			// Validate certificate chain.
			ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
			polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
			polHttps.dwAuthType = AUTHTYPE_SERVER;
			polHttps.fdwChecks = dwCertFlags;
			polHttps.pwszServerName = &ServerName[0];

			memset(&PolicyPara, 0, sizeof(PolicyPara));
			PolicyPara.cbSize = sizeof(PolicyPara);
			PolicyPara.pvExtraPolicyPara = &polHttps;

			memset(&PolicyStatus, 0, sizeof(PolicyStatus));
			PolicyStatus.cbSize = sizeof(PolicyStatus);

			if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
				pChainContext,
				&PolicyPara,
				&PolicyStatus)) {
				Status = GetLastError();
				printf("Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", Status);
				goto cleanup;
			}

			if (PolicyStatus.dwError) {
				Status = PolicyStatus.dwError;
				goto cleanup;
			}

			Status = SEC_E_OK;

		cleanup:
			if (pChainContext)  CertFreeCertificateChain(pChainContext);
			return Status;
		}

		/*****************************************************************************/
		static SECURITY_STATUS CreateCredentials(LPSTR pszUser, PCredHandle phCreds) { //                                                in                     out
			TimeStamp        tsExpiry;
			SECURITY_STATUS  Status;
			DWORD            cSupportedAlgs = 0;
			ALG_ID           rgbSupportedAlgs[16];
			PCCERT_CONTEXT   pCertContext = NULL;

			// Open the "MY" certificate store, where IE stores client certificates.
				// Windows maintains 4 stores -- MY, CA, ROOT, SPC.
			if (hMyCertStore == NULL) {
				hMyCertStore = CertOpenSystemStoreA(0, "MY");
				if (!hMyCertStore) {
					printf("**** Error 0x%x returned by CertOpenSystemStore\n", GetLastError());
					return SEC_E_NO_CREDENTIALS;
				}
			}

			// If a user name is specified, then attempt to find a client
			// certificate. Otherwise, just create a NULL credential.
			if (pszUser) {
				// Find client certificate. Note that this sample just searches for a
				// certificate that contains the user name somewhere in the subject name.
				// A real application should be a bit less casual.
				pCertContext = CertFindCertificateInStore(hMyCertStore,                     // hCertStore
														   X509_ASN_ENCODING,             // dwCertEncodingType
														   0,                                             // dwFindFlags
														   CERT_FIND_SUBJECT_STR_A,// dwFindType
														   pszUser,                         // *pvFindPara
														   NULL);                                 // pPrevCertContext


				if (pCertContext == NULL) {
					printf("**** Error 0x%x returned by CertFindCertificateInStore\n", GetLastError());
					if (GetLastError() == CRYPT_E_NOT_FOUND) printf("CRYPT_E_NOT_FOUND - property doesn't exist\n");
					return SEC_E_NO_CREDENTIALS;
				}
			}

			// Build Schannel credential structure. Currently, this sample only
			// specifies the protocol to be used (and optionally the certificate,
			// of course). Real applications may wish to specify other parameters as well.
			ZeroMemory(&SchannelCred, sizeof(SchannelCred));

			SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
			if (pCertContext) {
				SchannelCred.cCreds = 1;
				SchannelCred.paCred = &pCertContext;
			}

			SchannelCred.grbitEnabledProtocols = dwProtocol;

			if (aiKeyExch) rgbSupportedAlgs[cSupportedAlgs++] = aiKeyExch;

			if (cSupportedAlgs) {
				SchannelCred.cSupportedAlgs = cSupportedAlgs;
				SchannelCred.palgSupportedAlgs = rgbSupportedAlgs;
			}

			SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

			// The SCH_CRED_MANUAL_CRED_VALIDATION flag is specified because
			// this sample verifies the server certificate manually.
			// Applications that expect to run on WinNT, Win9x, or WinME
			// should specify this flag and also manually verify the server
			// certificate. Applications running on newer versions of Windows can
			// leave off this flag, in which case the InitializeSecurityContext
			// function will validate the server certificate automatically.
			SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;


			// Create an SSPI credential.
			Status = SChannel->AcquireCredentialsHandleW(0, (SEC_WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, 0, &SchannelCred, 0, 0, phCreds, &tsExpiry); // (out) Lifetime (optional)

			// cleanup: Free the certificate context. Schannel has already made its own copy.
			if (pCertContext) CertFreeCertificateContext(pCertContext);

			return Status;
		}

		/*****************************************************************************/
		static INT ConnectToServer(const char* pszServerName, INT iPortNumber, SOCKET* pSocket) { //                                    in                in                 out
			SOCKET Socket;
			struct sockaddr_in sin;
			struct hostent* hp;


			Socket = socket(PF_INET, SOCK_STREAM, 0);
			if (Socket == INVALID_SOCKET) {
				return WSAGetLastError();
			}


			else // No proxy used
			{
				sin.sin_family = AF_INET;
				sin.sin_port = htons((u_short)iPortNumber);
				if ((hp = gethostbyname(pszServerName)) == NULL) {
					return WSAGetLastError();
				} else
					memcpy(&sin.sin_addr, hp->h_addr, 4);
			}


			if (connect(Socket, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
				closesocket(Socket);
				return WSAGetLastError();
			}


			*pSocket = Socket;

			return SEC_E_OK;
		}

		/*****************************************************************************/
		static LONG DisconnectFromServer(SOCKET Socket, PCredHandle phCreds, CtxtHandle* phContext) {
			PBYTE                    pbMessage;
			DWORD                    dwType, dwSSPIFlags, dwSSPIOutFlags, cbMessage, cbData, Status;
			SecBufferDesc OutBuffer;
			SecBuffer     OutBuffers[1];
			TimeStamp     tsExpiry;


			dwType = SCHANNEL_SHUTDOWN; // Notify schannel that we are about to close the connection.

			OutBuffers[0].pvBuffer = &dwType;
			OutBuffers[0].BufferType = SECBUFFER_TOKEN;
			OutBuffers[0].cbBuffer = sizeof(dwType);

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			Status = SChannel->ApplyControlToken(phContext, &OutBuffer);
			if (FAILED(Status)) { goto cleanup; }


		// Build an SSL close notify message.
			dwSSPIFlags = 
				ISC_REQ_SEQUENCE_DETECT   |
				ISC_REQ_REPLAY_DETECT     |
				ISC_REQ_CONFIDENTIALITY   |
				ISC_RET_EXTENDED_ERROR    |
				ISC_REQ_ALLOCATE_MEMORY   |
				ISC_REQ_STREAM;

			OutBuffers[0].pvBuffer = NULL;
			OutBuffers[0].BufferType = SECBUFFER_TOKEN;
			OutBuffers[0].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			Status = SChannel->InitializeSecurityContextW(phCreds,
														phContext,
														NULL,
														dwSSPIFlags,
														0,
														SECURITY_NATIVE_DREP,
														NULL,
														0,
														phContext,
														&OutBuffer,
														&dwSSPIOutFlags,
														&tsExpiry);

			if (FAILED(Status)) { goto cleanup; }

			pbMessage = (PBYTE)OutBuffers[0].pvBuffer;
			cbMessage = OutBuffers[0].cbBuffer;


			// Send the close notify message to the server.
			if (pbMessage != NULL && cbMessage != 0) {
				cbData = send(Socket, (const char*)pbMessage, cbMessage, 0);
				if (cbData == SOCKET_ERROR || cbData == 0) {
					Status = WSAGetLastError();
					goto cleanup;
				}
				SChannel->FreeContextBuffer(pbMessage); // Free output buffer.
			}


		cleanup:
			SChannel->DeleteSecurityContext(phContext); // Free the security context.
			closesocket(Socket); // Close the socket.

			return Status;
		}

		/*****************************************************************************/
		static void GetNewClientCredentials(CredHandle* phCreds, CtxtHandle* phContext) {

			CredHandle						hCreds;
			SecPkgContext_IssuerListInfoEx	IssuerListInfo;
			PCCERT_CHAIN_CONTEXT			pChainContext;
			CERT_CHAIN_FIND_BY_ISSUER_PARA	FindByIssuerPara;
			PCCERT_CONTEXT					pCertContext;
			TimeStamp						tsExpiry;
			SECURITY_STATUS					Status;


			// Read list of trusted issuers from schannel.
			Status = SChannel->QueryContextAttributesW(phContext, SECPKG_ATTR_ISSUER_LIST_EX, (PVOID)&IssuerListInfo);

			// Enumerate the client certificates.
			ZeroMemory(&FindByIssuerPara, sizeof(FindByIssuerPara));

			FindByIssuerPara.cbSize = sizeof(FindByIssuerPara);
			FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
			FindByIssuerPara.dwKeySpec = 0;
			FindByIssuerPara.cIssuer = IssuerListInfo.cIssuers;
			FindByIssuerPara.rgIssuer = IssuerListInfo.aIssuers;

			pChainContext = NULL;

			while (TRUE) {   // Find a certificate chain.
				pChainContext = CertFindChainInStore(hMyCertStore,
													  X509_ASN_ENCODING,
													  0,
													  CERT_CHAIN_FIND_BY_ISSUER,
													  &FindByIssuerPara,
													  pChainContext);
				if (pChainContext == NULL) { printf("Error 0x%x finding cert chain\n", GetLastError()); break; }

				printf("\ncertificate chain found\n");

		// Get pointer to leaf certificate context.
				pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

				// Create schannel credential.
				SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
				SchannelCred.cCreds = 1;
				SchannelCred.paCred = &pCertContext;

				Status = SChannel->AcquireCredentialsHandleW(
					NULL,                   // Name of principal
					(SEC_WCHAR*)UNISP_NAME_W,           // Name of package
					SECPKG_CRED_OUTBOUND,   // Flags indicating use
					NULL,                   // Pointer to logon ID
					&SchannelCred,          // Package specific data
					NULL,                   // Pointer to GetKey() func
					NULL,                   // Value to pass to GetKey()
					&hCreds,                // (out) Cred Handle
					&tsExpiry);            // (out) Lifetime (optional)
				
				SChannel->FreeCredentialsHandle(phCreds); // Destroy the old credentials.

				*phCreds = hCreds;

			}
		}

		/*****************************************************************************/
		static SECURITY_STATUS ClientHandshakeLoop(SOCKET Socket, PCredHandle phCreds, CtxtHandle* phContext, BOOL fDoInitialRead, SecBuffer* pExtraData) {
			SecBufferDesc   OutBuffer, InBuffer;
			SecBuffer       InBuffers[2], OutBuffers[1];
			DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer;
			TimeStamp       tsExpiry;
			SECURITY_STATUS scRet;
			std::vector<char> IoBuffer(IO_BUFFER_SIZE);
			BOOL            fDoRead;


			dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

			cbIoBuffer = 0;
			fDoRead = fDoInitialRead;

			// Loop until the handshake is finished or an error occurs.
			scRet = SEC_I_CONTINUE_NEEDED;

			while (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS) {
				if (0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) // Read data from server.
				{
					if (fDoRead) {
						cbData = recv(Socket, &IoBuffer[0] + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
						if (cbData == SOCKET_ERROR) {
							scRet = SEC_E_INTERNAL_ERROR;
							break;
						} else if (cbData == 0) {
							scRet = SEC_E_INTERNAL_ERROR;
							break;
						}
						cbIoBuffer += cbData;
					} else
						fDoRead = TRUE;
				}

				InBuffers[0].pvBuffer = &IoBuffer[0];
				InBuffers[0].cbBuffer = cbIoBuffer;
				InBuffers[0].BufferType = SECBUFFER_TOKEN;

				InBuffers[1].pvBuffer = NULL;
				InBuffers[1].cbBuffer = 0;
				InBuffers[1].BufferType = SECBUFFER_EMPTY;

				InBuffer.cBuffers = 2;
				InBuffer.pBuffers = InBuffers;
				InBuffer.ulVersion = SECBUFFER_VERSION;

				OutBuffers[0].pvBuffer = NULL;
				OutBuffers[0].BufferType = SECBUFFER_TOKEN;
				OutBuffers[0].cbBuffer = 0;

				OutBuffer.cBuffers = 1;
				OutBuffer.pBuffers = OutBuffers;
				OutBuffer.ulVersion = SECBUFFER_VERSION;


				scRet = SChannel->InitializeSecurityContextW(phCreds, phContext, NULL, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, &InBuffer, 0, NULL, &OutBuffer, &dwSSPIOutFlags, &tsExpiry);

				if (scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR)) {
					if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) {
						cbData = send(Socket, (char*)(OutBuffers[0].pvBuffer), OutBuffers[0].cbBuffer, 0);
						if (cbData == SOCKET_ERROR || cbData == 0) {
							SChannel->FreeContextBuffer(OutBuffers[0].pvBuffer);
							SChannel->DeleteSecurityContext(phContext);
							return SEC_E_INTERNAL_ERROR;
						}
						SChannel->FreeContextBuffer(OutBuffers[0].pvBuffer);
						OutBuffers[0].pvBuffer = NULL;
					}
				}
				if (scRet == SEC_E_INCOMPLETE_MESSAGE) continue;
				
				if (scRet == SEC_E_OK) {
					if (InBuffers[1].BufferType == SECBUFFER_EXTRA) {
						pExtraData->pvBuffer = LocalAlloc(LMEM_FIXED, InBuffers[1].cbBuffer);

						MoveMemory(pExtraData->pvBuffer, &IoBuffer[0] + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);

						pExtraData->cbBuffer = InBuffers[1].cbBuffer;
						pExtraData->BufferType = SECBUFFER_TOKEN;
					} else {
						pExtraData->pvBuffer = NULL;
						pExtraData->cbBuffer = 0;
						pExtraData->BufferType = SECBUFFER_EMPTY;
					}
					break;
				}

				if (scRet == SEC_I_INCOMPLETE_CREDENTIALS) {
					GetNewClientCredentials(phCreds, phContext);

					// Go around again.
					fDoRead = FALSE;
					scRet = SEC_I_CONTINUE_NEEDED;
					continue;
				}

				// Copy any leftover data from the "extra" buffer, and go around again.
				if (InBuffers[1].BufferType == SECBUFFER_EXTRA) {
					MoveMemory(&IoBuffer[0], &IoBuffer[0] + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
					cbIoBuffer = InBuffers[1].cbBuffer;
				} else
					cbIoBuffer = 0;
			}

			// Delete the security context in the case of a fatal error.
			if (FAILED(scRet)) SChannel->DeleteSecurityContext(phContext);
			return scRet;
		}

		/*****************************************************************************/
		static SECURITY_STATUS PerformClientHandshake(SOCKET Socket, PCredHandle phCreds, const std::wstring& ServerName,CtxtHandle* phContext, SecBuffer* pExtraData) {

			SecBufferDesc   OutBuffer;
			SecBuffer       OutBuffers[1];
			DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData;
			TimeStamp       tsExpiry;
			SECURITY_STATUS scRet;


			dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   | ISC_REQ_REPLAY_DETECT     | ISC_REQ_CONFIDENTIALITY   |
				ISC_RET_EXTENDED_ERROR    | ISC_REQ_ALLOCATE_MEMORY   | ISC_REQ_STREAM;


  //  Initiate a ClientHello message and generate a token.
			OutBuffers[0].pvBuffer = NULL;
			OutBuffers[0].BufferType = SECBUFFER_TOKEN;
			OutBuffers[0].cbBuffer = 0;

			OutBuffer.cBuffers = 1;
			OutBuffer.pBuffers = OutBuffers;
			OutBuffer.ulVersion = SECBUFFER_VERSION;

			scRet = SChannel->InitializeSecurityContextW(
				phCreds,
				NULL,
				(SEC_WCHAR*)ServerName.c_str(),
				dwSSPIFlags,
				0,
				SECURITY_NATIVE_DREP,
				NULL,
				0,
				phContext,
				&OutBuffer,
				&dwSSPIOutFlags,
				&tsExpiry);

			// Send response to server if there is one.
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) {
				cbData = send(Socket, (char*)(OutBuffers[0].pvBuffer), OutBuffers[0].cbBuffer, 0);
				if (cbData == SOCKET_ERROR || cbData == 0) {
					SChannel->FreeContextBuffer(OutBuffers[0].pvBuffer);
					SChannel->DeleteSecurityContext(phContext);
					return SEC_E_INTERNAL_ERROR;
				}
				SChannel->FreeContextBuffer(OutBuffers[0].pvBuffer); // Free output buffer.
				OutBuffers[0].pvBuffer = NULL;
			}

			return ClientHandshakeLoop(Socket, phCreds, phContext, TRUE, pExtraData);
		}

		/*****************************************************************************/
		static DWORD EncryptSend(SOCKET Socket, CtxtHandle* phContext, char* pbIoBuffer, SecPkgContext_StreamSizes Sizes) {
			SECURITY_STATUS	scRet;            // unsigned long cbBuffer;    // Size of the buffer, in bytes
			SecBufferDesc	Message;        // unsigned long BufferType;  // Type of the buffer (below)
			SecBuffer		Buffers[4];    // void    SEC_FAR * pvBuffer;   // Pointer to the buffer
			DWORD			cbMessage, cbData;
			char*			pbMessage;


			pbMessage = pbIoBuffer + Sizes.cbHeader; // Offset by "header size"
			cbMessage = (DWORD)strlen(pbMessage);


				// Encrypt the HTTP request.
			Buffers[0].pvBuffer = pbIoBuffer;                                // Pointer to buffer 1
			Buffers[0].cbBuffer = Sizes.cbHeader;                        // length of header
			Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;    // Type of the buffer

			Buffers[1].pvBuffer = pbMessage;                                // Pointer to buffer 2
			Buffers[1].cbBuffer = cbMessage;                                // length of the message
			Buffers[1].BufferType = SECBUFFER_DATA;                        // Type of the buffer

			Buffers[2].pvBuffer = pbMessage + cbMessage;        // Pointer to buffer 3
			Buffers[2].cbBuffer = Sizes.cbTrailer;                    // length of the trailor
			Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;    // Type of the buffer

			Buffers[3].pvBuffer = SECBUFFER_EMPTY;                    // Pointer to buffer 4
			Buffers[3].cbBuffer = SECBUFFER_EMPTY;                    // length of buffer 4
			Buffers[3].BufferType = SECBUFFER_EMPTY;                    // Type of the buffer 4


			Message.ulVersion = SECBUFFER_VERSION;    // Version number
			Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
			Message.pBuffers = Buffers;                        // Pointer to array of buffers
			scRet = SChannel->EncryptMessage(phContext, 0, &Message, 0); // must contain four SecBuffer structures.
		
			return send(Socket, pbIoBuffer, Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer, 0);

		}

		/*****************************************************************************/
		static SECURITY_STATUS ReadDecrypt(SOCKET Socket, PCredHandle phCreds, CtxtHandle* phContext, char* pbIoBuffer, DWORD cbIoBufferLength) {
			SecBuffer		ExtraBuffer;
			SecBuffer*		pDataBuffer, * pExtraBuffer;

			SECURITY_STATUS	scRet;            // unsigned long cbBuffer;    // Size of the buffer, in bytes
			SecBufferDesc	Message;        // unsigned long BufferType;  // Type of the buffer (below)
			SecBuffer		Buffers[4];    // void    SEC_FAR * pvBuffer;   // Pointer to the buffer

			DWORD			cbIoBuffer, cbData, length;
			char*			buff;
			int i;



			  // Read data from server until done.
			cbIoBuffer = 0;
			scRet = 0;
			while (TRUE) // Read some data.
			{
				if (cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE) // get the data
				{
					cbData = recv(Socket, pbIoBuffer + cbIoBuffer, cbIoBufferLength - cbIoBuffer, 0);
					if (cbData == SOCKET_ERROR) {
						scRet = SEC_E_INTERNAL_ERROR;
						break;
					} else if (cbData == 0) // Server disconnected.
					{
						if (cbIoBuffer) {
							scRet = SEC_E_INTERNAL_ERROR;
							return scRet;
						} else
							break; // All Done
					} else // success
					{
						cbIoBuffer += cbData;
					}
				}


				// Decrypt the received data.
				Buffers[0].pvBuffer = pbIoBuffer;
				Buffers[0].cbBuffer = cbIoBuffer;
				Buffers[0].BufferType = SECBUFFER_DATA;  // Initial Type of the buffer 1
				Buffers[1].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 2
				Buffers[2].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 3
				Buffers[3].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 4

				Message.ulVersion = SECBUFFER_VERSION;    // Version number
				Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
				Message.pBuffers = Buffers;                        // Pointer to array of buffers
				scRet = SChannel->DecryptMessage(phContext, &Message, 0, NULL);
				if (scRet == SEC_I_CONTEXT_EXPIRED) break; // Server signalled end of session
		//      if( scRet == SEC_E_INCOMPLETE_MESSAGE - Input buffer has partial encrypted record, read more
				if (scRet != SEC_E_OK &&
					scRet != SEC_I_RENEGOTIATE &&
					scRet != SEC_I_CONTEXT_EXPIRED) {
					return scRet;
				}



// Locate data and (optional) extra buffers.
				pDataBuffer = NULL;
				pExtraBuffer = NULL;
				for (i = 1; i < 4; i++) {
					if (pDataBuffer  == NULL && Buffers[i].BufferType == SECBUFFER_DATA) pDataBuffer = &Buffers[i];
					if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) pExtraBuffer = &Buffers[i];
				}


				// Display the decrypted data.
				if (pDataBuffer) {
					length = pDataBuffer->cbBuffer;
					if (length) // check if last two chars are CR LF
					{
						buff = (char*)pDataBuffer->pvBuffer;
						if (buff[length-2] == 13 && buff[length-1] == 10) break; // printf("Found CRLF\n");
					}
				}



				// Move any "extra" data to the input buffer.
				if (pExtraBuffer) {
					MoveMemory(pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
					cbIoBuffer = pExtraBuffer->cbBuffer; // printf("cbIoBuffer= %d  \n", cbIoBuffer);
				} else
					cbIoBuffer = 0;


						  // The server wants to perform another handshake sequence.
				if (scRet == SEC_I_RENEGOTIATE) {
					printf("Server requested renegotiate!\n");
					scRet = ClientHandshakeLoop(Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					if (scRet != SEC_E_OK) return scRet;

					if (ExtraBuffer.pvBuffer) // Move any "extra" data to the input buffer.
					{
						MoveMemory(pbIoBuffer, ExtraBuffer.pvBuffer, ExtraBuffer.cbBuffer);
						cbIoBuffer = ExtraBuffer.cbBuffer;
					}
				}
			} // Loop till CRLF is found at the end of the data

			return SEC_E_OK;
		}

		/*****************************************************************************/
		static SECURITY_STATUS SMTPsession(SOCKET Socket, PCredHandle phCreds, CtxtHandle* phContext) {
			SecPkgContext_StreamSizes	Sizes;            // unsigned long cbBuffer;    // Size of the buffer, in bytes
			SECURITY_STATUS				scRet;            // unsigned long BufferType;  // Type of the buffer (below)
			DWORD						cbIoBufferLength, cbData;


			// Read stream encryption properties.
			scRet = SChannel->QueryContextAttributesW(phContext, SECPKG_ATTR_STREAM_SIZES, &Sizes);


			// Create a buffer.
			cbIoBufferLength = Sizes.cbHeader  +  Sizes.cbMaximumMessage  +  Sizes.cbTrailer;
			std::vector<char> pbIoBuffer(cbIoBufferLength);



			// Receive a Response
			scRet = ReadDecrypt(Socket, phCreds, phContext, &pbIoBuffer[0], cbIoBufferLength);
			if (scRet != SEC_E_OK) return scRet;


			// Build the request - must be < maximum message size
			sprintf_s(&pbIoBuffer[0]+Sizes.cbHeader, cbIoBufferLength, "%s", "EHLO \r\n"); // message begins after the header


			// Send a request.
			cbData = EncryptSend(Socket, phContext, &pbIoBuffer[0], Sizes);
			if (cbData == SOCKET_ERROR || cbData == 0) { return SEC_E_INTERNAL_ERROR; }


			// Receive a Response
			scRet = ReadDecrypt(Socket, phCreds, phContext, &pbIoBuffer[0], cbIoBufferLength);
			if (scRet != SEC_E_OK) return scRet;




			// Build the request - must be < maximum message size
			sprintf_s(&pbIoBuffer[0]+Sizes.cbHeader, cbIoBufferLength, "%s", "QUIT \r\n"); // message begins after the header


			// Send a request.
			cbData = EncryptSend(Socket, phContext, &pbIoBuffer[0], Sizes);
			if (cbData == SOCKET_ERROR || cbData == 0) { return SEC_E_INTERNAL_ERROR; }


			// Receive a Response
			scRet = ReadDecrypt(Socket, phCreds, phContext, &pbIoBuffer[0], cbIoBufferLength);
			if (scRet != SEC_E_OK) return scRet;


			return SEC_E_OK;
		}

		/*****************************************************************************/



		class SecureChannel {
			CredHandle ClientCreds;
			CtxtHandle Context;
			PCCERT_CONTEXT RemoteCertContext = NULL;
			SecBuffer  ExtraData;
		};


		void SecureSocket() {
			WSADATA WsaData;
			SOCKET  Socket = INVALID_SOCKET;

			CredHandle hClientCreds;
			CtxtHandle hContext;
			PCCERT_CONTEXT pRemoteCertContext = NULL;
			SecBuffer  ExtraData;

			BOOL fCredsInitialized = FALSE;
			BOOL fContextInitialized = FALSE;

			SECURITY_STATUS Status;

			_Secturity_.Init();
			WSAStartup(0x0101, &WsaData);

			CreateCredentials(pszUser, &hClientCreds);

			ConnectToServer("smtp.gmail.com", 465, &Socket);

			PerformClientHandshake(Socket, &hClientCreds, L"smtp.gmail.com", &hContext, &ExtraData);

			fContextInitialized = TRUE;

			Status = SChannel->QueryContextAttributesW(&hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (PVOID)&pRemoteCertContext);

			// Attempt to validate server certificate.
			Status = VerifyServerCertificate(pRemoteCertContext, L"smtp.gmail.com", 0);

			// Free the server certificate context.
			CertFreeCertificateContext(pRemoteCertContext);
			pRemoteCertContext = NULL;

			// Send Request, recover response. LPSTR pszRequest = "EHLO";
			SMTPsession(Socket, &hClientCreds, &hContext);

			// Send a close_notify alert to the server and close down the connection.
			DisconnectFromServer(Socket, &hClientCreds, &hContext);
			fContextInitialized = FALSE;

			Socket = INVALID_SOCKET;

		cleanup:
			printf("----- Begin Cleanup\n");

				// Free the server certificate context.
			if (pRemoteCertContext) {
				CertFreeCertificateContext(pRemoteCertContext);
				pRemoteCertContext = NULL;
			}

			// Free SSPI context handle.
			if (fContextInitialized) {
				SChannel->DeleteSecurityContext(&hContext);
				fContextInitialized = FALSE;
			}

			// Free SSPI credentials handle.
			if (fCredsInitialized) {
				SChannel->FreeCredentialsHandle(&hClientCreds);
				fCredsInitialized = FALSE;
			}

			// Close socket.
			if (Socket != INVALID_SOCKET) closesocket(Socket);

			// Shutdown WinSock subsystem.
			WSACleanup();

			// Close "MY" certificate store.
			if (hMyCertStore) CertCloseStore(hMyCertStore, 0);

			printf("----- All Done ----- \n");
		}
	}
}