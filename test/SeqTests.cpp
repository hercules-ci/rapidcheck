#include <catch.hpp>
#include <rapidcheck-catch.h>

#include "rapidcheck/Seq.h"

using namespace rc;

namespace {

template<typename Next, typename HasNext>
class SeqImplMock
{
public:
    SeqImplMock(Next next, HasNext hasNext)
        : m_next(next)
        , m_hasNext(hasNext) {}

    bool hasNext() const { return m_hasNext(); }
    typename std::result_of<Next()>::type next() { return m_next(); }

private:
    Next m_next;
    HasNext m_hasNext;
};

template<typename Next, typename HasNext>
Seq<typename std::result_of<Next()>::type> mockSeq(Next next, HasNext hasNext)
{ return SeqImplMock<Next, HasNext>(std::move(next), std::move(hasNext)); }

class LoggingSeqImpl
{
public:
    LoggingSeqImpl(std::string id)
        : m_id(std::move(id))
        , m_log{"ID constructed"} {}

    LoggingSeqImpl(const LoggingSeqImpl &other)
        : m_id(other.m_id)
        , m_log(other.m_log)
    { m_log.emplace_back("copy constructed"); }

    LoggingSeqImpl(LoggingSeqImpl &&other)
        : m_id(std::move(other.m_id))
        , m_log(std::move(other.m_log))
    { m_log.emplace_back("move constructed"); }

    bool hasNext() const { return true; }

    std::pair<std::string, std::vector<std::string>> next()
    { return { m_id, m_log }; }

private:
    std::string m_id;
    std::vector<std::string> m_log;
};

typedef Seq<std::pair<std::string, std::vector<std::string>>> LoggingSeq;

}

TEST_CASE("Seq") {
    SECTION("default constructed Seq is empty") {
        REQUIRE_FALSE(Seq<int>());
    }

    SECTION("calls the next() method of the implementation object") {
        bool nextCalled = false;
        auto seq = mockSeq([&]{
            nextCalled = true;
            return 0;
        }, []{
            return true;
        });

        seq.next();
        REQUIRE(nextCalled);
    }

    SECTION("calls the hasNext() method of the implementation object") {
        bool hasNextCalled = false;
        auto seq = mockSeq([]{
            return 0;
        }, [&]{
            hasNextCalled = true;
            return true;
        });

        REQUIRE(seq);
        REQUIRE(hasNextCalled);
    }

    SECTION("discards the implementation object if hasNext() returns false") {
        bool nextCalled = false;
        int hasNextCount = 0;
        auto seq = mockSeq([&]{
            nextCalled = true;
            return 0;
        }, [&]{
            hasNextCount++;
            return false;
        });

        REQUIRE_FALSE(seq);
        REQUIRE_FALSE(nextCalled);
        REQUIRE(hasNextCount == 1);
    }

    SECTION("copies implementation if constructed from lvalue") {
        LoggingSeqImpl impl("foobar");
        LoggingSeq seq(impl);

        const auto value = seq.next();
        std::vector<std::string> expectedLog{
            "ID constructed",
            "copy constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }

    SECTION("moves implementation if constructed from rvalue") {
        LoggingSeq seq(LoggingSeqImpl("foobar"));
        const auto value = seq.next();

        std::vector<std::string> expectedLog{
            "ID constructed",
            "move constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }

    SECTION("copy construction copies the implementation object") {
        LoggingSeq original(LoggingSeqImpl("foobar"));
        auto copy(original);

        const auto value = copy.next();
        std::vector<std::string> expectedLog{
            "ID constructed",
            "move constructed",
            "copy constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }

    SECTION("copy assignment copies the implementation object") {
        LoggingSeq original(LoggingSeqImpl("foobar"));
        LoggingSeq copy;
        copy = original;

        const auto value = copy.next();
        std::vector<std::string> expectedLog{
            "ID constructed",
            "move constructed",
            "copy constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }

    SECTION("move construction neither moves nor copies") {
        LoggingSeq original(LoggingSeqImpl("foobar"));
        LoggingSeq moved(std::move(original));

        const auto value = moved.next();
        std::vector<std::string> expectedLog{
            "ID constructed",
            "move constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }

    SECTION("move assignment neither moves nor copies") {
        LoggingSeq original(LoggingSeqImpl("foobar"));
        LoggingSeq moved;
        moved = std::move(original);

        const auto value = moved.next();
        std::vector<std::string> expectedLog{
            "ID constructed",
                "move constructed"};
        REQUIRE(value.first == "foobar");
        REQUIRE(value.second == expectedLog);
    }
}
