/*
   FALCON - The Falcon Programming Language.
   FILE: hash_ext.cpp

   Provides multiple hashing algorithms
   Main module file, providing the module object to
   the Falcon engine.
   -------------------------------------------------------------------
   Author: Maximilian Malek
   Begin: Thu, 25 Mar 2010 02:46:10 +0100

   -------------------------------------------------------------------
   (C) Copyright 2010: The above AUTHOR

         Licensed under the Falcon Programming Language License,
      Version 1.1 (the "License"); you may not use this file
      except in compliance with the License. You may obtain
      a copy of the License at

         http://www.falconpl.org/?page_id=license_1_1

      Unless required by applicable law or agreed to in writing,
      software distributed under the License is distributed on
      an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
      KIND, either express or implied. See the License for the
      specific language governing permissions and limitations
      under the License.

*/

/** \file
   Main module file, providing the module object to
   the Falcon engine.
*/

#include <falcon/module.h>
#include <falcon/symbol.h>
#include "hash_mod.h"
#include "hash_ext.h"
#include "hash_srv.h"
#include "hash_st.h"

#include "../include/version.h"

#include "hash_ext.inl"

/*#
    @module feather_hash hash
    @brief Various hash and checksum functions

    This module provides a selection of the most widely used checksum/hash algorithms:

    CRC32, Adler32, SHA-1, SHA-224, SHA-256, SHA-384, SHA-512, MD2, MD4, MD5, Whirlpool, Tiger,
    RIPEMD128, RIPEMD160, RIPEMD256, RIPEMD320

    @beginmodule feather_hash
*/

/*#
    @group checksums Checksums
    @brief Classes providing checksum functions

    This group of classes provides simple checksum functions to verify integrity of arbitrary data.
    They are NOT meant for use in cryptographic algorithms or @b safe data verification!
*/

/*#
    @group weak_hashes Weak hashes
    @brief Classes providing weak / deprecated hashes

    This group of classes provides hashes that are stronger (and longer) then checksums,
    but not recommended for serious cryptographic purposes (MD2, MD4, MD5 and partly SHA1 can be considered broken).
*/

/*#
    @group strong_hashes Strong hashes
    @brief Classes providing strong hashes, suitable for cryptography

    Hashes in this group are cryptographically strong and can be used for @b secure verification of data.
*/

/*#
    @class HashBase
    @brief Base class for each hash algorithm, specialized for overloading.

    The HashBase class provides a set of shared interfaces that are syntactically equivalent for each specialized hash.

    Hashes are generated by creating an instance of a specialized class and putting data into it.
    When the result is requested, a hash is finalized, which means that no more data can be added;
    any attempts to do so will raise an exception.

    Basic usage example:
    @code
        crc = CRC32()
        crc.update("abc")
        > crc // prints "352441c2"
    @endcode

    @note Instantiating HashBase directly and calling any method will raise an error.

    @section hashbase_overload Overloading HashBase

    To easily implement other hash algorithms in native falcon code, HashBase can be overloaded.
    For simplicity, only 2 methods have to be overloaded, and 2 new methods have to be added:
    @code
        class MyHash from HashBase
            state = nil // internal state
            outp = nil
            function bytes(): return 12       // must be overloaded and return a constant integer > 0
            function toMemBuf(): return self.outp  // must be overloaded and return a MemBuf with wordSize 1 and length equal to bytes()
            function process(buf)             // must be declared, as it is invoked by the module on update() calls
                // *mangle MemBuf and update state*
            end
            function finalize()               // must be declared, as it is invoked by the module to produce the actual digest
                // *transform state and assign result MemBuf(1, bytes()) to outp*
            end
        end
    @endcode

    How this works:
    - @b bytes() is internally invoked by bits() (once, the returned integer is cached by the module)
    - @b process() is invoked by update() and updateInt(), beeing passed a MemBuf with word size 1
    - @b toMemBuf() is invoked by toString() and toInt()
    - @b finalize() is called ONCE before toMemBuf() and is intended to do process remaining buffers, and produce the actual digest.
         Does not have to be called manually.
    @note You are strongly advised NOT to overload any other methods except the four above, unless you REALLY know what you're doing.

    Advantages of doing it this way:
    - It is not necessary to implement update() in native falcon code.
    - All value endian conversions, type mangling, and error checking is done by the module, so focus can be set on the algorithm itself.
    - The values returned by bytes(), toMemBuf() and toInt() are cached by the module, means less calls, less time.
    - The module ensures that finalize() is called only once, no explicit checking required.

*/

