#pragma once

#include <cstddef>
#include <stdint.h>
#include <cstring>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ITCHv2 {

struct be48{};

template<typename>
struct member_pointer_traits;

template<typename T, typename C>
struct member_pointer_traits<T C::*> {
    using member_type = T;
    using class_type = C;
};

template<auto Member>
using member_type_t =
    typename member_pointer_traits<
        decltype(Member)
    >::member_type;


template<auto Member, typename Encoding = void>
struct Field {
    using type = member_type_t<Member>;
    using encoding = Encoding;
    static constexpr auto member = Member;

    static_assert(
        std::is_void_v<Encoding> ||
        (std::is_same_v<Encoding, be48> && std::is_same_v<type, uint64_t>),
        "be48 encoding is only valid for an uint64_t storage"
    );

    static constexpr size_t size =
        std::is_same_v<Encoding, be48> ? 6 :
        sizeof(type);
};

template<size_t I, typename Layout>
struct OffsetAt {
    static constexpr size_t value =
        OffsetAt<I - 1, Layout>::value +
        std::tuple_element_t<I - 1, Layout>::size;
};

template<typename Layout>
struct OffsetAt<0, Layout> {
    static constexpr size_t value = 0;
};

template<typename T>
inline T load_be(const std::byte* p);

template<>
inline char load_be<char>(const std::byte* p) {
    return static_cast<char>(*p);
}

