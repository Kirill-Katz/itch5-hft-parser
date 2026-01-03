#pragma once

#include <cstdint>
#include <chrono>
#include <absl/container/flat_hash_map.h>
#include "array_level.hpp"
#include "parser_v2.hpp"
#include "vector_level.hpp"
#include "vector_level_split_array.hpp"
#include "array_level_binary_search.hpp"
#include "map_level.hpp"
#include "order_book.hpp"

struct OrderBookHandlerSingle {
    uint16_t target_stock_locate = -1;

    void handle_system_event(const ITCHv2::SystemEvent&) {}
    void handle_trading_action(const ITCHv2::TradingAction&) {}
    void handle_reg_sho(const ITCHv2::RegSho&) {}
    void handle_market_participant_pos(const ITCHv2::MarketParticipantPos&) {}
    void handle_mwcb_decline_level(const ITCHv2::MwcbDeclineLevel&) {}
    void handle_mwcb_status(const ITCHv2::MwcbStatus&) {}
    void handle_ipo_quotation_period_upd(const ITCHv2::IpoQuotationPeriodUpd&) {}
    void handle_luld_auction_collar(const ITCHv2::LuldAuctionCollar&) {}
    void handle_operational_halt(const ITCHv2::OperationalHalt&) {}
    void handle_trade_msg_non_cross(const ITCHv2::TradeMessageNonCross&) {}
    void handle_trade_msg_cross(const ITCHv2::TradeMessageCross&) {}
    void handle_broken_trade(const ITCHv2::BrokenTrade&) {}
    void handle_noii(const ITCHv2::Noii&) {}
    void handle_direct_listing_capital_raise(const ITCHv2::DirectListingCapitalRaise&) {}

    void handle_stock_directory(const ITCHv2::StockDirectory&);
    void handle_add_order_no_mpid(const ITCHv2::AddOrderNoMpid&);
    void handle_add_order_mpid(const ITCHv2::AddOrderMpid&);
    void handle_order_executed(const ITCHv2::OrderExecuted&);
    void handle_order_executed_price(const ITCHv2::OrderExecutedPrice&);
    void handle_order_cancel(const ITCHv2::OrderCancel&);
    void handle_order_delete(const ITCHv2::OrderDelete&);
    void handle_order_replace(const ITCHv2::OrderReplace&);

    void handle_after();
    void handle_before();

    using clock = std::chrono::high_resolution_clock;
    OB::OrderBook<OB::VectorLevelSplit> order_book;

    bool touched = false;
    std::chrono::time_point<clock> start;
    absl::flat_hash_map<uint64_t, uint64_t> latency_distribution;
};

inline void OrderBookHandlerSingle::handle_before() {
    touched = false;
    start = clock::now();
}

inline void OrderBookHandlerSingle::handle_after() {
    auto end = clock::now();
    auto latency = std::chrono::round<std::chrono::nanoseconds>(end - start).count();
    if (touched) {
        latency_distribution[latency]++;
    }
}

inline void OrderBookHandlerSingle::handle_stock_directory(const ITCHv2::StockDirectory& msg) {
    if (std::string_view(msg.stock, 8) == "NVDA    ") {
        target_stock_locate = msg.stock_locate;
    }
}

inline void OrderBookHandlerSingle::handle_add_order_no_mpid(const ITCHv2::AddOrderNoMpid& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.add_order(msg.order_reference_number, static_cast<OB::Side>(msg.buy_sell), msg.shares, msg.price);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_add_order_mpid(const ITCHv2::AddOrderMpid& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.add_order(msg.order_reference_number, static_cast<OB::Side>(msg.buy_sell), msg.shares, msg.price);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_order_executed(const ITCHv2::OrderExecuted& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.execute_order(msg.order_reference_number, msg.executed_shares);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_order_executed_price(const ITCHv2::OrderExecutedPrice& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.execute_order(msg.order_reference_number, msg.executed_shares);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_order_cancel(const ITCHv2::OrderCancel& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.cancel_order(msg.order_reference_number, msg.cancelled_shares);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_order_delete(const ITCHv2::OrderDelete& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.delete_order(msg.order_reference_number);
    touched = true;
}

inline void OrderBookHandlerSingle::handle_order_replace(const ITCHv2::OrderReplace& msg) {
    if (msg.stock_locate != target_stock_locate) return;
    order_book.replace_order(msg.order_reference_number, msg.new_reference_number, msg.shares, msg.price);
    touched = true;
}
