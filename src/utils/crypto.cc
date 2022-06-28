#include "crypto.h"
#include "file_util.h"
#include <iostream>

void CryptoEntry::baseInit(const char *pers) {
	int ret;
	mbedtls_entropy_init( &entropy );
    mbedtls_pk_init( &key );
    mbedtls_ctr_drbg_init( &ctr_drbg );

    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
      return;
    }
    allocated = true;
}
void CryptoEntry::writeFile(std::string path) {
	if(mode != ModeBoth)
		return;
	   mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;
    mbedtls_mpi_init( &N );
     mbedtls_mpi_init( &P );
      mbedtls_mpi_init( &Q );
    mbedtls_mpi_init( &D );
     mbedtls_mpi_init( &E );
      mbedtls_mpi_init( &DP );
    mbedtls_mpi_init( &DQ );
     mbedtls_mpi_init( &QP );

    mbedtls_rsa_context *rsa = mbedtls_pk_rsa( key );

    if( ( ret = mbedtls_rsa_export    ( rsa, &N, &P, &Q, &D, &E ) ) != 0 ||
        ( ret = mbedtls_rsa_export_crt( rsa, &DP, &DQ, &QP ) )      != 0 )
    {
       return;
    }
    mbedtls_mpi_write_file( "N:  ",  &N,  16, NULL );
    mbedtls_mpi_write_file( "E:  ",  &E,  16, NULL );
    mbedtls_mpi_write_file( "D:  ",  &D,  16, NULL );
    mbedtls_mpi_write_file( "P:  ",  &P,  16, NULL );
    mbedtls_mpi_write_file( "Q:  ",  &Q,  16, NULL );
    mbedtls_mpi_write_file( "DP: ",  &DP, 16, NULL );
    mbedtls_mpi_write_file( "DQ:  ", &DQ, 16, NULL );
    mbedtls_mpi_write_file( "QP:  ", &QP, 16, NULL );

    unsigned char output_buf[16000];
    memset(output_buf, 0, 16000);
    size_t len = 0;
    mbedtls_pk_write_key_pem( &key, output_buf, 16000 );
    len = strlen( (char *) output_buf );

    std::string dataStr(output_buf, output_buf+len);
    FileUtils::string_to_file(path, dataStr);
    mbedtls_mpi_free( &N );
     mbedtls_mpi_free( &P );
      mbedtls_mpi_free( &Q );
    mbedtls_mpi_free( &D ); 
    mbedtls_mpi_free( &E ); 
    mbedtls_mpi_free( &DP );
    mbedtls_mpi_free( &DQ ); 
    mbedtls_mpi_free( &QP );

}
std::string CryptoEntry::getPublicKey() {
	if(!allocated || mode == ModeDefault)
		return "";
	unsigned char pubKeyPem[1000];
	memset(pubKeyPem, 0, sizeof(pubKeyPem));
	if(mbedtls_pk_write_pubkey_pem(&key, pubKeyPem, sizeof(pubKeyPem)) != 0){
  		return "";
  }
  std::string out(pubKeyPem, pubKeyPem+strlen((char *)pubKeyPem));
  return out;
}
void CryptoEntry::initFromPublicKey(std::string input) {
	if(allocated)
		return;
	 baseInit();
	 int res = mbedtls_pk_parse_public_key(&key, (const unsigned char *)input.c_str(), input.size()+1);
	 if(res != 0 ) {
	    char error_buf[200];
		mbedtls_strerror( res, error_buf, 200 );
	    printf("Last error was: -0x%04x - %s\n\n", (unsigned int) -res, error_buf );
	      
	 }
	 mode = ModePublic;
}
void CryptoEntry::initBoth(std::string path) {
	if(allocated)
		return;
	baseInit();
	int res =
	 mbedtls_pk_parse_keyfile( &key, path.c_str(), "",
                    mbedtls_ctr_drbg_random, &ctr_drbg );
    if(res != 0) {
    		        char error_buf[200];

    	mbedtls_strerror( res, error_buf, 200 );
		    printf("Last error was: -0x%04x - %s\n\n", (unsigned int) -res, error_buf );
    }
	mode = ModeBoth;
}
void CryptoEntry::encrypt(std::string content, std::vector<uint8_t>& out) {
	if(!allocated || mode == ModeDefault)
		return;
	out.clear();


	out.resize(15000);
	size_t outSize;
	int res = mbedtls_pk_encrypt(&key, (const unsigned char *)content.c_str(), content.size(),
	 out.data(), &outSize, out.size(), mbedtls_ctr_drbg_random, &ctr_drbg);
      if(res != 0) {
      		        char error_buf[200];

	        	mbedtls_strerror( res, error_buf, 200 );
       		    printf("Last error was: -0x%04x - %s\n\n", (unsigned int) -res, error_buf );
	        }
	auto remaining = out.size()-outSize;
	out.erase(out.begin() + outSize, out.begin() + outSize+remaining);
}
void CryptoEntry::decrypt(std::string ciphertext, std::vector<uint8_t>& out) {
	if(!allocated || mode != ModeBoth)
		return;
	out.clear();

	out.resize(15000);
	size_t outSize = 0;
	mbedtls_pk_decrypt(&key,  (const unsigned char *)ciphertext.c_str(), ciphertext.size(),
	 out.data(), &outSize, out.size(), mbedtls_ctr_drbg_random, &ctr_drbg);
	auto remaining = out.size()-outSize;
	out.erase(out.begin() + outSize, out.end());
}
void CryptoEntry::initWithNewPair() {
	if(allocated)
		return;
	    int ret;
     
     char buf[1024];


     baseInit();

    if( ( ret = mbedtls_pk_setup( &key,
            mbedtls_pk_info_from_type( (mbedtls_pk_type_t) MBEDTLS_PK_RSA ) ) ) != 0 )
    {
       return;
    }

	 ret = mbedtls_rsa_gen_key( mbedtls_pk_rsa( key ), mbedtls_ctr_drbg_random, &ctr_drbg,
                               DFL_RSA_KEYSIZE, 65537 );
    if( ret != 0 )
    {
       return;
    }
    std::cout << "generated keys\n";
    mode = ModeBoth;
}