#include "stdafx.h"

#include "MPInclude.h"

#include "NICUtil.h"

MP_NIC_INFO* MP_NIC_INFO_New()
{
	MP_NIC_INFO* item = NULL;

	item = (MP_NIC_INFO*)malloc(sizeof(MP_NIC_INFO));
	if( item != NULL ){
		item->name = NULL;
		item->ipv4 = NULL;
		//item->ipv6 = NULL;
		item->macAddress = NULL;
	}
	return item;
}

void MP_NIC_INFO_Delete(void* item)
{
	MP_NIC_INFO* info = (MP_NIC_INFO*)item;
	if( info == NULL ){
		return ;
	}

	if( info->name != NULL ){
		free(info->name);
	}
	if( info->ipv4 != NULL ){
		free(info->ipv4);
	}
	//if( info->ipv6 != NULL ){
	//	free(info->ipv6);
	//}
	if( info->macAddress != NULL ){
		free(info->macAddress);
	}
	free(item);
}

MP_NIC_INFO* MP_NIC_INFO_Clone(MP_NIC_INFO* src)
{
	MP_NIC_INFO* dest = MP_NIC_INFO_New();
	if( dest != NULL ){
		if( src->name != NULL ){
			dest->name = (char*)malloc(strlen(src->name)+1);
#ifdef _WIN32
			strcpy_s(dest->name, strlen(src->name)+1, src->name);
#else
			strcpy(dest->name, src->name);
#endif
		}
		if( src->ipv4 != NULL ){
			dest->ipv4 = (char*)malloc(strlen(src->ipv4)+1);
#ifdef _WIN32
			strcpy_s(dest->ipv4, strlen(src->ipv4)+1, src->ipv4);
#else
			strcpy(dest->ipv4, src->ipv4);
#endif
		}
		if( src->macAddress != NULL ){
			dest->macAddress = (char*)malloc(strlen(src->macAddress)+1);
#ifdef _WIN32
			strcpy_s(dest->macAddress, strlen(src->macAddress)+1, src->macAddress);
#else
			strcpy(dest->macAddress, src->macAddress);
#endif
		}
	}

	return dest;
}



//NICの一覧情報を取得する
//戻り値：エラーでNULL、成功時検索結果のリスト、キー：char* のname、要素：MP_NIC_INFO*　不必要になったら上位層で開放すること
SORT_LIST_HANDLE MP_NIC_GetNICInfo(void)
{
	int ret = 0;
	MP_NIC_INFO* item = NULL;
	SORT_LIST_HANDLE listHandle = NULL;

#if defined(_WIN32)
	ULONG len = 0;
	WSADATA data;
	PIP_ADAPTER_ADDRESSES adpts = NULL;
	DWORD res = 0;
	int i=0;

	listHandle = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, MP_NIC_INFO_Delete);

	ZeroMemory(&data, sizeof(data));
	WSAStartup(MAKEWORD(2, 2), &data);

	res = GetAdaptersAddresses(AF_INET, 0, 0, 0, &len);
	if(res != ERROR_BUFFER_OVERFLOW){
		ret = -1;
		goto Err_End;
	}

	adpts = (PIP_ADAPTER_ADDRESSES)malloc(sizeof(BYTE)*len);
	if(adpts == 0){
		ret = -2;
		goto Err_End;
	}

	res = GetAdaptersAddresses(AF_INET, 0, 0, adpts, &len);
	if(res != ERROR_SUCCESS){
		ret = -3;
		goto Err_End;
	}

	for(PIP_ADAPTER_ADDRESSES adpt = adpts; adpt; adpt = adpt->Next){
		if(adpt->PhysicalAddressLength == 0){
			continue;
		}
		if(adpt->IfType == IF_TYPE_SOFTWARE_LOOPBACK){
			continue;
		}

		for(PIP_ADAPTER_UNICAST_ADDRESS uni = adpt->FirstUnicastAddress; uni; uni = uni->Next){
			if(~(uni->Flags) & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE){
				continue;
			}
			if(uni->Flags & IP_ADAPTER_ADDRESS_TRANSIENT){
				continue;
			}
			char host[NI_MAXHOST + 1] = {'\0'};
			if(getnameinfo(uni->Address.lpSockaddr, uni->Address.iSockaddrLength, host, sizeof(host), 0, 0, NI_NUMERICHOST) == 0){
				item = MP_NIC_INFO_New();

				len = WideCharToMultiByte( 932, 0, adpt->Description, -1, NULL, 0, NULL, NULL );
				item->name = (char*)malloc(len+1);
				if( item->name == NULL ){
					MP_NIC_INFO_Delete(item);
					ret = -4;
					goto Err_End;
				}
				ZeroMemory(item->name, sizeof(char)*(len+1));
				WideCharToMultiByte( 932, 0, adpt->Description, -1, item->name, len, NULL, NULL );

				item->ipv4 = (char*)malloc(strlen(host)+1);
				if( item->ipv4 == NULL ){
					MP_NIC_INFO_Delete(item);
					ret = -5;
					goto Err_End;
				}
				strcpy_s(item->ipv4, strlen(host)+1, host);

				item->macAddress = (char*)malloc(adpt->PhysicalAddressLength*3);
				if( item->macAddress == NULL ){
					MP_NIC_INFO_Delete(item);
					ret = -6;
					goto Err_End;
				}
				ZeroMemory(item->macAddress, adpt->PhysicalAddressLength*3);
				sprintf_s(item->macAddress, adpt->PhysicalAddressLength*3, "%02X-%02X-%02X-%02X-%02X-%02X",
					adpt->PhysicalAddress[0],
					adpt->PhysicalAddress[1],
					adpt->PhysicalAddress[2],
					adpt->PhysicalAddress[3],
					adpt->PhysicalAddress[4],
					adpt->PhysicalAddress[5]);
				memcpy(item->macAddressValue, adpt->PhysicalAddress, 6);

				if( SORT_LIST_AddItem(listHandle, item->name, item) < 0 ){
					MP_NIC_INFO_Delete(item);
				}
			}
		}
	}

