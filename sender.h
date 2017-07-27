// Copyright (c) 2016 The Zcash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DO_SENDMANY_H
#define DO_SENDMANY_H

#include "asyncrpcoperation.h"
#include "amount.h"
#include "base58.h"
#include "primitives/transaction.h"
#include "zcash/JoinSplit.hpp"
#include "zcash/Address.hpp"
#include "wallet.h"

#include <unordered_map>
#include <tuple>

#include <univalue.h>

// Default transaction fee if caller does not specify one.
#define ASYNC_RPC_OPERATION_DEFAULT_MINERS_FEE   10000

using namespace libzcash;

// A recipient is a tuple of address, amount, memo (optional if zaddr)
typedef std::tuple<std::string, CAmount, std::string> SendManyRecipient;

// Input UTXO is a tuple (quadruple) of txid, vout, amount, coinbase)
typedef std::tuple<uint256, int, CAmount, bool> SendManyInputUTXO;

// Input JSOP is a tuple of JSOutpoint, note and amount
typedef std::tuple<JSOutPoint, Note, CAmount> SendManyInputJSOP;

// Package of info which is passed to perform_joinsplit methods.
struct AsyncJoinSplitInfo
{
    std::vector<JSInput> vjsin;
    std::vector<JSOutput> vjsout;
    std::vector<Note> notes;
    CAmount vpub_old = 0;
    CAmount vpub_new = 0;
};

// A struct to help us track the witness and anchor for a given JSOutPoint
struct WitnessAnchorData {
	boost::optional<ZCIncrementalWitness> witness;
	uint256 anchor;
};

class CSender : public AsyncRPCOperation {
public:
    CSender(std::string fromAddress, std::vector<SendManyRecipient> tOutputs, std::vector<SendManyRecipient> zOutputs, int minDepth, CAmount fee = ASYNC_RPC_OPERATION_DEFAULT_MINERS_FEE, UniValue contextInfo = NullUniValue);
    virtual ~CSender();
    
    // We don't want to be copied or moved around
    CSender(CSender const&) = delete;             // Copy construct
    CSender(CSender&&) = delete;                  // Move construct
    CSender& operator=(CSender const&) = delete;  // Copy assign
    CSender& operator=(CSender &&) = delete;      // Move assign
    
    virtual void main();

    virtual UniValue getStatus() const;

    bool testmode = false;  // Set to true to disable sending txs and generating proofs
    
    std::string GetResult(){return result_;};

private:
    friend class TEST_FRIEND_AsyncRPCOperation_sendmany;    // class for unit testing

    UniValue contextinfo_;     // optional data to include in return value from getStatus()

    CAmount fee_;
    int mindepth_;
    std::string fromaddress_;
    bool isfromtaddr_;
    bool isfromzaddr_;
    CBitcoinAddress fromtaddr_;
    PaymentAddress frompaymentaddress_;
    SpendingKey spendingkey_;
    std::string result_;
    
    uint256 joinSplitPubKey_;
    unsigned char joinSplitPrivKey_[crypto_sign_SECRETKEYBYTES];

    // The key is the result string from calling JSOutPoint::ToString()
    std::unordered_map<std::string, WitnessAnchorData> jsopWitnessAnchorMap;

    std::vector<SendManyRecipient> t_outputs_;
    std::vector<SendManyRecipient> z_outputs_;
    std::vector<SendManyInputUTXO> t_inputs_;
    std::vector<SendManyInputJSOP> z_inputs_;
    
    CTransaction tx_;
   
    void add_taddr_change_output_to_tx(CAmount amount);
    void add_taddr_outputs_to_tx();
    bool find_unspent_notes();
    bool find_utxos(bool fAcceptCoinbase);
    boost::array<unsigned char, ZC_MEMO_SIZE> get_memo_from_hex_string(std::string s);
    bool main_impl();

    // JoinSplit without any input notes to spend
    UniValue perform_joinsplit(AsyncJoinSplitInfo &);

    // JoinSplit with input notes to spend (JSOutPoints))
    UniValue perform_joinsplit(AsyncJoinSplitInfo &, std::vector<JSOutPoint> & );

    // JoinSplit where you have the witnesses and anchor
    UniValue perform_joinsplit(
        AsyncJoinSplitInfo & info,
        std::vector<boost::optional < ZCIncrementalWitness>> witnesses,
        uint256 anchor);

    void sign_send_raw_transaction(UniValue obj);     // throws exception if there was an error

};

#endif /* DO_SENDMANY_H */

