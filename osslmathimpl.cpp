
#include "osslmathimpl.hpp"

namespace Dsrp
{
    OsslMathImpl::OsslMathImpl(HashInterface &hashInterface, Ng ngVal) :
		MathInterface(hashInterface, ngVal),
        N(BN_new()),
        g(BN_new()),
        k(BN_new()),
        ctx(BN_CTX_new())
    {
        // checkNg()????
        // set that we set Ng as bool
        
        bytes NN = ngVal.getN(); // could also use ng from base class
        bytes gg = ngVal.getg();
        
        bytes2bignum(NN, N);
        bytes2bignum(gg, g);
        
        bytes both = NN;
        both.insert(both.end(), gg.begin(), gg.end());
        bytes kk = hash.hash(both);
        bytes2bignum(kk, k);
    }
            
    OsslMathImpl::~OsslMathImpl()
    {
        BN_free(N);
        BN_free(g);
        BN_free(k);
        BN_CTX_free(ctx);
    }

    // A = g^a mod N
    void OsslMathImpl::calculateA(const bytes &aa, bytes &A_out)
    {
		checkNg(); // will throw on error
        BIGNUM *a = BN_new();
        BIGNUM *A = BN_new();
        BIGNUM *tmp1 = BN_new();
                
        bytes2bignum(aa, a);
        BN_mod_exp(A, g, a, N, ctx);
                
        bignum2bytes(A, A_out);
                
        BN_free(a);
        BN_free(A);
        BN_free(tmp1);
    }
            
    // u = H(A || B)
    // x = H(salt || H(username || ":" || password)
    // S = (B - k*(g^x)) ^ (a + ux)
    // K = H(S)
	void OsslMathImpl::clientChallenge(const bytes &salt, const bytes &aa, const bytes &AA, const bytes &BB, const bytes &username, const bytes &password, bytes &K_out, bytes &M1_out)
	{   
		checkNg(); // will throw on error
		BIGNUM *B = BN_new();
		BIGNUM *x = BN_new();
		BIGNUM *a = BN_new();
		BIGNUM *u = BN_new();
		BIGNUM *tmp1 = BN_new();
		BIGNUM *tmp2 = BN_new();
		BIGNUM *tmp3 = BN_new();
		BIGNUM *S = BN_new();
		
		// Calculate u = H(A || B)
		bytes AandB = AA;
		AandB.insert(AandB.end(), BB.begin(), BB.end());
		bytes uu = hash.hash(AandB);
		bytes2bignum(uu, u);
		bytes2bignum(BB, B);
		
		// Calculate x = HASH(salt || HASH(username || ":" || password)
		unsigned char colon = ':';
		bytes sup = salt;
		sup.insert(sup.end(), username.begin(), username.end());
		sup.insert(sup.end(), &colon, (&colon) + 1);
		sup.insert(sup.end(), password.begin(), password.end());
		bytes hashTmp = hash.hash(sup);
		hashTmp.insert(sup.begin(), salt.begin(), salt.end());
		sup = hash.hash(hashTmp);
		bytes2bignum(sup, x);
		
		//Calculate S
		// SRP-6a safety check
		if (!BN_is_zero(B) && !BN_is_zero(u))
		{
			bytes2bignum(aa, a);
			BN_mod_mul(tmp1, u, x, N, ctx);    /* tmp1 = ux */
			BN_mod_add(tmp2, a, tmp1, N, ctx); /* tmp2 = a+ux  */
			BN_mod_exp(tmp1, g, x, N, ctx);    /* tmp1 = (g^x)%N */
			BN_mod_mul(tmp3, k, tmp1, N, ctx); /* tmp3 = k*((g^x)%N)       */
			BN_sub(tmp1, B, tmp3);             /* tmp1 = (B-k*((g^x)%N) */
			BN_mod_exp(S, tmp1, tmp2, N, ctx); /* S = ((B-k*((g^x)%N)^(a+ux)%N) */
			
			// Calculate K
			bytes SS;
			bignum2bytes(S, SS);
			K_out = hash.hash(SS);
		
			// Calculate M1
			M1_out = calculateM1(username, salt, AA, BB, K_out);
		}
		else
		{
			BN_free(B);
			BN_free(x);
			BN_free(a);
			BN_free(u);
			BN_free(tmp1);
			BN_free(tmp2);
			BN_free(tmp3);
			BN_free(S);
			
			throw DsrpException("Dsrp:Authentification failed:B or u is zero.");
		}
		
		BN_free(B);
		BN_free(x);
		BN_free(a);
		BN_free(u);
		BN_free(tmp1);
		BN_free(tmp2);
		BN_free(tmp3);
		BN_free(S);
			
	}
	
