#pragma once
#include <stdint.h>
#include <stddef.h>
#if defined(ARDUINO)
#include <bearssl/bearssl.h>
#else
#include <openssl/evp.h>
#include <utility>
#endif
class Sha256Stream {
 public:
  Sha256Stream(){
#if defined(ARDUINO)
    br_sha256_init(&context_);
#else
    context_=EVP_MD_CTX_new();if(context_)EVP_DigestInit_ex(context_,EVP_sha256(),nullptr);
#endif
  }
#if !defined(ARDUINO)
  ~Sha256Stream(){EVP_MD_CTX_free(context_);}
  Sha256Stream(const Sha256Stream&)=delete;
  Sha256Stream& operator=(const Sha256Stream&)=delete;
  Sha256Stream(Sha256Stream&& other)noexcept:context_(other.context_){other.context_=nullptr;}
  Sha256Stream& operator=(Sha256Stream&& other)noexcept{std::swap(context_,other.context_);return *this;}
#endif
  void add(const uint8_t* data,size_t size){
#if defined(ARDUINO)
    br_sha256_update(&context_,data,size);
#else
    if(context_)EVP_DigestUpdate(context_,data,size);
#endif
  }
  bool digest(uint8_t out[32])const{
#if defined(ARDUINO)
    br_sha256_out(&context_,out);return true;
#else
    EVP_MD_CTX* copy=EVP_MD_CTX_new();if(!copy)return false;unsigned size=0;
    const bool ok=context_ && EVP_MD_CTX_copy_ex(copy,context_)==1 && EVP_DigestFinal_ex(copy,out,&size)==1 && size==32;
    EVP_MD_CTX_free(copy);return ok;
#endif
  }
 private:
#if defined(ARDUINO)
  br_sha256_context context_;
#else
  EVP_MD_CTX* context_=nullptr;
#endif
};
