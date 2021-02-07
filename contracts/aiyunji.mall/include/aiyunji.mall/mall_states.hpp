 #pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>

#include <deque>
#include <optional>
#include <string>
#include <type_traits>

namespace ayj {

using namespace std;
using namespace eosio;
using uint256_t = fixed_bytes<32>;

static constexpr eosio::name active_perm{"active"_n};
static constexpr eosio::name SYS_BANK{"eosio.token"_n};

static constexpr symbol   SYS_SYMBOL            = symbol(symbol_code("MGP"), 4);
static constexpr symbol   HST_SYMBOL            = symbol(symbol_code("HST"), 4);
static constexpr uint32_t seconds_per_year      = 24 * 3600 * 7 * 52;
static constexpr uint32_t seconds_per_month     = 24 * 3600 * 30;
static constexpr uint32_t seconds_per_week      = 24 * 3600 * 7;
static constexpr uint32_t seconds_per_day       = 24 * 3600;
static constexpr uint32_t seconds_per_hour      = 3600;
static constexpr uint32_t ratio_boost           = 10000;
static constexpr uint32_t MAX_STEP              = 20;

// static uint256_t uint256_default      =  uint256_t::make_from_word_sequence<uint64_t>(0ULL,0ULL,0ULL,0ULL);

#define CONTRACT_TBL [[eosio::table, eosio::contract("aiyunji.mall")]]

struct [[eosio::table("config"), eosio::contract("aiyunji.mall")]] config_t {
    uint16_t withdraw_fee_ratio         = 3000;  //boost by 10000
    uint16_t withdraw_mature_days       = 1;
    vector<uint64_t> allocation_ratios  = {
        4500,   //0: user share
        1500,   //1: shop_sunshine share
        500,    //2: shop_top share
        800,    //3: certified share
        500,    //4: pltform_top share
        1000,   //5: direct_referral share
        500,    //6: direct_agent share
        300,    //7: city_center share
        400     //8: ram_usage share
    };
    uint16_t platform_top_count         = 1000;
    name platform_admin;
    name platform_account;
    name mall_bank;  //bank for the mall

    config_t() {}

    EOSLIB_SERIALIZE(config_t,  (withdraw_fee_ratio)(withdraw_mature_days)(allocation_ratios)
                                (platform_top_count)(platform_admin)(platform_account)(mall_bank) )
        
};
typedef eosio::singleton< "config"_n, config_t > config_singleton;

struct platform_share_t {
    asset total_share               = asset(0, HST_SYMBOL);
    asset top_share                 = asset(0, HST_SYMBOL);
    asset ram_usage_share           = asset(0, HST_SYMBOL);
    asset lucky_draw_share          = asset(0, HST_SYMBOL);
    asset certified_user_share      = asset(0, HST_SYMBOL);
    uint64_t certified_user_count   = 0;

    void reset() {
        total_share.amount          = 0;
        top_share.amount            = 0;
        ram_usage_share.amount      = 0;
        lucky_draw_share.amount     = 0;
        certified_user_share.amount = 0;
        certified_user_count        = 0;
    }
};

struct [[eosio::table("global"), eosio::contract("aiyunji.mall")]] global_t {
    bool executing = false;
    platform_share_t platform_share;
    platform_share_t platform_share_cache; //cached in executing

    global_t() {}

    EOSLIB_SERIALIZE(global_t,  (executing)(platform_share)(platform_share_cache) )
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;

struct [[eosio::table("global2"), eosio::contract("aiyunji.mall")]] global2_t {
    uint64_t last_reward_shop_id                = 0;
    uint64_t last_sunshine_reward_spending_id   = 0;
    uint64_t last_platform_top_reward_step      = 0;
    uint128_t last_platform_top_reward_id       = 0;    //user_t idx id
    uint64_t last_certified_reward_step         = 0;
    time_point_sec last_shop_reward_finished_at;
    time_point_sec last_certified_reward_finished_at;
    time_point_sec last_platform_reward_finished_at;

    global2_t() {}

    EOSLIB_SERIALIZE(global2_t, (last_reward_shop_id)(last_sunshine_reward_spending_id)(last_platform_top_reward_step)(last_platform_top_reward_id)
                                (last_shop_reward_finished_at)(last_certified_reward_finished_at)(last_platform_reward_finished_at) )
                                
};
typedef eosio::singleton< "global2"_n, global2_t > global2_singleton;

struct CONTRACT_TBL citycenter_t {
    uint64_t id;
    name citycenter_name;
    name citycenter_account;
    asset share;
    time_point_sec created_at;
    time_point_sec updated_at;

    citycenter_t() {}
    citycenter_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    uint64_t name_key() const { return citycenter_name.value; }

    typedef eosio::multi_index<"citycenters"_n, citycenter_t,
        indexed_by<"ccname"_n, const_mem_fun<citycenter_t, uint64_t, &citycenter_t::name_key> >
    > tbl_t;

    EOSLIB_SERIALIZE( citycenter_t, (id)(citycenter_name)(citycenter_account)(share)
                                    (created_at)(updated_at) )
};

struct CONTRACT_TBL user_share_t {
    asset spending_reward           = asset(0, HST_SYMBOL);
    asset customer_referral_reward  = asset(0, HST_SYMBOL);
    asset shop_referral_reward      = asset(0, HST_SYMBOL);