	void OsslMathImpl::serverChallange(const bytes &username, const bytes &salt, const bytes &verificator, const bytes &AA, const bytes &bb, bytes &B_out, bytes &M1_out, bytes &M2_out, bytes &K_out)
	{
		checkNg(); // will throw on error
		
		bytes SS;
		
		BIGNUM *A = BN_new();
		BIGNUM *b = BN_new();
		BIGNUM *B = BN_new();
		BIGNUM *v = BN_new();
		BIGNUM *S = BN_new();
		BIGNUM *u = BN_new();
		BIGNUM *tmp1 = BN_new();
		BIGNUM *tmp2 = BN_new();

		bytes2bignum(bb, b);
		bytes2bignum(verificator, v);
		
		// there is neccessary to add the SRP6a security check
		// SRP-6a safety check
		
		BN_mod(tmp1, A, N, ctx);
		
		// I added the v != 0 check
		if (!BN_is_zero(tmp1) && !BN_is_zero(v))
		{
		
			// Calculate B = k*v + g^b
			BN_mod_mul(tmp1, k, v, N, ctx);
			BN_mod_exp(tmp2, g, b, N, ctx);
			BN_mod_add(B, tmp1, tmp2, N, ctx);
			bignum2bytes(B, B_out);
		
			// Calculate u = H(A || B)
			bytes AABB = AA;
			bytes BB;
			bignum2bytes(B, BB);
			
			AABB.insert(AABB.end(), BB.begin(), BB.end());
			bytes uu = hash.hash(AABB);
			bytes2bignum(uu, u);
		
			// Calculate S = (A *(v^u)) ^ b
			BN_mod_exp(tmp1, v, u, N, ctx);
			BN_mod_mul(tmp2, A, tmp1, N, ctx);
			BN_mod_exp(S, tmp2, b, N, ctx);
			bignum2bytes(S, SS);
			K_out = hash.hash(SS);
			
			// Calculate M1 = H(H(N) XOR H(g) || H (s || A || B || K))
			M1_out = calculateM1(username, salt, AA, BB, K_out);
			
			// Calculate M2 = H(A || M || K)
			bytes toHashM2 = AA;
			toHashM2.insert(toHashM2.end(), M1_out.begin(), M1_out.end());
			toHashM2.insert(toHashM2.end(), K_out.begin(), K_out.end());
			M2_out = hash.hash(toHashM2);
		}
		else
		{
			BN_free(A);
			BN_free(b);
			BN_free(B);
			BN_free(v);
			BN_free(S);
			BN_free(u);
			BN_free(tmp1);
			BN_free(tmp2);
			
			throw DsrpException("Dsrp:Authentification failed:serverChalange: invalid parameters");
		}
		
		BN_free(A);
		BN_free(b);
		BN_free(B);
		BN_free(v);
		BN_free(S);
		BN_free(u);
		BN_free(tmp1);
		BN_free(tmp2);              
	}
	
	// M = H(H(N) XOR H(g) | H(username) | s | A | B | K)
	bytes OsslMathImpl::calculateM1(const bytes &username, const bytes &s, const bytes &A, const bytes &B, const bytes &K)
	{   
		bytes NN;
		bytes gg;
		
		bignum2bytes(N, NN);
		bignum2bytes(g, gg);
		
		bytes H_N = hash.hash(NN); 
		bytes H_g = hash.hash(gg);
		
		bytes H_username = hash.hash(username);
		bytes H_xor;
		
		H_xor.resize(hash.outputLen());
		for (int i = 0; i < hash.outputLen(); i++ ) H_xor[i] = H_N[i] ^ H_g[i];
	
		bytes toHash = H_xor;
		toHash.insert(toHash.end(), H_username.begin(), H_username.end());
		toHash.insert(toHash.end(), s.begin(), s.end());
		toHash.insert(toHash.end(), A.begin(), A.end());
		toHash.insert(toHash.end(), B.begin(), B.end());
		toHash.insert(toHash.end(), K.begin(), K.end());
		
		return hash.hash(toHash);
	}
	
	void OsslMathImpl::checkNg()
	{
		if (BN_is_zero(N)) throw DsrpException("OsslMathImpl: N was not set");
		if (BN_is_zero(g)) throw DsrpException("OsslMathImpl: g was not set");
		if (BN_is_zero(k)) throw DsrpException("OsslMathImpl: k was not set");
	}
    
}