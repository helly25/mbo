// SPDX-FileCopyrightText: Copyright (c) The helly25 authors (helly25.com)
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mbo/digest/digest.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

// Known-answer vectors: the FIPS 180-4 / NIST CAVP examples plus
// padding-boundary lengths (55/56/63/64/65 bytes straddle the 0x80+length
// padding cases). Every hex value was generated with python's hashlib, whose
// implementations are independently NIST-validated.
namespace mbo::digest {
namespace {

using ::testing::ElementsAreArray;

static_assert(IsDigestAlgorithm<sha256::Algorithm>);
static_assert(IsDigestAlgorithm<sha224::Algorithm>);
static_assert(HasStreaming<sha256::Algorithm>);
static_assert(HasStreaming<sha224::Algorithm>);

constexpr uint8_t Nibble(char chr) noexcept {
  if (chr >= '0' && chr <= '9') {
    return static_cast<uint8_t>(chr - '0');
  }
  return static_cast<uint8_t>(chr - 'a' + 10);  // NOLINT(*-magic-numbers)
}

// Parses "ab01..." into bytes at compile time (test-side inverse of
// `ToHexString`).
template<std::size_t DigestSize>
constexpr std::array<uint8_t, DigestSize> FromHex(std::string_view hex) noexcept {
  std::array<uint8_t, DigestSize> result = {};
  if (hex.size() != 2 * DigestSize) {
    return result;  // Wrong-length vector: returns zeros, which cannot match a real digest.
  }
  for (std::size_t i = 0; i < DigestSize; ++i) {
    // NOLINTNEXTLINE(*-constant-array-index)
    result[i] = static_cast<uint8_t>(
        (static_cast<uint32_t>(Nibble(hex[2 * i])) << 4U) | static_cast<uint32_t>(Nibble(hex[(2 * i) + 1])));
  }
  return result;
}

struct TestVector {
  std::string_view input;
  std::string_view hex;
};

// 160 'a's; the boundary-length inputs below are prefixes of this (block
// sizes range from 64 to the SHA3-224 rate of 144).
constexpr std::string_view kManyAs =
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

constexpr std::string_view kTwoBlockMsg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
constexpr std::string_view kFourBlockMsg =
    "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";

template<typename Algo>
struct AlgoTraits;

template<>
struct AlgoTraits<sha256::Algorithm> {
  static constexpr std::string_view kName = "sha256";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {.input = "abc", .hex = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {.input = kTwoBlockMsg, .hex = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
      {.input = kFourBlockMsg, .hex = "cf5b16a778af8380036ce59e7b0492370b249b11e8f07a51afac45037afee9d1"},
      {.input = std::string_view{"\0", 1}, .hex = "6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d"},
      {.input = kManyAs.substr(0, 55), .hex = "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
      {.input = kManyAs.substr(0, 56), .hex = "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
      {.input = kManyAs.substr(0, 63), .hex = "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
      {.input = kManyAs.substr(0, 64), .hex = "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
      {.input = kManyAs.substr(0, 65), .hex = "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"},
  });
  static constexpr std::string_view kMillionAHex = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
};

template<>
struct AlgoTraits<sha224::Algorithm> {
  static constexpr std::string_view kName = "sha224";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"},
      {.input = "abc", .hex = "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"},
      {.input = kTwoBlockMsg, .hex = "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525"},
      {.input = kFourBlockMsg, .hex = "c97ca9a559850ce97a04a96def6d99a9e0e0e2ab14e6b8df265fc0b3"},
      {.input = std::string_view{"\0", 1}, .hex = "fff9292b4201617bdc4d3053fce02734166a683d7d858a7f5f59b073"},
      {.input = kManyAs.substr(0, 55), .hex = "fb0bd626a70c28541dfa781bb5cc4d7d7f56622a58f01a0b1ddd646f"},
      {.input = kManyAs.substr(0, 56), .hex = "d40854fc9caf172067136f2e29e1380b14626bf6f0dd06779f820dcd"},
      {.input = kManyAs.substr(0, 63), .hex = "1d4e051f4d6fed2a63fd2421e65834cec00d64456553de3496ae8b1d"},
      {.input = kManyAs.substr(0, 64), .hex = "a88cd5cde6d6fe9136a4e58b49167461ea95d388ca2bdb7afdc3cbf4"},
      {.input = kManyAs.substr(0, 65), .hex = "ff8716f600af42959d0efb52e1f21b01bb328733009344d511c299fb"},
  });
  static constexpr std::string_view kMillionAHex = "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67";
};

template<>
struct AlgoTraits<sha1::Algorithm> {
  static constexpr std::string_view kName = "sha1";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
      {.input = "abc", .hex = "a9993e364706816aba3e25717850c26c9cd0d89d"},
      {.input = kTwoBlockMsg, .hex = "84983e441c3bd26ebaae4aa1f95129e5e54670f1"},
      {.input = kFourBlockMsg, .hex = "a49b2446a02c645bf419f995b67091253a04a259"},
      {.input = std::string_view{"\0", 1}, .hex = "5ba93c9db0cff93f52b521d7420e43f6eda2784f"},
      {.input = kManyAs.substr(0, 55), .hex = "c1c8bbdc22796e28c0e15163d20899b65621d65a"},
      {.input = kManyAs.substr(0, 56), .hex = "c2db330f6083854c99d4b5bfb6e8f29f201be699"},
      {.input = kManyAs.substr(0, 63), .hex = "03f09f5b158a7a8cdad920bddc29b81c18a551f5"},
      {.input = kManyAs.substr(0, 64), .hex = "0098ba824b5c16427bd7a1122a5a442a25ec644d"},
      {.input = kManyAs.substr(0, 65), .hex = "11655326c708d70319be2610e8a57d9a5b959d3b"},
  });
  static constexpr std::string_view kMillionAHex = "34aa973cd4c4daa4f61eeb2bdbad27316534016f";
};

template<>
struct AlgoTraits<sha512::Algorithm> {
  static constexpr std::string_view kName = "sha512";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "",
       .hex = "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd"
              "47417a81a538327af927da3e"},
      {.input = "abc",
       .hex = "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423"
              "643ce80e2a9ac94fa54ca49f"},
      {.input = kTwoBlockMsg,
       .hex = "204a8fc6dda82f0a0ced7beb8e08a41657c16ef468b228a8279be331a703c33596fd15c13b1b07f9aa1d3bea57789ca031ad85c7"
              "a71dd70354ec631238ca3445"},
      {.input = kFourBlockMsg,
       .hex = "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329ee"
              "b6dd26545e96e55b874be909"},
      {.input = std::string_view{"\0", 1},
       .hex = "b8244d028981d693af7b456af8efa4cad63d282e19ff14942c246e50d9351d22704a802a71c3580b6370de4ceb293c324a842334"
              "2557d4e5c38438f0e36910ee"},
      {.input = kManyAs.substr(0, 111),
       .hex = "fa9121c7b32b9e01733d034cfc78cbf67f926c7ed83e82200ef86818196921760b4beff48404df811b953828274461673c68d04e"
              "297b0eb7b2b4d60fc6b566a2"},
      {.input = kManyAs.substr(0, 112),
       .hex = "c01d080efd492776a1c43bd23dd99d0a2e626d481e16782e75d54c2503b5dc32bd05f0f1ba33e568b88fd2d970929b719ecbb152"
              "f58f130a407c8830604b70ca"},
      {.input = kManyAs.substr(0, 127),
       .hex = "828613968b501dc00a97e08c73b118aa8876c26b8aac93df128502ab360f91bab50a51e088769a5c1eff4782ace147dce3642554"
              "199876374291f5d921629502"},
      {.input = kManyAs.substr(0, 128),
       .hex = "b73d1929aa615934e61a871596b3f3b33359f42b8175602e89f7e06e5f658a243667807ed300314b95cacdd579f3e33abdfbe351"
              "909519a846d465c59582f321"},
      {.input = kManyAs.substr(0, 129),
       .hex = "4f681e0bd53cda4b5a2041cc8a06f2eabde44fb16c951fbd5b87702f07aeab611565b19c47fde30587177ebb852e3971bbd8d3fd"
              "30da18d71037dfbd98420429"},
  });
  static constexpr std::string_view kMillionAHex =
      "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973ebde0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e"
      "4eadb217ad8cc09b";
};

