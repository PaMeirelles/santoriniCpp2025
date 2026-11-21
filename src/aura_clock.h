#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace Santorini::AuraClock {
// E[ceil((L-i)/2) | L>i], including the current side-to-move search.
// Frozen original pooled distribution: 56,800 legally completed games.
inline constexpr std::array<double, 93> own_turns_left = {
    15.516742957746478,
    14.946848591549296,
    14.516742957746478,
    13.946848591549296,
    13.516742957746478,
    12.946848591549296,
    12.516742957746478,
    11.948110715920697,
    11.519176586602804,
    11.049276753029819,
    10.645525180881776,
    10.370866878174915,
    10.01219038457981,
    9.7711827134525286,
    9.4444121353106532,
    9.2767519466073409,
    9.0010331645831183,
    8.8223855821674171,
    8.5924462985975509,
    8.4379461898007122,
    8.2226868541216049,
    8.0781411794418485,
    7.915924774798504,
    7.8081798679408241,
    7.6593448265803215,
    7.5730135118383535,
    7.4407287898047656,
    7.3466574298579843,
    7.2304427369191364,
    7.1660213539833393,
    7.0543541304167698,
    6.9909098971266408,
    6.9273795980280619,
    6.85996445798426,
    6.8173590666230508,
    6.7299550994227069,
    6.6927178071881075,
    6.6335651117261865,
    6.5886751361161524,
    6.5574414584315575,
    6.5130287648054148,
    6.508742867660593,
    6.4384942199387414,
    6.399187513363267,
    6.3072075169015696,
    6.2761277494718533,
    6.2361653426686416,
    6.172870020354754,
    6.048996733551097,
    5.9907391816804179,
    5.8943163246776829,
    5.8049353701527613,
    5.6850875342754694,
    5.5809827115559596,
    5.4308068459657699,
    5.2758711029604397,
    5.1366213151927438,
    5.0064417177914109,
    4.8260582010582009,
    4.6282778171509564,
    4.4244741873804969,
    4.1928746928746925,
    4.0156950672645744,
    3.85990099009901,
    3.7072767364939359,
    3.5660493827160495,
    3.3752577319587629,
    3.2325038880248833,
    3.1219512195121952,
    3.006282722513089,
    2.9107806691449816,
    2.855439642324888,
    2.8608534322820036,
    2.8040540540540539,
    2.696236559139785,
    2.6176470588235294,
    2.4940711462450591,
    2.3798076923076925,
    2.290909090909091,
    2.0948905109489053,
    2.195876288659794,
    2.112676056338028,
    2.0714285714285716,
    1.9750000000000001,
    1.8181818181818181,
    1.95,
    1.6875,
    1.2666666666666666,
    1.375,
    1.3333333333333333,
    1.5,
    1,
    1
};
inline double expected_own_turns(int elapsed_plies) {
    if (elapsed_plies < 0) throw std::invalid_argument("negative elapsed ply");
    // Treat the final four plies as requiring no extra thinking: two own turns.
    // Keep the original L>elapsed conditioning; clamp the mean divisor afterward.
    const double original = elapsed_plies < static_cast<int>(own_turns_left.size())
        ? own_turns_left[elapsed_plies] : 10.0;
    return std::max(2.0, original - 2.0);
}
inline int budget_ms(int remaining_ms, int elapsed_plies) {
    const double turns = std::max(2.0, expected_own_turns(elapsed_plies));
    if (remaining_ms <= 0) return 0;
    // Leave up to 50ms for interruption granularity and protocol overhead.
    // On small clocks reserve 10%; never force a 50ms minimum search.
    const int reserve = std::min(50, remaining_ms / 10);
    return std::max(0, static_cast<int>(std::floor((remaining_ms - reserve) / turns)));
}
}