Err_End:
	if( adpts != NULL ){
		free(adpts);
	}
	WSACleanup();

#elif defined(__APPLE__)
	struct ifaddrs *ifa_list = NULL;
	struct ifaddrs *ifa = NULL;
	char addrstr[256] = "";
	int count = 0;
	unsigned char *macAddr;
	struct sockaddr_dl* dl = NULL;

	if( getifaddrs(&ifa_list) < 0 ){
		ret = -1;
		goto Err_End;
	}

	listHandle = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, MP_NIC_INFO_Delete);

	for(ifa = ifa_list; ifa != NULL; ifa=ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET ) {
			item = SORT_LIST_FindItem(listHandle, ifa->ifa_name);

			item = MP_NIC_INFO_New();

			item->name = (char*)malloc(sizeof(ifa->ifa_name)+1);
			if( item->name == NULL ){
				MP_NIC_INFO_Delete(item);
				ret = -2;
				goto Err_End;
			}
			strcpy(item->name, ifa->ifa_name );

			inet_ntop(AF_INET, &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, addrstr, sizeof(addrstr));
			item->ipv4 = (char*)malloc(strlen(addrstr)+1);
			if( item->ipv4 == NULL ){
				MP_NIC_INFO_Delete(item);
				ret = -3;
				goto Err_End;
			}
			strcpy(item->ipv4, addrstr);

			if( SORT_LIST_AddItem(listHandle, item->name, item) < 0 ){
				MP_NIC_INFO_Delete(item);
			}
		}
	}
	for(ifa = ifa_list; ifa != NULL; ifa=ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_LINK ) {
			item = SORT_LIST_FindItem(listHandle, ifa->ifa_name);
			if( item != NULL ){
				dl = (struct sockaddr_dl*)ifa->ifa_addr;
				macAddr = dl->sdl_data+dl->sdl_nlen;

				item->macAddress = (char*)malloc(18);
				if( item->macAddress == NULL ){
					MP_NIC_INFO_Delete(item);
					ret = -4;
					goto Err_End;
				}
				memset(item->macAddress, 0, 18);
				sprintf(item->macAddress, "%02X-%02X-%02X-%02X-%02X-%02X",
					macAddr[0],
					macAddr[1],
					macAddr[2],
					macAddr[3],
					macAddr[4],
					macAddr[5]);
				memcpy(item->macAddressValue, macAddr, 6);

			}
		}
	}

Err_End:
	if( ifa_list != NULL ){
		freeifaddrs(ifa_list);
	}
#else
	//Linux系
	struct ifreq ifr[16];
	struct ifreq ifrMAC;
	struct ifconf ifc;
	int fd = 0;
	int count = 0;
	int i = 0;
	int len = 0;

	listHandle = SORT_LIST_CreateHandle(0, SORT_LIST_COMPDEF_strcmp, NULL, MP_NIC_INFO_Delete);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if( fd == 0 ){
		ret = -1;
		goto Err_End;
	}
	ifc.ifc_len = sizeof(ifr);
	ifc.ifc_ifcu.ifcu_buf = (void *)ifr;

	if(ioctl(fd, SIOCGIFCONF, &ifc) < 0 ){
		ret = -2;
		goto Err_End;
	}
	count = ifc.ifc_len / sizeof(struct ifreq);
	for (i=0; i<count; i++) {
		item = MP_NIC_INFO_New();

		item->name = (char*)malloc(sizeof(ifr[i].ifr_name)+1);
		if( item->name == NULL ){
			MP_NIC_INFO_Delete(item);
			ret = -3;
			goto Err_End;
		}
		strcpy(item->name, ifr[i].ifr_name );

		len = strlen(inet_ntoa(((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr));
		item->ipv4 = (char*)malloc(len+1);
		if( item->ipv4 == NULL ){
			MP_NIC_INFO_Delete(item);
			ret = -4;
			goto Err_End;
		}
		strcpy(item->ipv4, inet_ntoa(((struct sockaddr_in *)&ifr[i].ifr_addr)->sin_addr));
		
		ifrMAC.ifr_addr.sa_family = AF_INET;
		strcpy(ifrMAC.ifr_name, ifr[i].ifr_name);

		if( ioctl(fd, SIOCGIFHWADDR, &ifrMAC) < 0 ){
			MP_NIC_INFO_Delete(item);
			continue;
		}
		item->macAddress = (char*)malloc(18);
		if( item->macAddress == NULL ){
			MP_NIC_INFO_Delete(item);
			ret = -5;
			goto Err_End;
		}
		memset(item->macAddress, 0, 18);
		sprintf(item->macAddress, "%02X-%02X-%02X-%02X-%02X-%02X",
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[0],
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[1],
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[2],
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[3],
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[4],
			(unsigned char)ifrMAC.ifr_hwaddr.sa_data[5]);
		memcpy(item->macAddressValue, ifrMAC.ifr_hwaddr.sa_data, 6);
		if( SORT_LIST_AddItem(listHandle, item->name, item) < 0 ){
			MP_NIC_INFO_Delete(item);
		}
	}
Err_End:
	if( fd > 0 ){
		close(fd);
	}

#endif
	if( ret < 0 ){
		if( listHandle != NULL ){
			SORT_LIST_CloseHandle(&listHandle);
		}
	}
	return listHandle;
}