template<>
struct AlgoTraits<sha384::Algorithm> {
  static constexpr std::string_view kName = "sha384";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "",
       .hex = "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"},
      {.input = "abc",
       .hex = "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7"},
      {.input = kTwoBlockMsg,
       .hex = "3391fdddfc8dc7393707a65b1b4709397cf8b1d162af05abfe8f450de5f36bc6b0455a8520bc4e6f5fe95b1fe3c8452b"},
      {.input = kFourBlockMsg,
       .hex = "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712fcc7c71a557e2db966c3e9fa91746039"},
      {.input = std::string_view{"\0", 1},
       .hex = "bec021b4f368e3069134e012c2b4307083d3a9bdd206e24e5f0d86e13d6636655933ec2b413465966817a9c208a11717"},
      {.input = kManyAs.substr(0, 111),
       .hex = "3c37955051cb5c3026f94d551d5b5e2ac38d572ae4e07172085fed81f8466b8f90dc23a8ffcdea0b8d8e58e8fdacc80a"},
      {.input = kManyAs.substr(0, 112),
       .hex = "187d4e07cb306103c69967bf544d0dfbe9042577599c73c330abc0cb64c61236d5ed565ee19119d8c31779a38f791fcd"},
      {.input = kManyAs.substr(0, 127),
       .hex = "9bd06b1763c2cf7aef40e795dc65bc96d59c41b537f3ad72ebdefd485476b5717c1aeb37c327fe9c1831b12b9efd08ae"},
      {.input = kManyAs.substr(0, 128),
       .hex = "edb12730a366098b3b2beac75a3bef1b0969b15c48e2163c23d96994f8d1bef760c7e27f3c464d3829f56c0d53808b0b"},
      {.input = kManyAs.substr(0, 129),
       .hex = "39b6f5a7b0e781dbc419f72e49b30eaac10f2c98c4403bc610da31067fd1b48f324138c8615d2b496d08d73d5e865326"},
  });
  static constexpr std::string_view kMillionAHex =
      "9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b07b8b3dc38ecc4ebae97ddd87f3d8985";
};

