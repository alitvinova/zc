// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core_io.h"
#include "chain.h"
//#include "zcbenchmarks.h"

#include "dummysender.h"

#include <stdint.h>
#include <boost/assign/list_of.hpp>
#include <numeric>


#include <cstdio>
#define Z_SENDMANY_MAX_ZADDR_OUTPUTS    ((MAX_TX_SIZE / JSDescription().GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION)) - 1)

#define CTXIN_SPEND_DUST_SIZE   148
#define CTXOUT_REGULAR_SIZE     34


using namespace std;

using namespace libzcash;

extern CAmount AmountFromValue(const UniValue& value);

extern CChain chainActive;
extern CWallet *pwalletMain; //= new CWallet("wallet.dat");
extern ZCJoinSplit *pzcashParams;

extern bool EnsureWalletIsAvailable(bool avoidException);

UniValue ParseNonRFCJSONValue(const std::string& strVal)
{
  UniValue jVal;
  if (!jVal.read(std::string("[")+strVal+std::string("]")) ||
    !jVal.isArray() || jVal.size()!=1)
      throw runtime_error(string("Error parsing JSON:")+strVal);
      return jVal[0];
}


CBitcoinAddress  wallet_test_init()
{
  printf("Starting initialization....\n");
  pwalletMain = new CWallet();
  printf("pwalletMain is created\n");
  bool fFirstRun = true;
  pwalletMain->LoadWallet(fFirstRun);
  printf("Wallet is loaded\n");
  RegisterValidationInterface(pwalletMain);
  printf("Wallet registered\n");
  //auto sk = libzcash::SpendingKey::random();
  //pwalletMain->AddSpendingKey(sk);
  ECC_Start();
  pzcashParams = ZCJoinSplit::Unopened();
  SelectParams(CBaseChainParams::MAIN);
  CPubKey demoPubkey = pwalletMain->GenerateNewKey();
  printf("PubKey is generated...\n");
  CBitcoinAddress demoAddress = CBitcoinAddress(CTxDestination(demoPubkey.GetID()));
  printf("Bitcoin address is created....\n");
  UniValue retValue;
  string strAccount = "";
  string strPurpose = "receive";
  /*Initialize Wallet with an account */
  CWalletDB walletdb(pwalletMain->strWalletFile);
  CAccount account;
  account.vchPubKey = demoPubkey;
  pwalletMain->SetAddressBook(account.vchPubKey.GetID(), strAccount, strPurpose);
  walletdb.WriteAccount(strAccount, account);
  return demoAddress;
}

