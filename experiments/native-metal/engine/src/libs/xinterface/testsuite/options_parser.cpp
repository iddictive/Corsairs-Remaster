#include "options_parser.hpp"

#include <catch2/catch.hpp>

namespace
{

// TODO: Either move STRING_CODEC outside of Engine or make a re-usable codec for testing
class TestStringCodec : public VSTRING_CODEC
{
  public:
    uint32_t GetNum() override
    {
        return map_.size();
    }

    uint32_t Convert(const char *pString) override
    {
        std::string str(pString);
        const uint32_t hash = std::hash<std::string>{}(str);
        map_.emplace(hash, str);
        return hash;
    }

    uint32_t Convert(const char *pString, int32_t iLen) override
    {
        std::string str(pString, iLen);
        const uint32_t hash = std::hash<std::string>{}(str);
        map_.emplace(hash, str);
        return hash;
    }

    const char *Convert(uint32_t code) override
    {
        return map_[code].c_str();
    }

    void VariableChanged() override
    {
    }

  private:
    std::unordered_map<uint32_t, std::string> map_;
};

} // namespace

TEST_CASE("Remove carriage return characters from string", "[xinterface]")
{
    using namespace std::string_literals;

    auto value = "Hello,\r\nWorld!\r\n"s;

    storm::removeCarriageReturn(value);

    CHECK(value == "Hello,\nWorld!\n");
}

TEST_CASE("Serialize options", "[xinterface]")
{
    TestStringCodec string_codec{};
    ATTRIBUTES attribute(string_codec);

    SECTION("Nested attributes preserve their order and format")
    {
        attribute.SetAttribute("title", std::string_view{"My title"});
        auto *const text = attribute.CreateAttribute("text", "");
        text->SetAttribute("t1", std::string_view{"First"});
        text->SetAttribute("t2", std::string_view{"Second"});

        CHECK(storm::serializeOptions(attribute) == "title=My title\ntext.t1=First\ntext.t2=Second");
    }

    SECTION("Empty values are omitted while their children are preserved")
    {
        auto *const controls = attribute.CreateAttribute("controls", "");
        controls->SetAttribute("forward", std::string_view{"key_w"});

        CHECK(storm::serializeOptions(attribute) == "controls.forward=key_w");
    }

    SECTION("An empty tree produces no payload")
    {
        CHECK(storm::serializeOptions(attribute).empty());
    }

    SECTION("A tree containing only empty values produces no payload")
    {
        attribute.CreateAttribute("empty", "");

        CHECK(storm::serializeOptions(attribute).empty());
    }
}

TEST_CASE("Parse options", "[xinterface]")
{
    using namespace std::string_literals;
    using namespace std::string_view_literals;

    TestStringCodec string_codec{};

    SECTION("With final newline")
    {
        auto source = "title=My title\ntext=Hello, World!\n"s;

        ATTRIBUTES attribute(&string_codec);

        storm::parseOptions(source, attribute);

        CHECK(attribute.GetAttributesNum() == 2);

        const char *title_attribute = attribute.GetAttribute("title");
        REQUIRE(title_attribute != nullptr);
        CHECK(title_attribute == "My title"sv);

        const char *text_attribute = attribute.GetAttribute("text");
        REQUIRE(text_attribute != nullptr);
        CHECK(text_attribute == "Hello, World!"sv);
    }

    SECTION("Without final newline")
    {
        auto source = "title=My title\ntext=Hello, World!"s;

        ATTRIBUTES attribute(&string_codec);

        storm::parseOptions(source, attribute);

        CHECK(attribute.GetAttributesNum() == 2);

        const char *title_attribute = attribute.GetAttribute("title");
        REQUIRE(title_attribute != nullptr);
        CHECK(title_attribute == "My title"sv);

        const char *text_attribute = attribute.GetAttribute("text");
        REQUIRE(text_attribute != nullptr);
        CHECK(text_attribute == "Hello, World!"sv);
    }

    SECTION("With int32_t text values")
    {
        auto source =
            "title=Inform the governor about the French attack.\n"
            "text.t1=Malcolm was right about spotting French near #sisland_Oxbay#. A war fleet is upon #sOxbay# port! "
            "I "
            "had better get away before I am detected and destroyed with the rest of the English here.\n"
            "text.t2=A French squadron captured the English colony, #sOxbay# on #sisland_Oxbay#. The French succeeded because the fort's defences were so weak. I escaped only by great good fortune. I suppose I should inform the governor of the main English colony - #sRedmond# on the island #sisland_Redmond#, Sir Robert Christopher Silehard, about the attack, so he can take appropriate measures. By chance, I noticed someone else also managed to escape the French. I saw a ship sailing away from the bay. I wonder who the captain was.\n"s;

        ATTRIBUTES attribute(&string_codec);

        storm::parseOptions(source, attribute);

        CHECK(attribute.GetAttributesNum() == 2);
        const char *title_attribute = attribute.GetAttribute("title");
        REQUIRE(title_attribute != nullptr);
        CHECK(title_attribute == "Inform the governor about the French attack."sv);

        const auto *text_attribute = attribute.GetAttributeClass("text");
        REQUIRE(text_attribute != nullptr);
        CHECK(text_attribute->GetAttributesNum() == 2);

       const char *t1 = text_attribute->GetAttribute("t1");
        REQUIRE(t1 != nullptr);
        CHECK(
            t1 ==
            "Malcolm was right about spotting French near #sisland_Oxbay#. A war fleet is upon #sOxbay# port! I had better get away before I am detected and destroyed with the rest of the English here."sv);

        const char *t2 = text_attribute->GetAttribute("t2");
        REQUIRE(t2 != nullptr);
        CHECK(
            t2 ==
            "A French squadron captured the English colony, #sOxbay# on #sisland_Oxbay#. The French succeeded because the fort's defences were so weak. I escaped only by great good fortune. I suppose I should inform the governor of the main English colony - #sRedmond# on the island #sisland_Redmond#, Sir Robert Christopher Silehard, about the attack, so he can take appropriate measures. By chance, I noticed someone else also managed to escape the French. I saw a ship sailing away from the bay. I wonder who the captain was."sv);
    }
}
