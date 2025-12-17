#include "ruleset_id.h"
#include <stdexcept>

// Global cached data - initialized once at startup
static std::vector<Ruleset> g_allRulesets;
static int g_totalRulesets = 0;
static bool g_initialized = false;

bool Ruleset::operator==(const Ruleset &other) const
{
    return underpopEnd == other.underpopEnd &&
           birthStart == other.birthStart &&
           birthEnd == other.birthEnd &&
           overpopStart == other.overpopStart;
}

void initRulesets()
{
    if (g_initialized)
        return;

    // Generate all valid rulesets once
    for (int a = 0; a <= 7; a++)
    {
        for (int b = a + 1; b <= 8; b++)
        {
            for (int c = b; c <= 8; c++)
            {
                for (int d = c + 1; d <= 9; d++)
                {
                    g_allRulesets.push_back({a, b, c, d});
                }
            }
        }
    }

    g_totalRulesets = static_cast<int>(g_allRulesets.size());
    g_initialized = true;
}

int rulesetToId(const Ruleset &ruleset)
{
    if (!g_initialized)
    {
        throw std::runtime_error("Rulesets not initialized - call initRulesets() first");
    }

    for (int i = 0; i < g_totalRulesets; i++)
    {
        if (g_allRulesets[i] == ruleset)
        {
            return i;
        }
    }

    throw std::runtime_error("Invalid ruleset");
}

Ruleset idToRuleset(int id)
{
    if (!g_initialized)
    {
        throw std::runtime_error("Rulesets not initialized - call initRulesets() first");
    }

    if (id < 0 || id >= g_totalRulesets)
    {
        throw std::runtime_error("Invalid ruleset ID");
    }

    return g_allRulesets[id];
}

int getTotalRulesets()
{
    if (!g_initialized)
    {
        throw std::runtime_error("Rulesets not initialized - call initRulesets() first");
    }

    return g_totalRulesets;
}