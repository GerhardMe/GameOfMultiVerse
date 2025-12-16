#include "ruleset_id.h"
#include <stdexcept>

bool Ruleset::operator==(const Ruleset &other) const
{
    return underpopEnd == other.underpopEnd &&
           birthStart == other.birthStart &&
           birthEnd == other.birthEnd &&
           overpopStart == other.overpopStart;
}

std::vector<Ruleset> generateAllRulesets()
{
    std::vector<Ruleset> rulesets;

    for (int a = 0; a <= 7; a++)
    {
        for (int b = a + 1; b <= 8; b++)
        {
            for (int c = b; c <= 8; c++)
            {
                for (int d = c + 1; d <= 9; d++)
                {
                    rulesets.push_back({a, b, c, d});
                }
            }
        }
    }

    return rulesets;
}

int rulesetToId(const Ruleset &ruleset)
{
    static std::vector<Ruleset> allRulesets = generateAllRulesets();

    for (size_t i = 0; i < allRulesets.size(); i++)
    {
        if (allRulesets[i] == ruleset)
        {
            return i;
        }
    }

    throw std::runtime_error("Invalid ruleset");
}

Ruleset idToRuleset(int id)
{
    static std::vector<Ruleset> allRulesets = generateAllRulesets();

    if (id < 0 || id >= (int)allRulesets.size())
    {
        throw std::runtime_error("Invalid ruleset ID");
    }

    return allRulesets[id];
}

int getTotalRulesets()
{
    static std::vector<Ruleset> allRulesets = generateAllRulesets();
    return allRulesets.size();
}