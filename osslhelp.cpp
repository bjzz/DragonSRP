

#include <stdlib.h>
#include "osslhelp.hpp"

namespace Dsrp
{
	void bytes2bignum(const bytes &in, BIGNUM *out)
	{
		unsigned char *bytesIn = (unsigned char*) malloc(sizeof(unsigned char) * in.size());
		// should check if bytesIn==NULL !!!!!
		copy(in.begin(), in.end(), bytesIn);
		BN_bin2bn(bytesIn, in.size(), out);
		free(bytesIn);
	}
	
	void bignum2bytes(BIGNUM *in, bytes& out)
	{
		int len = BN_num_bytes(in);
		unsigned char *bytes = (unsigned char *) malloc(sizeof(unsigned char) * len);
		copy(bytes, bytes + len, out.begin());
		free(bytes);
	}
}