/*#
    @class CRC32
    @from HashBase
    @ingroup checksums
    @brief Calculates a 32 bits long CRC32 checksum
*/

/*#
    @class Adler32
    @from HashBase
    @ingroup checksums
    @brief Calculates a 32 bits long Adler32 checksum
*/

/*#
    @class SHA1Hash
    @from HashBase
    @ingroup weak_hashes
    @brief Calculates a 160 bits long SHA-1 hash
*/

/*#
    @class MD2Hash
    @from HashBase
    @ingroup weak_hashes
    @brief Calculates a 128 bits long MD2 (Message Digest 2) hash
*/

/*#
    @class MD4Hash
    @from HashBase
    @ingroup weak_hashes
    @brief Calculates a 128 bits long MD4 (Message Digest 4) hash
*/

/*#
    @class MD5Hash
    @from HashBase
    @ingroup weak_hashes
    @brief Calculates a 128 bits long MD5 (Message Digest 5) hash
*/

/*#
    @class SHA224Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 224 bits long SHA-224 hash (SHA-2 family)
*/

/*#
    @class SHA256Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 256 bits long SHA-256 hash (SHA-2 family)
*/

/*#
    @class SHA384Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 384 bits long SHA-384 hash (SHA-2 family)
*/

/*#
    @class SHA512Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 512 bits long SHA-512 hash (SHA-2 family)
*/

/*#
    @class TigerHash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 192 bits long Tiger hash
*/

/*#
    @class WhirlpoolHash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 512 bits long Whirlpool hash
*/

/*#
    @class RIPEMD128Hash
    @from HashBase
    @ingroup weak_hashes
    @brief Calculates a 128 bits long RIPEMD-128 hash (RIPEMD family)
*/

/*#
    @class RIPEMD160Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 160 bits long RIPEMD-160 hash (RIPEMD family)
*/

/*#
    @class RIPEMD256Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 256 bits long RIPEMD-256 hash (RIPEMD family)
*/

/*#
    @class RIPEMD320Hash
    @from HashBase
    @ingroup strong_hashes
    @brief Calculates a 320 bits long RIPEMD-320 hash (RIPEMD family)
*/

template <class HASH> Falcon::Symbol *SimpleRegisterHash(Falcon::Module *self, const char *name,
                                                         Falcon::InheritDef *parent)
{
    Falcon::Symbol *cls = self->addClass(name, Falcon::Ext::Hash_init<HASH>);
    self->addClassMethod(cls, "update", Falcon::Ext::Hash_update<HASH>);
    self->addClassMethod(cls, "updateInt",   Falcon::Ext::Hash_updateInt<HASH>).asSymbol()->
        addParam("num")->addParam("bytes");
    self->addClassMethod(cls, "isFinalized",Falcon::Ext::Hash_isFinalized<HASH>);
    self->addClassMethod(cls, "bytes", Falcon::Ext::Hash_bytes<HASH>);
    self->addClassMethod(cls, "bits", Falcon::Ext::Hash_bits<HASH>);
    self->addClassMethod(cls, "toMemBuf",  Falcon::Ext::Hash_toMemBuf<HASH>);
    self->addClassMethod(cls, "toString",   Falcon::Ext::Hash_toString<HASH>);
    self->addClassMethod(cls, "toInt", Falcon::Ext::Hash_toInt<HASH>);
    self->addClassMethod(cls, "reset", Falcon::Ext::Hash_reset<HASH>);
    cls->setWKS(true);

    if(parent)
        cls->getClassDef()->addInheritance(parent);

    return cls;
}

