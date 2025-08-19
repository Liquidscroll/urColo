// urColo - tests model training and colour suggestions via JSON round trip
#include "urColo/Model.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

TEST_CASE("model training and suggestion") {
    uc::Palette p{"p"};
    p.addSwatch("r", {1.0f, 0.0f, 0.0f, 1.0f});
    p.addSwatch("g", {0.0f, 1.0f, 0.0f, 1.0f});

    uc::Model m(123);
    m.train({p});
    nlohmann::json j = m;
    CHECK(j["trained"].get<bool>() == true);

    uc::Model copy = j.get<uc::Model>();
    auto samples = copy.suggest(5);
    CHECK(samples.size() == 5);

    // Compute average L from samples
    double sumL = 0.0;
    for (const auto &sw : samples) {
        uc::Colour c = uc::Colour::fromImVec4(sw._colour);
        sumL += c.lab.L;
    }
    double avg = sumL / static_cast<double>(samples.size());
    double trainedL = j["mean"][0].get<double>();
    CHECK(avg == doctest::Approx(trainedL).epsilon(0.5));
}