    asset total_rewards() const { //used for top 1000 split
        return spending_reward + customer_referral_reward + shop_referral_reward; 
    }
};

struct CONTRACT_TBL user_t {
    name account;
    name referral_account;
    user_share_t share;
    user_share_t share_cache;
    time_point_sec created_at;
    time_point_sec updated_at;

    user_t(){}
    user_t(const name& a): account(a) {}
 
    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return account.value; }

    uint128_t by_total_rewards() const {
        auto total = share_cache.total_rewards();
        return (uint128_t (std::numeric_limits<uint64_t>::max() - total.amount)) << 64 | account.value; 
    }

    typedef eosio::multi_index<"users"_n, user_t,
        indexed_by<"totalrewards"_n, const_mem_fun<user_t, uint128_t, &user_t::by_total_rewards> >
    > tbl_t;

    EOSLIB_SERIALIZE( user_t,   (account)(referral_account)(share)(share_cache)
                                (created_at)(updated_at) )
};

struct CONTRACT_TBL shop_share_t {
    asset total_spending                    = asset(0, HST_SYMBOL);
    asset day_spending                      = asset(0, HST_SYMBOL);
    asset sunshine_share                    = asset(0, HST_SYMBOL);
    asset top_share                         = asset(0, HST_SYMBOL);

    void reset() {
        day_spending.amount = 0;
        sunshine_share.amount = 0;
        top_share.amount = 0;
    }
};

struct CONTRACT_TBL shop_t {
    uint64_t id;                //shop_id
    uint64_t parent_id;         //0 means top
    uint64_t citycenter_id;
    name shop_account;          //shop funds account
    name referral_account;
    uint64_t top_reward_count               = 10;   //can be customized by shop admin
    uint64_t top_rewarded_count             = 0;
    uint256_t last_sunshine_reward_spend_idx;// = uint256_default; //spending_t::id
    uint256_t last_top_reward_spend_idx;     //= uint256_default; //spending_t::id
    shop_share_t share;
    shop_share_t share_cache;
    time_point_sec created_at;
    time_point_sec updated_at;

    shop_t() {}
    shop_t(const uint64_t& i): id(i) {}

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }
    uint64_t by_parent_id() const { return parent_id; }
    uint64_t by_referral() const { return referral_account.value; }
    typedef eosio::multi_index< "shops"_n, shop_t,
        indexed_by<"parentid"_n, const_mem_fun<shop_t, uint64_t, &shop_t::by_parent_id> >,
        indexed_by<"referrer"_n, const_mem_fun<shop_t, uint64_t, &shop_t::by_referral> >
    > tbl_t;

    EOSLIB_SERIALIZE( shop_t,   (id)(parent_id)(citycenter_id)(shop_account)(referral_account)
                                (top_reward_count)(top_rewarded_count)(last_sunshine_reward_spend_idx)(last_top_reward_spend_idx)
                                (share)(share_cache)(created_at)(updated_at) )
};

struct CONTRACT_TBL spending_share_t {
    asset day_spending;     //accumulated for day(s), reset after reward
    asset total_spending;   //accumulated continuously, never reset
};

struct CONTRACT_TBL spending_t {
    uint64_t id;
    uint64_t shop_id;
    name customer;
    spending_share_t share;
    spending_share_t share_cache;
    time_point_sec created_at;

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return id; }

    ///to ensure uniquness
    uint128_t shop_customer_key() const { return ((uint128_t) shop_id) << 64 | customer.value; }
    ///to derive platform top 1000, across shops
    uint128_t by_total_spending() const { return (uint128_t (std::numeric_limits<uint64_t>::max() - share_cache.total_spending.amount)) << 64 | id; }
    ///to derive shop top 10 and all
    uint256_t by_shop_day_spending() const  { return uint256_t::make_from_word_sequence<uint64_t>(shop_id, std::numeric_limits<uint64_t>::max() - share_cache.day_spending.amount, id, 0ULL); }

    typedef eosio::multi_index
    < "spends"_n, spending_t,
        indexed_by<"shopcustidx"_n, const_mem_fun<spending_t, uint128_t, &spending_t::shop_customer_key> >,
        indexed_by<"spendidx"_n,    const_mem_fun<spending_t, uint128_t, &spending_t::by_total_spending> >,
        indexed_by<"shopspends"_n,  const_mem_fun<spending_t, uint256_t, &spending_t::by_shop_day_spending> >
    > tbl_t;

    EOSLIB_SERIALIZE( spending_t, (id)(shop_id)(customer)(share)(share_cache)(created_at) )
};

struct CONTRACT_TBL certification_t {
    name user;
    time_point_sec certified_at;

    certification_t() {}
    certification_t(const name& u): user(u) { certified_at = time_point_sec( current_time_point() ); }

    uint64_t scope() const { return 0; }
    uint64_t primary_key() const { return user.value; }

    typedef eosio::multi_index<"certify"_n, certification_t> tbl_t;

    EOSLIB_SERIALIZE( certification_t, (user)(certified_at) )
};


} //end of namespace ayj