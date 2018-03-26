//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2018
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/utils/tests.h"
#include "td/utils/filesystem.h"
#include "td/utils/port/path.h"

#include "td/telegram/SecureStorage.h"

using namespace td;

TEST(SecureStorage, secret) {
  using namespace td::secure_storage;
  auto secret = Secret::create_new();
  std::string key = "cucumber";
  auto encrypted_secret = secret.encrypt(key);
  ASSERT_TRUE(encrypted_secret.as_slice() != secret.as_slice());
  auto decrypted_secret = encrypted_secret.decrypt(key).ok();
  ASSERT_TRUE(secret.as_slice() == decrypted_secret.as_slice());
  ASSERT_TRUE(encrypted_secret.decrypt("notcucumber").is_error());
}

TEST(SecureStorage, simple) {
  using namespace td::secure_storage;

  BufferSlice value("Small tale about cucumbers");
  auto value_secret = Secret::create_new();

  auto value_view = BufferSliceDataView(value.copy());
  BufferSlice prefix = gen_random_prefix(value_view.size());
  auto prefix_view = BufferSliceDataView(std::move(prefix));
  auto full_value_view = ConcatDataView(prefix_view, value_view);
  auto hash = calc_value_hash(full_value_view).move_as_ok();

  Encryptor encryptor(calc_aes_cbc_state(PSLICE() << value_secret.as_slice() << hash.as_slice()), full_value_view);
  auto encrypted_value = encryptor.pread(0, encryptor.size()).move_as_ok();

  Decryptor decryptor(calc_aes_cbc_state(PSLICE() << value_secret.as_slice() << hash.as_slice()));
  auto res = decryptor.append(encrypted_value.copy()).move_as_ok();
  auto decrypted_hash = decryptor.finish().ok();
  ASSERT_TRUE(decrypted_hash.as_slice() == hash.as_slice());
  ASSERT_TRUE(res.as_slice() == value.as_slice());

  {
    auto encrypted_value = encrypt_value(value_secret, value.as_slice()).move_as_ok();
    auto decrypted_value =
        decrypt_value(value_secret, encrypted_value.hash, encrypted_value.data.as_slice()).move_as_ok();
    ASSERT_TRUE(decrypted_value.as_slice() == value.as_slice());
  }

  {
    std::string value_path = "value.txt";
    std::string encrypted_path = "encrypted.txt";
    std::string decrypted_path = "decrypted.txt";
    td::unlink(value_path).ignore();
    td::unlink(encrypted_path).ignore();
    td::unlink(decrypted_path).ignore();
    std::string value(100000, 'a');
    td::write_file(value_path, value);
    auto hash = encrypt_file(value_secret, value_path, encrypted_path).move_as_ok();
    decrypt_file(value_secret, hash, encrypted_path, decrypted_path).ensure();
    ASSERT_TRUE(td::read_file(decrypted_path).move_as_ok().as_slice() == value);
  }
}
