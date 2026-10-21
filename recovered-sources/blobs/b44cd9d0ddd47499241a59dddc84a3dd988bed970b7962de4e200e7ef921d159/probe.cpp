
#include <memory>
#include <cassert>
#include <iostream>
#include "attributes.h"
#include "string_compare.hpp"
#include "string_codec.h"
void STRING_CODEC::VariableChanged() {}
int main() {
    STRING_CODEC codec;
    {
        ATTRIBUTES root(codec);
        auto *known=root.CreateAttribute("ShipName", "Manowar");
        const std::string longName(2048, 'x');
        auto *longChild=root.CreateAttribute(longName, "long value");
        assert(root.GetAttributeClass(longName)==longChild);
        assert(std::string_view(root.GetAttribute(longName))=="long value");
        const auto count=codec.GetNum();
        for (const auto size : {size_t(0),size_t(1),size_t(1023),size_t(1024),size_t(2048)}) {
            std::string miss(size, 'z');
            assert(!root.GetAttributeClass(miss));
            assert(root.GetAttribute(miss)==nullptr);
        }
        assert(codec.GetNum()==count);
        assert(root.GetAttributeClass("sHIPnAME")==known);
        assert(std::string_view(root.GetAttribute("sHIPnAME"))=="Manowar");
        const std::string bounded="ShipName_suffix";
        const std::string_view view(bounded.data(),8);
        assert(root.GetAttributeClass(view)==known);
        assert(std::string_view(root.GetAttribute(view))=="Manowar");
        assert(!root.GetAttributeClass(std::string_view("ShipName\0junk",13)));
        assert(root.GetAttribute(std::string_view("ShipName\0junk",13))==nullptr);
        auto *empty=root.CreateAttribute("", "empty value");
        assert(root.GetAttributeClass(std::string_view{})==empty);
        assert(std::string_view(root.GetAttribute(std::string_view{}))=="empty value");
        assert(codec.GetNum()==count+1);
    }
    // STRING_CODEC's old scalar delete is unrelated to the lookup delta.
    std::cout<<"PASS actual attributes lookups: long hit/miss, case, bounded/embedded-NUL/empty views, noninterning misses\n";
}