template<>
struct AlgoTraits<sha512_224::Algorithm> {
  static constexpr std::string_view kName = "sha512_224";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4"},
      {.input = "abc", .hex = "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa"},
      {.input = kTwoBlockMsg, .hex = "e5302d6d54bb242275d1e7622d68df6eb02dedd13f564c13dbda2174"},
      {.input = kFourBlockMsg, .hex = "23fec5bb94d60b23308192640b0c453335d664734fe40e7268674af9"},
      {.input = std::string_view{"\0", 1}, .hex = "283bb59af7081ed08197227d8f65b9591ffe1155be43e9550e57f941"},
      {.input = kManyAs.substr(0, 111), .hex = "3ebe1b48e8c66acb9ae014db95b4bec93de7e9572bff41cf566bd7d0"},
      {.input = kManyAs.substr(0, 112), .hex = "79b41fef2a0439d2705724a67615f7bcbcd2bf5664a7774b80818eb6"},
      {.input = kManyAs.substr(0, 127), .hex = "65aec5ddd181bb86e1921d493a0667492cb8dbc2b560ec061ed2c492"},
      {.input = kManyAs.substr(0, 128), .hex = "261b94bcba554264b3b738e9e09e7dc68ac8e0b4c8517fe9bb7c3617"},
      {.input = kManyAs.substr(0, 129), .hex = "3a19e0ab45e58ffb1db38df972ac85842bff2bbacd16ec9819a6a434"},
  });
  static constexpr std::string_view kMillionAHex = "37ab331d76f0d36de422bd0edeb22a28accd487b7a8453ae965dd287";
};

template<>
struct AlgoTraits<sha512_256::Algorithm> {
  static constexpr std::string_view kName = "sha512_256";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a"},
      {.input = "abc", .hex = "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23"},
      {.input = kTwoBlockMsg, .hex = "bde8e1f9f19bb9fd3406c90ec6bc47bd36d8ada9f11880dbc8a22a7078b6a461"},
      {.input = kFourBlockMsg, .hex = "3928e184fb8690f840da3988121d31be65cb9d3ef83ee6146feac861e19b563a"},
      {.input = std::string_view{"\0", 1}, .hex = "10baad1713566ac2333467bddb0597dec9066120dd72ac2dcb8394221dcbe43d"},
      {.input = kManyAs.substr(0, 111), .hex = "0239e429f98d0ed61ee8e2a7c30afe98c1c3a80ce5dff62a107e9c538f7632ce"},
      {.input = kManyAs.substr(0, 112), .hex = "9216b5303edb66504570bee90e48ea5beaa5e9fe9f760bbd3e0460559fc005f6"},
      {.input = kManyAs.substr(0, 127), .hex = "2fe3b2a6ee7e12f6fe4ba82166541ad9b4ed882c493581cbe300d68f3757b778"},
      {.input = kManyAs.substr(0, 128), .hex = "b88f97e274f9c1d49f181c8cbd01a9c74930ad055a46ac4499a1d601f1c80bf2"},
      {.input = kManyAs.substr(0, 129), .hex = "fb9035c9009ed4a60e37510339ebdb1c771339f30aa581d5dea3690a524c23f1"},
  });
  static constexpr std::string_view kMillionAHex = "9a59a052930187a97038cae692f30708aa6491923ef5194394dc68d56c74fb21";
};

