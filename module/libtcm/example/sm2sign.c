#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <dlfcn.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


#include "data_type.h"
#include "alloc.h"
#include "memfunc.h"
#include "basefunc.h"
#include "struct_deal.h"
#include "crypto_func.h"
#include "channel.h"
#include "memdb.h"
#include "message.h"
#include "connector.h"
#include "ex_module.h"
#include "tcm_constants.h"
#include "app_struct.h"
#include "pik_struct.h"
#include "sm4.h"
#include "tcmfunc.h"
#include "vtcm_alg.h"

int main(int argc,char **argv)
{

    int ret;
   
    UINT32 handle;
    int PcrLength;
    BYTE * PcrValue;
    BYTE *Buf;
    BYTE *SignBuf;
    int  Buflen;
    int SignLen;
    UINT32 authHandle;
    UINT32 keyHandle;
    UINT32 keyAuthHandle;

    BYTE inDigest[DIGEST_SIZE];
    BYTE outDigest[DIGEST_SIZE];
    int i,j;

    struct timeval start, end;
    int crypttime,decrypttime;
    float encrypt_speed, decrypt_speed;
    int  encrypt_len=DIGEST_SIZE*8;

    ret=_TSMD_Init();

    ret= TCM_LibInit(); 

    Buf=malloc(DIGEST_SIZE*128);
    if(Buf==NULL)
	return -ENOMEM;
    SignBuf=Buf+DIGEST_SIZE*96;

    TCM_PUBKEY * pubkey;
    TCM_KEY * tcmkey;
    Memset(Buf,'A',DIGEST_SIZE*16);

    tcmkey=malloc(sizeof(*tcmkey));
    if(tcmkey==NULL)
	return -EINVAL;
    // data sign
    ret = TCM_ExLoadTcmKey(tcmkey, "sm2sign.key");
    if(ret!=0)
    {
	printf("ExLoadTcmKey error!\n");
	return -EINVAL;	
    }

    ret=TCM_APCreate(TCM_ET_SMK, NULL, "sss", &authHandle);
    printf("authHandle is : %x\n",authHandle);
    if(ret<0)
    {
	printf("TCM_APCreate failed!\n");
	return -EINVAL;	
    }	
    ret=TCM_LoadKey(0x40000000,authHandle,tcmkey,&keyHandle);
    if(ret<0)
    {
	printf("TCM_LoadKey failed!\n");
	return -EINVAL;	
    }	

    ret=TCM_APCreate(TCM_ET_KEYHANDLE, keyHandle, "kkk", &keyAuthHandle);
    if(ret<0)
    {
	printf("TCM_APCreate %dfailed!\n",12);
	return -EINVAL;	
    }	
    printf("keyAuthHandle is : %x\n",keyAuthHandle);

    ret=TCM_SM2Sign(keyHandle,keyAuthHandle,SignBuf,&SignLen,Buf,DIGEST_SIZE*16);

    ret=TCM_APTerminate(authHandle);
    if(ret<0)
    {
	printf("TCM_APTerminate %x failed!\n",authHandle);
	return -EINVAL;	
    }	
    ret=TCM_APTerminate(keyAuthHandle);
    if(ret<0)
    {
	printf("TCM_APTerminate %x failed!\n",keyAuthHandle);
	return -EINVAL;	
    }	
    ret=TCM_EvictKey(keyHandle);
    if(ret<0)
    {
	printf("TCM_EvictKey %x failed!\n",keyHandle);
	return -EINVAL;	
    }	

    // SM2 Verify

    pubkey=malloc(sizeof(*pubkey));
    if(pubkey==NULL)
	return -EINVAL;
    ret = TCM_ExLoadTcmPubKey(pubkey, "sm2signpub.key");
    if(ret!=0)
    {
	printf("ExLoadTcmPubKey error!\n");
	return -EINVAL;	
    }

    ret=TCM_ExSM2Verify(pubkey,SignBuf,SignLen,Buf,DIGEST_SIZE*16);
    printf("TCM_ExSM2Verify verify result is %d!\n",ret);

    print_bin_data(SignBuf,SignLen,16);	
    return ret;	

}

