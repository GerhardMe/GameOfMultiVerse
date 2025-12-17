#ifndef RULESET_ID_H
#define RULESET_ID_H

#include <vector>

struct Ruleset
{
    int underpopEnd; // [0, underpopEnd] -> death
    int birthStart;  // [birthStart, birthEnd] -> birth/survival
    int birthEnd;
    int overpopStart; // [overpopStart, 8] -> death

    bool operator==(const Ruleset &other) const;
};

// Initialize the ruleset system (call once at startup)
void initRulesets();

// Convert ruleset to ID (its index in canonical ordering)
int rulesetToId(const Ruleset &ruleset);

// Convert ID to ruleset
Ruleset idToRuleset(int id);

// Get total number of valid rulesets (fast, pre-computed)
int getTotalRulesets();

#endif // RULESET_ID_H