template<>
struct AlgoTraits<md5::Algorithm> {
  static constexpr std::string_view kName = "md5";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "d41d8cd98f00b204e9800998ecf8427e"},
      {.input = "abc", .hex = "900150983cd24fb0d6963f7d28e17f72"},
      {.input = kTwoBlockMsg, .hex = "8215ef0796a20bcaaae116d3876c664a"},
      {.input = kFourBlockMsg, .hex = "03dd8807a93175fb062dfb55dc7d359c"},
      {.input = std::string_view{"\0", 1}, .hex = "93b885adfe0da089cdf634904fd59f71"},
      {.input = kManyAs.substr(0, 55), .hex = "ef1772b6dff9a122358552954ad0df65"},
      {.input = kManyAs.substr(0, 56), .hex = "3b0c8ac703f828b04c6c197006d17218"},
      {.input = kManyAs.substr(0, 63), .hex = "b06521f39153d618550606be297466d5"},
      {.input = kManyAs.substr(0, 64), .hex = "014842d480b571495a4a0363793f7367"},
      {.input = kManyAs.substr(0, 65), .hex = "c743a45e0d2e6a95cb859adae0248435"},
  });
  static constexpr std::string_view kMillionAHex = "7707d6ae4e027c70eea2a935c2296f21";
};

template<>
struct AlgoTraits<sha3_224::Algorithm> {
  static constexpr std::string_view kName = "sha3_224";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"},
      {.input = "abc", .hex = "e642824c3f8cf24ad09234ee7d3c766fc9a3a5168d0c94ad73b46fdf"},
      {.input = kTwoBlockMsg, .hex = "8a24108b154ada21c9fd5574494479ba5c7e7ab76ef264ead0fcce33"},
      {.input = kFourBlockMsg, .hex = "543e6868e1666c1a643630df77367ae5a62a85070a51c14cbf665cbc"},
      {.input = std::string_view{"\0", 1}, .hex = "bdd5167212d2dc69665f5a8875ab87f23d5ce7849132f56371a19096"},
      {.input = kManyAs.substr(0, 143), .hex = "73b1b22b54f515f626a6abdde6af25cd4801dc6e9dc7fa3f77e1c122"},
      {.input = kManyAs.substr(0, 144), .hex = "f9019111996dcf160e284e320fd6d8825cabcd41a5ffdc4c5e9d64b6"},
      {.input = kManyAs.substr(0, 145), .hex = "7f0521c84aeacc8a46aba17171acbdd22522509a71c663257fbdee0e"},
  });
  static constexpr std::string_view kMillionAHex = "d69335b93325192e516a912e6d19a15cb51c6ed5c15243e7a7fd653c";
};

template<>
struct AlgoTraits<sha3_256::Algorithm> {
  static constexpr std::string_view kName = "sha3_256";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"},
      {.input = "abc", .hex = "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"},
      {.input = kTwoBlockMsg, .hex = "41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376"},
      {.input = kFourBlockMsg, .hex = "916f6061fe879741ca6469b43971dfdb28b1a32dc36cb3254e812be27aad1d18"},
      {.input = std::string_view{"\0", 1}, .hex = "5d53469f20fef4f8eab52b88044ede69c77a6a68a60728609fc4a65ff531e7d0"},
      {.input = kManyAs.substr(0, 135), .hex = "8094bb53c44cfb1e67b7c30447f9a1c33696d2463ecc1d9c92538913392843c9"},
      {.input = kManyAs.substr(0, 136), .hex = "3fc5559f14db8e453a0a3091edbd2bc25e11528d81c66fa570a4efdcc2695ee1"},
      {.input = kManyAs.substr(0, 137), .hex = "f8d6846cedd2ccfadf15c5879ef95af724d799eed7391fb1c91f95344e738614"},
  });
  static constexpr std::string_view kMillionAHex = "5c8875ae474a3634ba4fd55ec85bffd661f32aca75c6d699d0cdcb6c115891c1";
};

