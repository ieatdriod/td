//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/utils/Random.h"
#include "td/utils/buffer.h"
#include "td/utils/crypto.h"
#include "td/utils/format.h"
#include "td/utils/int_types.h"
#include "td/utils/Status.h"
#include "td/utils/optional.h"
#include "td/utils/port/FileFd.h"

namespace td {
// secureValueSendingStatePending = SecureValueSendingState;
// secureValueSendingStateFailed = SecureValueSendingState;
//
// secureValue key:string sending_state:SecureValueSendingState unencrypted_text:string data:string files:vector<File> = SecureValue;
//
// inputSecureValue key:string unencrypted_text:string data:string files:vector<InputFile> = InputSecureValue;
//
// updateSecureValue value:SecureValue = Update;
//
// setSecureValue password:string value:InputSecureValue = SecureValue;
// getSecureValue password:string key:string = SecureValue;
//

// Types
// Password
// Secret - 32 bytes with sum == 239
// EncryptedSecret - encrypted secret
// ValueHash - 32 bytes, sha256 from value
//
// ValueFull = ValueText? ValueData? ValueFile* = [Value]
// Value = ValueText | ValueData | ValueFile
//
// ValueMeta = random_prefix, secret, hash
//
// Helpers
//   calc_aes_cbc_state :: ValueSecret -> ValueHash -> AesCbcState
//
// Encryption.
// To encrypt data:
//   RandomPrefix, ValueSecret, Value:
//   calc_value_hash :: RandomPrefix -> Value -> ValueHash
//   do_encrypt :: RandomPrefix -> Value -> AesCbcState -> EncryptedValue // async
//   encrypt :: (ValueSecret, RandomPrefix, Value) -> (EncryptedValue, ValueHash)
//
//
// To decrypt data:
//   ValueSecret, ValueHash, EncryptedValue
//   do_decrypt :: EncryptedValue -> AesCbcState -> (RandomPrefix, Value, ValueHash) // async
//   decrypt :: (ValueSecret, ValueHash, EncryptedValue) -> Value
//
// To encrypt FullValue:
//   ValueSecret, [(RandomPrefix, Value)]
//   (ValueSecret, [(RandomPrefix, Value)]) -> [(ValueSecret, RandomPrefix, Value)]
//   [(ValueSecret, RandomPrefix, Value)] -> [(EncryptedValue, ValueHash)]
//

namespace secure_storage {
// Helpers
class ValueHash {
 public:
  ValueHash(UInt256 hash) : hash_(hash) {
  }
  Slice as_slice() const {
    return td::as_slice(hash_);
  }

 private:
  UInt256 hash_;
};

class DataView {
 public:
  virtual int64 size() = 0;
  virtual Result<BufferSlice> pread(int64 offset, int64 size) = 0;
  virtual ~DataView() = default;
};

class FileDataView : public DataView {
 public:
  FileDataView(FileFd &fd, int64 size);

  int64 size() override;
  Result<BufferSlice> pread(int64 offset, int64 size) override;

 private:
  FileFd &fd_;
  int64 size_;
};

class BufferSliceDataView : public DataView {
 public:
  BufferSliceDataView(BufferSlice buffer_slice);
  int64 size() override;
  Result<BufferSlice> pread(int64 offset, int64 size) override;

 private:
  BufferSlice buffer_slice_;
};

class ConcatDataView : public DataView {
 public:
  ConcatDataView(DataView &left, DataView &right);
  int64 size() override;
  Result<BufferSlice> pread(int64 offset, int64 size) override;

 private:
  DataView &left_;
  DataView &right_;
};

AesCbcState calc_aes_cbc_state(Slice seed);
Result<ValueHash> calc_value_hash(DataView &data_view);
BufferSlice gen_random_prefix(int64 data_size);

class Password {
 public:
  Password(std::string password);
  Slice as_slice() const;

 private:
  std::string password_;
};

class EncryptedSecret;

class Secret {
 public:
  static Result<Secret> create(Slice secret);
  static Secret create_new();

  Slice as_slice() const;
  EncryptedSecret encrypt(Slice key);

 private:
  Secret(UInt256 secret);
  UInt256 secret_;
};

class EncryptedSecret {
 public:
  static Result<EncryptedSecret> create(Slice encrypted_secret);
  Result<Secret> decrypt(Slice key);
  Slice as_slice() const;

 private:
  EncryptedSecret(UInt256 encrypted_secret);
  UInt256 encrypted_secret_;
};

// Decryption
class Decryptor {
 public:
  Decryptor(AesCbcState aes_cbc_state);
  Result<BufferSlice> append(BufferSlice data);
  Result<ValueHash> finish();

 private:
  AesCbcState aes_cbc_state_;
  Sha256State sha256_state_;
  bool skipped_prefix_{false};
  size_t to_skip_{0};
};

// Encryption
class Encryptor : public DataView {
 public:
  Encryptor(AesCbcState aes_cbc_state, DataView &data_view);
  int64 size() override;
  Result<BufferSlice> pread(int64 offset, int64 size) override;

 private:
  AesCbcState aes_cbc_state_;
  int64 current_offset_{0};
  DataView &data_view_;
};

// Main functions

//   decrypt :: (ValueSecret, ValueHash, EncryptedValue) -> Value
//   encrypt :: (ValueSecret, RandomPrefix, Value) -> (EncryptedValue, ValueHash)

struct EncryptedValue {
  BufferSlice data;
  ValueHash hash;
};
struct EncryptedFile {
  std::string path;
  ValueHash hash;
};

Result<EncryptedValue> encrypt_value(const Secret &secret, Slice data);
Result<ValueHash> encrypt_file(const Secret &secret, std::string src, std::string dest);

Result<BufferSlice> decrypt_value(const Secret &secret, const ValueHash &hash, Slice data);
Status decrypt_file(const Secret &secret, const ValueHash &hash, std::string src, std::string dest);

}  // namespace secure_storage
}  // namespace td