Falcon::Module *hash_module_init(void)
{
    #define FALCON_DECLARE_MODULE self

    // initialize the module
    Falcon::Module *self = new Falcon::Module();
    self->name( "hash" );
    self->language( "en_US" );
    self->engineVersion( FALCON_VERSION_NUM );
    self->version( VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION );

    //============================================================
    // Here declare the international string table implementation
    //
    #include "hash_st.h"

    //============================================================
    // API declarations
    //
    Falcon::Symbol *baseSym = SimpleRegisterHash<Falcon::Mod::HashBaseFalcon>(self, "HashBase", NULL);

    SimpleRegisterHash<Falcon::Mod::CRC32>         (self, "CRC32"        , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::Adler32>       (self, "Adler32"      , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::SHA1Hash>      (self, "SHA1Hash"     , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::SHA224Hash>    (self, "SHA224Hash"   , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::SHA256Hash>    (self, "SHA256Hash"   , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::SHA384Hash>    (self, "SHA384Hash"   , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::SHA512Hash>    (self, "SHA512Hash"   , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::MD2Hash>       (self, "MD2Hash"      , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::MD4Hash>       (self, "MD4Hash"      , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::MD5Hash>       (self, "MD5Hash"      , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::WhirlpoolHash> (self, "WhirlpoolHash", new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::TigerHash>     (self, "TigerHash"    , new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::RIPEMD128Hash> (self, "RIPEMD128Hash", new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::RIPEMD160Hash> (self, "RIPEMD160Hash", new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::RIPEMD256Hash> (self, "RIPEMD256Hash", new Falcon::InheritDef(baseSym));
    SimpleRegisterHash<Falcon::Mod::RIPEMD320Hash> (self, "RIPEMD320Hash", new Falcon::InheritDef(baseSym));

    self->addExtFunc("crc32",      Falcon::Ext::Func_hashSimple<Falcon::Mod::CRC32>);
    self->addExtFunc("adler32",    Falcon::Ext::Func_hashSimple<Falcon::Mod::Adler32>);
    self->addExtFunc("md2",        Falcon::Ext::Func_hashSimple<Falcon::Mod::MD2Hash>);
    self->addExtFunc("md4",        Falcon::Ext::Func_hashSimple<Falcon::Mod::MD4Hash>);
    self->addExtFunc("md5",        Falcon::Ext::Func_hashSimple<Falcon::Mod::MD5Hash>);
    self->addExtFunc("sha1",       Falcon::Ext::Func_hashSimple<Falcon::Mod::SHA1Hash>);
    self->addExtFunc("sha224",     Falcon::Ext::Func_hashSimple<Falcon::Mod::SHA224Hash>);
    self->addExtFunc("sha256",     Falcon::Ext::Func_hashSimple<Falcon::Mod::SHA256Hash>);
    self->addExtFunc("sha384",     Falcon::Ext::Func_hashSimple<Falcon::Mod::SHA384Hash>);
    self->addExtFunc("sha512",     Falcon::Ext::Func_hashSimple<Falcon::Mod::SHA512Hash>);
    self->addExtFunc("tiger",      Falcon::Ext::Func_hashSimple<Falcon::Mod::TigerHash>);
    self->addExtFunc("whirlpool",  Falcon::Ext::Func_hashSimple<Falcon::Mod::WhirlpoolHash>);
    self->addExtFunc("ripemd128",  Falcon::Ext::Func_hashSimple<Falcon::Mod::RIPEMD128Hash>);
    self->addExtFunc("ripemd160",  Falcon::Ext::Func_hashSimple<Falcon::Mod::RIPEMD160Hash>);
    self->addExtFunc("ripemd256",  Falcon::Ext::Func_hashSimple<Falcon::Mod::RIPEMD256Hash>);
    self->addExtFunc("ripemd320",  Falcon::Ext::Func_hashSimple<Falcon::Mod::RIPEMD320Hash>);

    self->addExtFunc("hash", Falcon::Ext::Func_hash)
        ->addParam("raw")->addParam("which");

    self->addExtFunc("makeHash", Falcon::Ext::Func_makeHash)
        ->addParam("name");

    self->addExtFunc("hmac", Falcon::Ext::Func_hmac)
        ->addParam("raw")->addParam("which")->addParam("key")->addParam("data");

    self->addExtFunc("getSupportedHashes", Falcon::Ext::Func_GetSupportedHashes);

    // generate CRC32 table
    Falcon::Mod::CRC32::GenTab();

    return self;
}

FALCON_MODULE_DECL
{
    return hash_module_init();
}

/* end of hash.cpp */