template<>
struct AlgoTraits<sha3_384::Algorithm> {
  static constexpr std::string_view kName = "sha3_384";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "",
       .hex = "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004"},
      {.input = "abc",
       .hex = "ec01498288516fc926459f58e2c6ad8df9b473cb0fc08c2596da7cf0e49be4b298d88cea927ac7f539f1edf228376d25"},
      {.input = kTwoBlockMsg,
       .hex = "991c665755eb3a4b6bbdfb75c78a492e8c56a22c5c4d7e429bfdbc32b9d4ad5aa04a1f076e62fea19eef51acd0657c22"},
      {.input = kFourBlockMsg,
       .hex = "79407d3b5916b59c3e30b09822974791c313fb9ecc849e406f23592d04f625dc8c709b98b43b3852b337216179aa7fc7"},
      {.input = std::string_view{"\0", 1},
       .hex = "127677f8b66725bbcb7c3eae9698351ca41e0eb6d66c784bd28dcdb3b5fb12d0c8e840342db03ad1ae180b92e3504933"},
      {.input = kManyAs.substr(0, 103),
       .hex = "af61fb4fd1c6afe80857fcba888318a0a1426635b4509f09707e3787630bdb621655ffa54f5884088ccc000f81436414"},
      {.input = kManyAs.substr(0, 104),
       .hex = "3a4f3b6284e571238884e95655e8c8a60e068e4059a9734abc08823a900d161592860243f00619ae699a29092ed91a16"},
      {.input = kManyAs.substr(0, 105),
       .hex = "cb73ab2f8f5fbb13f0e115a7062ba1644aa16534aa80d076ef27f8550deb900d89bdfa169b45073223acadb6001204d3"},
  });
  static constexpr std::string_view kMillionAHex =
      "eee9e24d78c1855337983451df97c8ad9eedf256c6334f8e948d252d5e0e76847aa0774ddb90a842190d2c558b4b8340";
};

template<>
struct AlgoTraits<sha3_512::Algorithm> {
  static constexpr std::string_view kName = "sha3_512";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "",
       .hex = "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d"
              "95b6d3e301758586281dcd26"},
      {.input = "abc",
       .hex = "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57647e3934057340b4c"
              "f408d5a56592f8274eec53f0"},
      {.input = kTwoBlockMsg,
       .hex = "04a371e84ecfb5b8b77cb48610fca8182dd457ce6f326a0fd3d7ec2f1e91636dee691fbe0c985302ba1b0d8dc78c086346b533b4"
              "9c030d99a27daf1139d6e75e"},
      {.input = kFourBlockMsg,
       .hex = "afebb2ef542e6579c50cad06d2e578f9f8dd6881d7dc824d26360feebf18a4fa73e3261122948efcfd492e74e82e2189ed0fb440"
              "d187f382270cb455f21dd185"},
      {.input = std::string_view{"\0", 1},
       .hex = "7127aab211f82a18d06cf7578ff49d5089017944139aa60d8bee057811a15fb55a53887600a3eceba004de51105139f32506fe5b"
              "53e1913bfa6b32e716fe97da"},
      {.input = kManyAs.substr(0, 71),
       .hex = "070faf98d2a8fddf8ed886408744dc06456096c2e045f26f3c7b010530e6bbb3db535a54d636856f4e0e1e982461cb9a7e8e57ff"
              "8895cff1619af9f0e486e28c"},
      {.input = kManyAs.substr(0, 72),
       .hex = "a8ae722a78e10cbbc413886c02eb5b369a03f6560084aff566bd597bb7ad8c1ccd86e81296852359bf2faddb5153c0a744572298"
              "7875e74287adac21adebe952"},
      {.input = kManyAs.substr(0, 73),
       .hex = "23e6a8815f8201dbbf6a5463be8dcadb1acea9df5f8998954e59ac9565cf6d29b17aa27a5e8b0fc06343db6122d6e544d27583dd"
              "c78504d08203217e7e65b6bd"},
  });
  static constexpr std::string_view kMillionAHex =
      "3c3a876da14034ab60627c077bb98f7e120a2a5370212dffb3385a18d4f38859ed311d0a9d5141ce9cc5c66ee689b266a8aa18ace8282a0e"
      "0db596c90b0a7b87";
};