UniValue mysendmany(const UniValue& params, bool fHelp)
{
    printf("starting mysendmany......\n");
    bool avoidException = true;
    printf("Create Wallet...\n");
    CBitcoinAddress taddr = wallet_test_init(); 
    if (!EnsureWalletIsAvailable(avoidException))
    {
        printf("Wallet is not available...\n");
        return NullUniValue;
    }
    printf("Params size: %u....\n", (int) params.size());
    if (fHelp || params.size() < 2 || params.size() > 4)
        throw runtime_error("Invalid params");

    // Check that the from address is valid.
    auto fromaddress = params[0].get_str();
    bool fromTaddr = false;
    printf("Fromaddr: %s\n", fromaddress.c_str());
    printf("Taddr created...\n");
    fromTaddr = taddr.IsValid();
    
    libzcash::PaymentAddress zaddr;
    printf("Seems good....\n");
    if (!fromTaddr) {
        printf("sending from zaddr.....\n");
        CZCPaymentAddress address(fromaddress);
        try {
            zaddr = address.Get();
        } catch (const std::runtime_error&) {
            // invalid
            printf("exception =(\n");
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid from address, should be a taddr or zaddr.");
        }
        printf("sending from zaddr.....\n");
    }
    else printf("sending from taddr....\n");

    // Check that we have the spending key
    //pwalletMain = new CWallet("wallet.dat");
    //pwalletMain->LoadWallet(fFirstRun);
    //RegisterValidationInterface(pwalletMain);
    
    /*if (!fromTaddr) {
        if (!pwalletMain->HaveSpendingKey(zaddr)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "From address does not belong to this node, zaddr spending key not found.");
        }
    }
    */
    UniValue outputs = params[1].get_array();

    if (outputs.size()==0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, amounts array is empty.");

    // Keep track of addresses to spot duplicates
    set<std::string> setAddress;

    // Recipients
    std::vector<SendManyRecipient> taddrRecipients;
    std::vector<SendManyRecipient> zaddrRecipients;
    CAmount nTotalOut = 0;

    for (const UniValue& o : outputs.getValues()) {
        if (!o.isObject())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");

        // sanity check, report error if unknown key-value pairs
        for (const string& name_ : o.getKeys()) {
            std::string s = name_;
            if (s != "address" && s != "amount" && s!="memo")
                throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, unknown key: ")+s);
        }

        string address = find_value(o, "address").get_str();
        bool isZaddr = false;
        CBitcoinAddress taddr(address);
        if (!taddr.IsValid()) {
            try {
                CZCPaymentAddress zaddr(address);
                zaddr.Get();
                isZaddr = true;
            } catch (const std::runtime_error&) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, unknown address format: ")+address );
            }
        }

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+address);
        setAddress.insert(address);

        UniValue memoValue = find_value(o, "memo");
        string memo;
        if (!memoValue.isNull()) {
            memo = memoValue.get_str();
            if (!isZaddr) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Memo can not be used with a taddr.  It can only be used with a zaddr.");
            } else if (!IsHex(memo)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected memo data in hexadecimal format.");
            }
            if (memo.length() > ZC_MEMO_SIZE*2) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,  strprintf("Invalid parameter, size of memo is larger than maximum allowed %d", ZC_MEMO_SIZE ));
            }
        }

        UniValue av = find_value(o, "amount");
        CAmount nAmount = AmountFromValue( av );
        if (nAmount < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, amount must be positive");

        if (isZaddr) {
            zaddrRecipients.push_back( SendManyRecipient(address, nAmount, memo) );
        } else {
            taddrRecipients.push_back( SendManyRecipient(address, nAmount, memo) );
        }

        nTotalOut += nAmount;
    }

    // Check the number of zaddr outputs does not exceed the limit.
    if (zaddrRecipients.size() > Z_SENDMANY_MAX_ZADDR_OUTPUTS)  {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, too many zaddr outputs");
    }

    // As a sanity check, estimate and verify that the size of the transaction will be valid.
    // Depending on the input notes, the actual tx size may turn out to be larger and perhaps invalid.
    size_t txsize = 0;
    CMutableTransaction mtx;
    mtx.nVersion = 2;
    for (int i = 0; i < zaddrRecipients.size(); i++) {
        mtx.vjoinsplit.push_back(JSDescription());
    }
    CTransaction tx(mtx);
    txsize += tx.GetSerializeSize(SER_NETWORK, tx.nVersion);
    if (fromTaddr) {
        txsize += CTXIN_SPEND_DUST_SIZE;
        txsize += CTXOUT_REGULAR_SIZE;      // There will probably be taddr change
    }
    txsize += CTXOUT_REGULAR_SIZE * taddrRecipients.size();
    if (txsize > MAX_TX_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Too many outputs, size of raw transaction would be larger than limit of %d bytes", MAX_TX_SIZE ));
    }

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 2) {
        nMinDepth = params[2].get_int();
    }
    if (nMinDepth < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Minimum number of confirmations cannot be less than 0");
    }

    // Fee in Zatoshis, not currency format)
    CAmount nFee = ASYNC_RPC_OPERATION_DEFAULT_MINERS_FEE;
    if (params.size() > 3) {
        if (params[3].get_real() == 0.0) {
            nFee = 0;
        } else {
            nFee = AmountFromValue( params[3] );
        }

        // Check that the user specified fee is sane.
        if (nFee > nTotalOut) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Fee %s is greater than the sum of outputs %s", FormatMoney(nFee), FormatMoney(nTotalOut)));
        }
    }

    // Use input parameters as the optional context info to be returned by z_getoperationstatus and z_getoperationresult.
    UniValue o(UniValue::VOBJ);
    o.push_back(Pair("fromaddress", params[0]));
    o.push_back(Pair("amounts", params[1]));
    o.push_back(Pair("minconf", nMinDepth));
    o.push_back(Pair("fee", std::stod(FormatMoney(nFee))));
    UniValue contextInfo = o;

    printf("Everything seems good....\n");
    return 0;
    // Create operation and add to global queue
    CSender sender(fromaddress, taddrRecipients, zaddrRecipients, nMinDepth, nFee, contextInfo);
    sender.create_tx();
    return sender.GetResult();
    return 0;
}

int main()
{
  printf("starting main......\n");
  //CZCPaymentAddress pa = pwalletMain->GenerateNewZKey();
  std::string zaddr1 = "tmQP9L3s31cLsghVYf2Jb5MhKj1jRBPoeQn";//pa.ToString();
  std::string callstr = std::string("tmRr6yJonqGK23UVhrKuyvTpF8qxQQjKigJ ") + std::string("[{\"address\":\"") + zaddr1 + std::string("\", \"amount\":123.456}]");
  std::string fromaddr = "tmRr6yJonqGK23UVhrKuyvTpF8qxQQjKigJ";
  std::string toaddr = "[{\"address\":\"tmQP9L3s31cLsghVYf2Jb5MhKj1jRBPoeQn\", \"amount\":50.0}]";
  UniValue params(UniValue::VARR);
  params.push_back(fromaddr);
  params.push_back(ParseNonRFCJSONValue(toaddr));
  mysendmany(params, 0);
  return 0;
}
