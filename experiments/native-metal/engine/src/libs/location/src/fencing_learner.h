#pragma once

// A small online transition model. Only finished, observed attacks are samples;
// the animation currently winding up never enters its own prediction.
class FencingLearner
{
  public:
    enum Attack { None = -1, Fast, Force, Round, Break, Feint, Count };

    void Observe(Attack action)
    {
        if (action < None || action >= Count || action == pending)
            return;
        if (pending != None)
            Train(pending);
        pending = action;
    }

    float PredictedBreak() const
    {
        float total = 0.0f;
        for (float weight : frequency)
            total += weight;
        const float overall = (frequency[Break] + 0.5f) / (total + 2.5f);
        if (previous == None)
            return overall;
        float rowTotal = 0.0f;
        for (float weight : transition[previous])
            rowTotal += weight;
        const float conditional = (transition[previous][Break] + 0.5f) / (rowTotal + 2.5f);
        const float trust = rowTotal / (rowTotal + 4.0f);
        return overall + trust * (conditional - overall);
    }

    float ParryBias(float skill) const
    {
        if (seen <= 4 || !(skill > 0.0f))
            return 0.0f;
        if (skill > 1.0f)
            skill = 1.0f;
        const float confidence = float(seen - 4) / 8.0f;
        const float bias = (PredictedBreak() - 0.2f) * 0.4f * confidence * skill;
        if (bias > 0.25f)
            return 0.25f;
        if (bias < -0.08f)
            return -0.08f;
        return bias;
    }

  private:
    void Train(Attack action)
    {
        // Forget old habits instead of letting the first tactic win forever.
        for (int i = 0; i < Count; ++i)
        {
            frequency[i] *= 0.92f;
            for (float &weight : transition[i])
                weight *= 0.92f;
        }
        frequency[action] += 1.0f;
        if (previous != None)
            transition[previous][action] += 1.0f;
        previous = action;
        if (seen < 12)
            ++seen;
    }

    float frequency[Count]{};
    float transition[Count][Count]{};
    Attack previous = None;
    Attack pending = None;
    int seen = 0;
};