template<>
struct AlgoTraits<blake2b::Algorithm> {
  static constexpr std::string_view kName = "blake2b";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "",
       .hex = "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d25e1031afee585313896444934eb04b903a685b"
              "1448b755d56f701afe9be2ce"},
      {.input = "abc",
       .hex = "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d17d87c5392aab792dc252d5de4533cc9518d38aa8"
              "dbf1925ab92386edd4009923"},
      {.input = kTwoBlockMsg,
       .hex = "7285ff3e8bd768d69be62b3bf18765a325917fa9744ac2f582a20850bc2b1141ed1b3e4528595acc90772bdf2d37dc8a47130b44"
              "f33a02e8730e5ad8e166e888"},
      {.input = kFourBlockMsg,
       .hex = "ce741ac5930fe346811175c5227bb7bfcd47f42612fae46c0809514f9e0e3a11ee1773287147cdeaeedff50709aa716341fe6524"
              "0f4ad6777d6bfaf9726e5e52"},
      {.input = std::string_view{"\0", 1},
       .hex = "2fa3f686df876995167e7c2e5d74c4c7b6e48f8068fe0e44208344d480f7904c36963e44115fe3eb2a3ac8694c28bcb4f5a0f327"
              "6f2e79487d8219057a506e4b"},
      {.input = kManyAs.substr(0, 127),
       .hex = "94596b9d6199c807c40ae1a935f3633ba5a8dd5655f7f1bd44f5285b1ce8dbb0054771eba409539df85a963296d2878880710515"
              "3c90fa3ec3d761228e90f8b8"},
      {.input = kManyAs.substr(0, 128),
       .hex = "fc6c71f688f43ea7d60817478808f3cac753e61571865c95adbc2d9122c943a76b92c2cb1047ef3fe7bf6e436ec1d0a99a9e5b21"
              "6780bf7fed9d7ca91d3a8f3b"},
      {.input = kManyAs.substr(0, 129),
       .hex = "55e6e0eb418149a8af92fd9ddc99254781b2f522a131b4f4d984404b71a00e1167b8124d5dcddd4c6977b299392335d6edd303da"
              "6d344d74bbef2d38101b232b"},
  });
  static constexpr std::string_view kMillionAHex =
      "98fb3efb7206fd19ebf69b6f312cf7b64e3b94dbe1a17107913975a793f177e1d077609d7fba363cbba00d05f7aa4e4fa8715d6428104c0a"
      "75643b0ff3fd3eaf";
};

template<>
struct AlgoTraits<blake2b_256::Algorithm> {
  static constexpr std::string_view kName = "blake2b_256";
  static constexpr auto kVectors = std::to_array<TestVector>({
      {.input = "", .hex = "0e5751c026e543b2e8ab2eb06099daa1d1e5df47778f7787faab45cdf12fe3a8"},
      {.input = "abc", .hex = "bddd813c634239723171ef3fee98579b94964e3bb1cb3e427262c8c068d52319"},
      {.input = kTwoBlockMsg, .hex = "5f7a93da9c5621583f22e49e8e91a40cbba37536622235a380f434b9f68e49c4"},
      {.input = kFourBlockMsg, .hex = "90a0bcf5e5a67ac1578c2754617994cfc248109275a809a0721feebd1e918738"},
      {.input = std::string_view{"\0", 1}, .hex = "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314"},
      {.input = kManyAs.substr(0, 127), .hex = "59e2f1aba240f20aa591016f5ef429990bc9c2131dcd0d30f0ffd75ed18f317d"},
      {.input = kManyAs.substr(0, 128), .hex = "ae2aa48507885c4c950fb809b2076f959cde9f8ea6da260d9a3587df33dac450"},
      {.input = kManyAs.substr(0, 129), .hex = "2f64744a6de0d2c0b56e64cf6e29a5aaa255010d415d51c75ccc82f73dccd865"},
  });
  static constexpr std::string_view kMillionAHex = "0741850f36cba4259628355d1073e24ddb9ca0e1bfac36fd39ae5dc2101e23a4";
};

template<typename Algo>
class DigestTest : public ::testing::Test {};

using AllAlgorithms = ::testing::Types<
    sha256::Algorithm,
    sha224::Algorithm,
    sha1::Algorithm,
    sha512::Algorithm,
    sha384::Algorithm,
    sha512_224::Algorithm,
    sha512_256::Algorithm,
    md5::Algorithm,
    sha3_224::Algorithm,
    sha3_256::Algorithm,
    sha3_384::Algorithm,
    sha3_512::Algorithm,
    blake2b::Algorithm,
    blake2b_256::Algorithm>;
TYPED_TEST_SUITE(DigestTest, AllAlgorithms);

TYPED_TEST(DigestTest, KnownAnswers) {
  using Traits = AlgoTraits<TypeParam>;
  for (const TestVector& vector : Traits::kVectors) {
    EXPECT_THAT(TypeParam::Digest(vector.input), ElementsAreArray(FromHex<TypeParam::kDigestSize>(vector.hex)))
        << Traits::kName << " input of length " << vector.input.size();
    EXPECT_EQ(ToHexString(TypeParam::Digest(vector.input)), vector.hex);
  }
}