template<>
inline uint16_t load_be<uint16_t>(const std::byte* p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

template<>
inline uint32_t load_be<uint32_t>(const std::byte* p) {
    return (uint32_t(p[0]) << 24) |
           (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8)  |
           uint32_t(p[3]);
}

template<>
inline uint64_t load_be<uint64_t>(const std::byte* p) {
    return (uint64_t(p[0]) << 56) |
           (uint64_t(p[1]) << 48) |
           (uint64_t(p[2]) << 40) |
           (uint64_t(p[3]) << 32) |
           (uint64_t(p[4]) << 24) |
           (uint64_t(p[5]) << 16) |
           (uint64_t(p[6]) << 8)  |
           uint64_t(p[7]);
}

inline uint64_t load_be(const std::byte* p, be48 encoding) {
    uint64_t x;
    __builtin_memcpy(&x, p, 8);
    x = __builtin_bswap64(x);
    return x & 0x0000FFFFFFFFFFFFULL;
}

template<typename Msg, auto Member>
inline void load_into(
    Msg& m,
    const std::byte* p,
    Field<Member, void>
)
requires (std::is_array_v<member_type_t<Member>>)
{
    using T = member_type_t<Member>;
    constexpr size_t N = std::extent_v<T>;
    std::memcpy(m.*Member, p, N); // for char[N]
}

template<typename Msg, auto Member, typename Encoding>
inline void load_into(
    Msg& m,
    const std::byte* p,
    Field<Member, Encoding>
)
requires (!std::is_array_v<member_type_t<Member>>)
{
    using T = member_type_t<Member>;

    if constexpr (std::is_void_v<Encoding>) {
        m.*Member = load_be<T>(p); // for char and the rest of the fields
    } else {
        m.*Member = load_be(p, Encoding{}); // for 48 bit fields
    }
}

template<typename Layout, typename Msg, size_t... I>
__attribute__((hot))
inline void parse_impl(
    Msg& m,
    const std::byte* src,
    std::index_sequence<I...>
) {
    (
        load_into(
            m,
            src + OffsetAt<I, Layout>::value,
            std::tuple_element_t<I, Layout>{}
        ),
        ...
    );
}

template<typename Layout, typename Msg>
__attribute__((hot))
inline Msg parse_itch(const std::byte* src) {
    Msg m;
    parse_impl<Layout, Msg>(
        m,
        src,
        std::make_index_sequence<std::tuple_size_v<Layout>>{}
    );
    return m;
}

enum class MessageType {
    SYSTEM_EVENT                = 'S',
    STOCK_DIRECTORY             = 'R',
    STOCK_TRADING_ACTION        = 'H',
    REG_SHO                     = 'Y',
    MARKET_PARTICIPANT_POSITION = 'L',
    MWCB_DECLINE_LEVEL_MESSAGE  = 'V',
    MWCB_STATUS_MESSAGE         = 'W',
    IPO_QUOTING_PERIOD_UPD      = 'K',
    LULD_AUCTION_COLLAR         = 'J',
    OPERATIONAL_HALT            = 'h',

    ADD_ORDER_NO_MPID = 'A',
    ADD_ORDER_MPID    = 'F',

    ORDER_EXECUTED       = 'E',
    ORDER_EXECUTED_PRICE = 'C',
    ORDER_CANCEL         = 'X',
    ORDER_DELETE         = 'D',
    ORDER_REPLACE        = 'U',

    NON_CROSS_TRADE_MSG = 'P',
    CROSS_TRADE_MSG     = 'Q',
    BROKEN_TRADE_MSG    = 'B',

    NOII_MSG = 'I',

    DIRECT_LISTING_CAPITAL_RAISE = 'O',
};

#define ITCH_MESSAGE_LIST(X) \
    X(SYSTEM_EVENT,                 SystemEvent,                 system_event,                  unlikely) \
    X(STOCK_DIRECTORY,              StockDirectory,              stock_directory,               unlikely) \
    X(STOCK_TRADING_ACTION,         TradingAction,               trading_action,                unlikely) \
    X(REG_SHO,                      RegSho,                      reg_sho,                       unlikely) \
    X(MARKET_PARTICIPANT_POSITION,  MarketParticipantPos,        market_participant_pos,        unlikely) \
    X(MWCB_DECLINE_LEVEL_MESSAGE,   MwcbDeclineLevel,            mwcb_decline_level,            unlikely) \
    X(MWCB_STATUS_MESSAGE,          MwcbStatus,                  mwcb_status,                   unlikely) \
    X(IPO_QUOTING_PERIOD_UPD,       IpoQuotationPeriodUpd,       ipo_quotation_period_upd,      unlikely) \
    X(LULD_AUCTION_COLLAR,          LuldAuctionCollar,           luld_auction_collar,           unlikely) \
    X(OPERATIONAL_HALT,             OperationalHalt,             operational_halt,              unlikely) \
    X(ADD_ORDER_NO_MPID,            AddOrderNoMpid,              add_order_no_mpid,             likely) \
    X(ADD_ORDER_MPID,               AddOrderMpid,                add_order_mpid,                likely) \
    X(ORDER_EXECUTED,               OrderExecuted,               order_executed,                likely) \
    X(ORDER_EXECUTED_PRICE,         OrderExecutedPrice,          order_executed_price,          likely) \
    X(ORDER_CANCEL,                 OrderCancel,                 order_cancel,                  likely) \
    X(ORDER_DELETE,                 OrderDelete,                 order_delete,                  likely) \
    X(ORDER_REPLACE,                OrderReplace,                order_replace,                 likely) \
    X(NON_CROSS_TRADE_MSG,          TradeMessageNonCross,        trade_msg_non_cross,           unlikely) \
    X(CROSS_TRADE_MSG,              TradeMessageCross,           trade_msg_cross,               unlikely) \
    X(BROKEN_TRADE_MSG,             BrokenTrade,                 broken_trade,                  unlikely) \
    X(NOII_MSG,                     Noii,                        noii,                          unlikely) \
    X(DIRECT_LISTING_CAPITAL_RAISE, DirectListingCapitalRaise,   direct_listing_capital_raise,  unlikely)

struct SystemEvent {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     event_code;
};

using SystemEventLayout = std::tuple<
    Field<&SystemEvent::stock_locate>,
    Field<&SystemEvent::tracking_number>,
    Field<&SystemEvent::timestamp, be48>,
    Field<&SystemEvent::event_code>
>;

struct StockDirectory {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char  stock[8];
    char  market_category;
    char  financial_status_indicator;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_classification;
    char issue_sub_type[2];
    char authenticity;
    char short_sale_threshold_indicator;
    char ipo_flag;
    char luld_reference_price_tier;
    char etp_flag;
    uint32_t etp_leverage_factor;
    char inverse_indicator;
};

using StockDirectoryLayout = std::tuple<
    Field<&StockDirectory::stock_locate>,
    Field<&StockDirectory::tracking_number>,
    Field<&StockDirectory::timestamp, be48>,
    Field<&StockDirectory::stock>,
    Field<&StockDirectory::market_category>,
    Field<&StockDirectory::financial_status_indicator>,
    Field<&StockDirectory::round_lot_size>,
    Field<&StockDirectory::round_lots_only>,
    Field<&StockDirectory::issue_classification>,
    Field<&StockDirectory::issue_sub_type>,
    Field<&StockDirectory::authenticity>,
    Field<&StockDirectory::short_sale_threshold_indicator>,
    Field<&StockDirectory::ipo_flag>,
    Field<&StockDirectory::luld_reference_price_tier>,
    Field<&StockDirectory::etp_flag>,
    Field<&StockDirectory::etp_leverage_factor>,
    Field<&StockDirectory::inverse_indicator>
>;

struct TradingAction {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     trading_state;
    char     reserved;
    char     reason[4];
};

using TradingActionLayout = std::tuple<
    Field<&TradingAction::stock_locate>,
    Field<&TradingAction::tracking_number>,
    Field<&TradingAction::timestamp, be48>,
    Field<&TradingAction::stock>,
    Field<&TradingAction::trading_state>,
    Field<&TradingAction::reserved>,
    Field<&TradingAction::reason>
>;

struct RegSho {
    uint16_t locate_code;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     reg_sho_action;
};

using RegShoLayout = std::tuple<
    Field<&RegSho::locate_code>,
    Field<&RegSho::tracking_number>,
    Field<&RegSho::timestamp, be48>,
    Field<&RegSho::stock>,
    Field<&RegSho::reg_sho_action>
>;

struct MarketParticipantPos {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     mpid[4];
    char     stock[8];
    char     primary_market_maker;
    char     market_maker_mode;
    char     market_participant_state;
};

using MarketParticipantPosLayout = std::tuple<
    Field<&MarketParticipantPos::stock_locate>,
    Field<&MarketParticipantPos::tracking_number>,
    Field<&MarketParticipantPos::timestamp, be48>,
    Field<&MarketParticipantPos::mpid>,
    Field<&MarketParticipantPos::stock>,
    Field<&MarketParticipantPos::primary_market_maker>,
    Field<&MarketParticipantPos::market_maker_mode>,
    Field<&MarketParticipantPos::market_participant_state>
>;

struct MwcbDeclineLevel {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t level1;
    uint64_t level2;
    uint64_t level3;
};

using MwcbDeclineLevelLayout = std::tuple<
    Field<&MwcbDeclineLevel::stock_locate>,
    Field<&MwcbDeclineLevel::tracking_number>,
    Field<&MwcbDeclineLevel::timestamp, be48>,
    Field<&MwcbDeclineLevel::level1>,
    Field<&MwcbDeclineLevel::level2>,
    Field<&MwcbDeclineLevel::level3>
>;

struct MwcbStatus {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     breached_level;
};

using MwcbStatusLayout = std::tuple<
    Field<&MwcbStatus::stock_locate>,
    Field<&MwcbStatus::tracking_number>,
    Field<&MwcbStatus::timestamp, be48>,
    Field<&MwcbStatus::breached_level>
>;

struct IpoQuotationPeriodUpd {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    uint32_t ipo_quotation_release_time;
    char     ipo_quatation_release_qualifier;
    uint32_t ipo_price;
};

using IpoQuotationPeriodUpdLayout = std::tuple<
    Field<&IpoQuotationPeriodUpd::stock_locate>,
    Field<&IpoQuotationPeriodUpd::tracking_number>,
    Field<&IpoQuotationPeriodUpd::timestamp, be48>,
    Field<&IpoQuotationPeriodUpd::stock>,
    Field<&IpoQuotationPeriodUpd::ipo_quotation_release_time>,
    Field<&IpoQuotationPeriodUpd::ipo_quatation_release_qualifier>,
    Field<&IpoQuotationPeriodUpd::ipo_price>
>;

struct LuldAuctionCollar {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    uint32_t auction_collar_reference_price;
    uint32_t upper_auction_collar_price;
    uint32_t lower_auction_collar_price;
    uint32_t auction_collar_extension;
};

using LuldAuctionCollarLayout = std::tuple<
    Field<&LuldAuctionCollar::stock_locate>,
    Field<&LuldAuctionCollar::tracking_number>,
    Field<&LuldAuctionCollar::timestamp, be48>,
    Field<&LuldAuctionCollar::stock>,
    Field<&LuldAuctionCollar::auction_collar_reference_price>,
    Field<&LuldAuctionCollar::upper_auction_collar_price>,
    Field<&LuldAuctionCollar::lower_auction_collar_price>,
    Field<&LuldAuctionCollar::auction_collar_extension>
>;

struct OperationalHalt {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    char     stock[8];
    char     market_code;
    char     operational_halt_action;
};

using OperationalHaltLayout = std::tuple<
    Field<&OperationalHalt::stock_locate>,
    Field<&OperationalHalt::tracking_number>,
    Field<&OperationalHalt::timestamp, be48>,
    Field<&OperationalHalt::stock>,
    Field<&OperationalHalt::market_code>,
    Field<&OperationalHalt::operational_halt_action>
>;

struct AddOrderNoMpid {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
};

using AddOrderNoMpidLayout = std::tuple<
    Field<&AddOrderNoMpid::stock_locate>,
    Field<&AddOrderNoMpid::tracking_number>,
    Field<&AddOrderNoMpid::timestamp, be48>,
    Field<&AddOrderNoMpid::order_reference_number>,
    Field<&AddOrderNoMpid::buy_sell>,
    Field<&AddOrderNoMpid::shares>,
    Field<&AddOrderNoMpid::stock>,
    Field<&AddOrderNoMpid::price>
>;

struct AddOrderMpid {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
    char     attribution[4];
};

using AddOrderMpidLayout = std::tuple<
    Field<&AddOrderMpid::stock_locate>,
    Field<&AddOrderMpid::tracking_number>,
    Field<&AddOrderMpid::timestamp, be48>,
    Field<&AddOrderMpid::order_reference_number>,
    Field<&AddOrderMpid::buy_sell>,
    Field<&AddOrderMpid::shares>,
    Field<&AddOrderMpid::stock>,
    Field<&AddOrderMpid::price>,
    Field<&AddOrderMpid::attribution>
>;

struct OrderExecuted {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
};

using OrderExecutedLayout = std::tuple<
    Field<&OrderExecuted::stock_locate>,
    Field<&OrderExecuted::tracking_number>,
    Field<&OrderExecuted::timestamp, be48>,
    Field<&OrderExecuted::order_reference_number>,
    Field<&OrderExecuted::executed_shares>,
    Field<&OrderExecuted::match_number>
>;

struct OrderExecutedPrice {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    char     printable;
    uint32_t execution_price;
};

using OrderExecutedPriceLayout = std::tuple<
    Field<&OrderExecutedPrice::stock_locate>,
    Field<&OrderExecutedPrice::tracking_number>,
    Field<&OrderExecutedPrice::timestamp, be48>,
    Field<&OrderExecutedPrice::order_reference_number>,
    Field<&OrderExecutedPrice::executed_shares>,
    Field<&OrderExecutedPrice::match_number>,
    Field<&OrderExecutedPrice::printable>,
    Field<&OrderExecutedPrice::execution_price>
>;

struct OrderCancel {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint32_t cancelled_shares;
};

using OrderCancelLayout = std::tuple<
    Field<&OrderCancel::stock_locate>,
    Field<&OrderCancel::tracking_number>,
    Field<&OrderCancel::timestamp, be48>,
    Field<&OrderCancel::order_reference_number>,
    Field<&OrderCancel::cancelled_shares>
>;

struct OrderDelete {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
};

using OrderDeleteLayout = std::tuple<
    Field<&OrderDelete::stock_locate>,
    Field<&OrderDelete::tracking_number>,
    Field<&OrderDelete::timestamp, be48>,
    Field<&OrderDelete::order_reference_number>
>;

struct OrderReplace {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    uint64_t new_reference_number;
    uint32_t shares;
    uint32_t price;
};

using OrderReplaceLayout = std::tuple<
    Field<&OrderReplace::stock_locate>,
    Field<&OrderReplace::tracking_number>,
    Field<&OrderReplace::timestamp, be48>,
    Field<&OrderReplace::order_reference_number>,
    Field<&OrderReplace::new_reference_number>,
    Field<&OrderReplace::shares>,
    Field<&OrderReplace::price>
>;

struct TradeMessageNonCross {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t order_reference_number;
    char     buy_sell;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
    uint64_t match_number;
};

using TradeMessageNonCrossLayout = std::tuple<
    Field<&TradeMessageNonCross::stock_locate>,
    Field<&TradeMessageNonCross::tracking_number>,
    Field<&TradeMessageNonCross::timestamp, be48>,
    Field<&TradeMessageNonCross::order_reference_number>,
    Field<&TradeMessageNonCross::buy_sell>,
    Field<&TradeMessageNonCross::shares>,
    Field<&TradeMessageNonCross::stock>,
    Field<&TradeMessageNonCross::price>,
    Field<&TradeMessageNonCross::match_number>
>;

struct TradeMessageCross {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint32_t shares;
    char     stock[8];
    uint32_t cross_price;
    uint64_t match_number;
    char     cross_type;
};

using TradeMessageCrossLayout = std::tuple<
    Field<&TradeMessageCross::stock_locate>,
    Field<&TradeMessageCross::tracking_number>,
    Field<&TradeMessageCross::timestamp, be48>,
    Field<&TradeMessageCross::shares>,
    Field<&TradeMessageCross::stock>,
    Field<&TradeMessageCross::cross_price>,
    Field<&TradeMessageCross::match_number>,
    Field<&TradeMessageCross::cross_type>
>;

struct BrokenTrade {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;
    uint64_t match_number;
};

using BrokenTradeLayout = std::tuple<
    Field<&BrokenTrade::stock_locate>,
    Field<&BrokenTrade::tracking_number>,
    Field<&BrokenTrade::timestamp, be48>,
    Field<&BrokenTrade::match_number>
>;

struct Noii {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;

    uint64_t paired_shares;
    uint64_t imbalance_shares;
    char     imbalance_direction;

    char     stock[8];
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_reference_price;
    char     cross_type;
    char     price_variation_indicator;
};

using NoiiLayout = std::tuple<
    Field<&Noii::stock_locate>,
    Field<&Noii::tracking_number>,
    Field<&Noii::timestamp, be48>,

    Field<&Noii::paired_shares>,
    Field<&Noii::imbalance_shares>,
    Field<&Noii::imbalance_direction>,

    Field<&Noii::stock>,
    Field<&Noii::far_price>,
    Field<&Noii::near_price>,
    Field<&Noii::current_reference_price>,
    Field<&Noii::cross_type>,
    Field<&Noii::price_variation_indicator>
>;

struct DirectListingCapitalRaise {
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint64_t timestamp;

    char     stock[8];
    char     open_eligibility_status;
    uint32_t minimum_allowable_price;
    uint32_t maximum_allowable_price;
    uint32_t near_execution_price;
    uint64_t near_execution_time;
    uint32_t lower_price_range_collar;
    uint32_t upper_price_range_collar;
};

using DirectListingCapitalRaiseLayout = std::tuple<
    Field<&DirectListingCapitalRaise::stock_locate>,
    Field<&DirectListingCapitalRaise::tracking_number>,
    Field<&DirectListingCapitalRaise::timestamp, be48>,

    Field<&DirectListingCapitalRaise::stock>,
    Field<&DirectListingCapitalRaise::open_eligibility_status>,
    Field<&DirectListingCapitalRaise::minimum_allowable_price>,
    Field<&DirectListingCapitalRaise::maximum_allowable_price>,
    Field<&DirectListingCapitalRaise::near_execution_price>,
    Field<&DirectListingCapitalRaise::near_execution_time>,
    Field<&DirectListingCapitalRaise::lower_price_range_collar>,
    Field<&DirectListingCapitalRaise::upper_price_range_collar>
>;

struct Message {
    MessageType type;
    size_t      size;
    union {
        SystemEvent system_event;
        StockDirectory stock_directory;
        TradingAction trading_action;
        RegSho reg_sho;
        MarketParticipantPos market_participant_pos;
        MwcbDeclineLevel mwcb_decline_level;
        MwcbStatus mwcb_status;
        IpoQuotationPeriodUpd ipo_quotation_period_upd;
        LuldAuctionCollar luld_auction_collar;
        OperationalHalt operational_halt;
        AddOrderNoMpid add_order_no_mpid;
        AddOrderMpid add_order_mpid;
        OrderExecuted order_executed;
        OrderExecutedPrice order_executed_price;
        OrderCancel order_cancel;
        OrderDelete order_delete;
        OrderReplace order_replace;
        TradeMessageNonCross trade_msg_non_cross;
        TradeMessageCross trade_msg_cross;
        BrokenTrade broken_trade;
        Noii noii;
        DirectListingCapitalRaise direct_listing_capital_raise;
    };
};

class ItchParser {
public:
    template <typename Handler>
    void parse(std::byte const *  src, size_t len, Handler& handler);

    template <typename SpecificHandler>
    void parse_specific(std::byte const * src, size_t len, SpecificHandler& handler);

    Message parse_msg(std::byte const * src);
};

inline uint16_t load_be16(const std::byte* p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

[[gnu::noinline]] static void bad_type(char t) {
    throw std::runtime_error("Unknown message type: " + std::to_string(t));
}

inline Message ItchParser::parse_msg(std::byte const * src) {
    uint16_t size = load_be16(src);
    src += 2;

    auto raw_type = char(src[0]);
    MessageType type = static_cast<MessageType>(raw_type);
    src += 1;

    Message msg{};
    msg.type = type;
    msg.size = size;

    switch (type) {
    #define X(NAME, TYPE, FIELD, LIKELINESS) \
        case MessageType::NAME: \
        [[LIKELINESS]] \
        { \
            msg.FIELD = parse_itch<TYPE##Layout, TYPE>(src); \
            asm volatile("" : : "r,m"(msg.FIELD)); \
            return msg; \
        } \

        ITCH_MESSAGE_LIST(X)

        default:
            bad_type(raw_type);
    #undef X
    }

    return msg;
}

template<typename SpecificHandler>
void ItchParser::parse_specific(std::byte const * src, size_t len, SpecificHandler& handler) {
    std::byte const * end = src + len;

    while (end - src >= 3) {
        uint16_t size = load_be16(src);
        if (end - src < 2 + size) {
            break;
        }
        src += 2;

        auto raw_type = char(src[0]);
        MessageType type = static_cast<MessageType>(raw_type);
        src += 1;

        switch (type) {
        #define X(NAME, TYPE, FIELD, LIKELINESS) \
            case MessageType::NAME: \
            [[LIKELINESS]] \
            { \
                handler.handle_before(); \
                auto m = parse_itch<TYPE##Layout, TYPE>(src); \
                handler.handle_##FIELD(m); \
                handler.handle_after(); \
                break; \
            } \

            ITCH_MESSAGE_LIST(X)

            default:
                bad_type(raw_type);
        #undef X
        }

        src += size - 1;
    }
}

template <typename Handler>
void ItchParser::parse(std::byte const *  src, size_t len, Handler& handler) {
    std::byte const * end = src + len;

    while (end - src >= 3) {
        uint16_t size = load_be16(src);
        if (end - src < 2 + size) {
            break;
        }

        Message msg = parse_msg(src);
        handler.handle(msg);

        src += msg.size + 2;
    }
}
}
