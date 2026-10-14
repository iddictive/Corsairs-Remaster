#include "../src/modelr.h"

class NoderLightPathTestAccessor
{
  public:
    static bool Set(NODER &node, const std::string_view lightPath)
    {
        return node.SetLightPath(lightPath);
    }

    static const std::string &Get(const NODER &node)
    {
        return node.sys_LightPath;
    }
};

int main()
{
    NODER root;
    NODER child;
    root.next.push_back(&child);

    if (!NoderLightPathTestAccessor::Set(root, "morning8"))
        return 1;
    if (NoderLightPathTestAccessor::Get(root) != "morning8" || NoderLightPathTestAccessor::Get(child) != "morning8")
        return 2;

    if (NoderLightPathTestAccessor::Set(root, "morning8"))
        return 3;

    if (!NoderLightPathTestAccessor::Set(root, "day11"))
        return 4;
    if (NoderLightPathTestAccessor::Get(root) != "day11" || NoderLightPathTestAccessor::Get(child) != "day11")
        return 5;
    return 0;
}