TYPED_TEST(DigestTest, ConstexprMatchesRuntime) {
  using Traits = AlgoTraits<TypeParam>;
  // Compile-time proof for one vector per padding case...
  static constexpr auto kCompileTime = TypeParam::Digest(Traits::kVectors[1].input);  // "abc"
  static_assert(kCompileTime == FromHex<TypeParam::kDigestSize>(AlgoTraits<TypeParam>::kVectors[1].hex));
  // ...and runtime==constexpr equality across all vectors (guards the
  // dual-path `Load32BE`).
  for (const TestVector& vector : Traits::kVectors) {
    const auto runtime = TypeParam::Digest(vector.input);
    EXPECT_THAT(runtime, ElementsAreArray(FromHex<TypeParam::kDigestSize>(vector.hex)));
  }
}

TYPED_TEST(DigestTest, StreamingMatchesOneShot) {
  using Traits = AlgoTraits<TypeParam>;
  std::string all;
  for (const TestVector& vector : Traits::kVectors) {
    all += vector.input;
  }
  const auto expected = TypeParam::Digest(all);
  for (const std::size_t chunk_size : {1U, 3U, 7U, 13U, 63U, 64U, 65U, 200U}) {
    Streamer<TypeParam> stream;
    for (std::size_t pos = 0; pos < all.size(); pos += chunk_size) {
      stream.Update(std::string_view(all).substr(pos, chunk_size));
    }
    EXPECT_THAT(stream.Finalize(), ElementsAreArray(expected)) << "chunk size " << chunk_size;
  }
}

TYPED_TEST(DigestTest, StreamerIsPeekable) {
  Streamer<TypeParam> stream;
  stream.Update("abc");
  EXPECT_THAT(stream.Finalize(), ElementsAreArray(TypeParam::Digest("abc")));
  stream.Update("def");
  EXPECT_THAT(stream.Finalize(), ElementsAreArray(TypeParam::Digest("abcdef")));
}

TYPED_TEST(DigestTest, MillionA) {
  using Traits = AlgoTraits<TypeParam>;
  const std::string input(1'000'000, 'a');
  EXPECT_EQ(ToHexString(TypeParam::Digest(input)), Traits::kMillionAHex);
}

// HMAC known answers (generated with python's hmac module) covering short,
// empty, exactly-block-sized, and longer-than-block keys.
struct HmacVector {
  std::string_view key;
  std::string_view message;
  std::string_view hex;
};

constexpr std::string_view kFoxMsg = "The quick brown fox jumps over the lazy dog";

// 200 'k's; the block-boundary keys below are prefixes of this.
constexpr std::string_view kManyKs =
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
    "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk";

template<typename Algo>
struct HmacTraits;

template<>
struct HmacTraits<md5::Algorithm> {
  static constexpr auto kVectors = std::to_array<HmacVector>({
      {.key = "key", .message = kFoxMsg, .hex = "80070713463e7749b90c2dc24911e275"},
      {.key = "", .message = "", .hex = "74e6f7298a9c2d168935f58c001bad88"},
      {.key = kManyKs.substr(0, 64), .message = "abc", .hex = "0be890bbca0302e362a6c689fc3debcb"},
      {.key = kManyKs.substr(0, 101), .message = kFoxMsg, .hex = "ef4a1032746d820207cadec8410a3cd0"},
  });
};

template<>
struct HmacTraits<sha1::Algorithm> {
  static constexpr auto kVectors = std::to_array<HmacVector>({
      {.key = "key", .message = kFoxMsg, .hex = "de7c9b85b8b78aa6bc8a7a36f70a90701c9db4d9"},
      {.key = "", .message = "", .hex = "fbdb1d1b18aa6c08324b7d64b71fb76370690e1d"},
      {.key = kManyKs.substr(0, 64), .message = "abc", .hex = "7c44f6972fe89fcc6df413921b6e3616adffa964"},
      {.key = kManyKs.substr(0, 101), .message = kFoxMsg, .hex = "f1ceec83638f69d7fe92200114bb7a01281e8f85"},
  });
};

template<>
struct HmacTraits<sha256::Algorithm> {
  static constexpr auto kVectors = std::to_array<HmacVector>({
      {.key = "key", .message = kFoxMsg, .hex = "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8"},
      {.key = "", .message = "", .hex = "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad"},
      {.key = kManyKs.substr(0, 64),
       .message = "abc",
       .hex = "ae0c0e4a2340cf50185eb46aaa8723f4769153661612e212fb0d1fa3170c6202"},
      {.key = kManyKs.substr(0, 101),
       .message = kFoxMsg,
       .hex = "6db9aaee699c68867a3485a31581ae5c6e07c93dab89a6e7d63cc9e6782125ae"},
  });
};

