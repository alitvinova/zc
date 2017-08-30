#include "clientversion.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "key.h"
#include "keystore.h"
#include "script/script.h"
#include "script/script_error.h"
#include "primitives/transaction.h"

#include "main.h"

#include <map>
#include <string>

/*
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
*/

#include <univalue.h>

#include "zcash/Note.hpp"
#include "zcash/Address.hpp"
#include "zcash/Proof.hpp"

#include <openssl/crypto.h>

void memory_cleanse(void *ptr, size_t len)
{
  OPENSSL_cleanse(ptr, len);
}

using namespace std;

int main()
{
   printf("Lets begin\n");
  //auto p = ZCJoinSplit::Generate();

  //printf("JS generated\n");

  // construct a merkle tree
  ZCIncrementalMerkleTree merkleTree;
  libzcash::SpendingKey k = libzcash::SpendingKey::random();
  libzcash::PaymentAddress addr = k.address();

  printf("Merkle tree created\n");

  libzcash::Note note(addr.a_pk, 100, uint256(), uint256());

  // commitment from coin
  uint256 commitment = note.cm();

  // insert commitment into the merkle tree
  merkleTree.append(commitment);

  // compute the merkle root we will be working with
  uint256 rt = merkleTree.root();

  auto witness = merkleTree.witness();

  // create JSDescription
  uint256 pubKeyHash;
  boost::array<libzcash::JSInput, ZC_NUM_JS_INPUTS> inputs = {
    libzcash::JSInput(witness, note, k),
    libzcash::JSInput() // dummy input of zero value
  };
  boost::array<libzcash::JSOutput, ZC_NUM_JS_OUTPUTS> outputs = {
    libzcash::JSOutput(addr, 50),
    libzcash::JSOutput(addr, 50)
  };
  return 0;
}