template<>
struct HmacTraits<sha512::Algorithm> {
  static constexpr auto kVectors = std::to_array<HmacVector>({
      {.key = "key",
       .message = kFoxMsg,
       .hex = "b42af09057bac1e2d41708e48a902e09b5ff7f12ab428a4fe86653c73dd248fb82f948a549f7b791a5b41915ee4d1ec3935357e4"
              "e2317250d0372afa2ebeeb3a"},
      {.key = "",
       .message = "",
       .hex = "b936cee86c9f87aa5d3c6f2e84cb5a4239a5fe50480a6ec66b70ab5b1f4ac6730c6c515421b327ec1d69402e53dfb49ad7381eb0"
              "67b338fd7b0cb22247225d47"},
      {.key = kManyKs.substr(0, 128),
       .message = "abc",
       .hex = "b3327a783059e7fa5af7b4836f7b7d3a9403e96a2cb5fd4dd50a78b3eb9b4e71111fc2286d58ea8f997d4271e709ae184d7d825c"
              "bab47c392970b4d84190b9c3"},
      {.key = kManyKs.substr(0, 165),
       .message = kFoxMsg,
       .hex = "ce0ed52d7b91569f67f074974d1295d0a639df4c37962a27be14eb236e9dee327ab8c9535ed4c715f3f961a5148a3297d8c8b7ea"
              "e7529aae1f05e0931bda1e27"},
  });
};

template<>
struct HmacTraits<sha3_256::Algorithm> {
  static constexpr auto kVectors = std::to_array<HmacVector>({
      {.key = "key", .message = kFoxMsg, .hex = "8c6e0683409427f8931711b10ca92a506eb1fafa48fadd66d76126f47ac2c333"},
      {.key = "", .message = "", .hex = "e841c164e5b4f10c9f3985587962af72fd607a951196fc92fb3a5251941784ea"},
      {.key = kManyKs.substr(0, 136),
       .message = "abc",
       .hex = "ac426c65e5140ac0a28def30012fc621fae635683ff5bb254d58f6f3a73e8830"},
      {.key = kManyKs.substr(0, 173),
       .message = kFoxMsg,
       .hex = "815cf700506a8751b80b40e1fd2f95550f6ee8263b0d6032e2d9c08f6c548635"},
  });
};

// Compile-time proof that HMAC is constexpr end to end.
static_assert(
    Hmac<sha256::Algorithm>::Digest(
        HmacTraits<sha256::Algorithm>::kVectors[0].key,
        HmacTraits<sha256::Algorithm>::kVectors[0].message)
    == FromHex<sha256::kDigestSize>(HmacTraits<sha256::Algorithm>::kVectors[0].hex));

template<typename Algo>
class HmacTest : public ::testing::Test {};

using HmacAlgorithms =
    ::testing::Types<md5::Algorithm, sha1::Algorithm, sha256::Algorithm, sha512::Algorithm, sha3_256::Algorithm>;
TYPED_TEST_SUITE(HmacTest, HmacAlgorithms);

TYPED_TEST(HmacTest, KnownAnswers) {
  for (const HmacVector& vector : HmacTraits<TypeParam>::kVectors) {
    EXPECT_EQ(ToHexString(Hmac<TypeParam>::Digest(vector.key, vector.message)), vector.hex)
        << "key length " << vector.key.size() << ", message length " << vector.message.size();
  }
}

TYPED_TEST(HmacTest, StreamingMatchesOneShotAndPeeks) {
  const HmacVector& vector = HmacTraits<TypeParam>::kVectors[0];
  HmacStreamer<TypeParam> stream(vector.key);
  for (const char& chr : vector.message) {
    stream.Update(std::string_view(&chr, 1));
  }
  EXPECT_EQ(ToHexString(stream.Finalize()), vector.hex);

  constexpr std::size_t kSplit = 10;
  HmacStreamer<TypeParam> peeking(vector.key);
  peeking.Update(vector.message.substr(0, kSplit));
  (void)peeking.Finalize();  // Peek must not disturb the stream.
  peeking.Update(vector.message.substr(kSplit));
  EXPECT_EQ(ToHexString(peeking.Finalize()), vector.hex);
}

TEST(DigestTest, ToHexString) {
  constexpr std::array<uint8_t, 4> kBytes = {0x00, 0x0F, 0xA5, 0xFF};
  EXPECT_EQ(ToHexString(kBytes), "000fa5ff");
}

}  // namespace
}  // namespace mbo::digest
