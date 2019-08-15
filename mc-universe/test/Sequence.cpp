#define CATCH_CONFIG_MAIN
#include "Sequence.hpp"


TEST_CASE("Sequence processes inits", "[grid][sequence]" ) {
    Grid* g = new Grid("lala", 8, 8);
    vector<Initialiser*> inits;
    uint16_t px[8][8] = {{0}};

    // set a few contrived values
    px[0][0] = 65;
    px[1][2] = 129;

    g->accumulate(*px, inits);
    REQUIRE(*(g->px+0)+0 == 65);
    REQUIRE(*(g->px+(1*8)+2) == 129);
    REQUIRE(inits.size() == 0);

    g->accumulate(*px, inits);
    REQUIRE(*(g->px+0)+0 == 130);
    REQUIRE(*(g->px+(1*8)+2) == 0);
    REQUIRE(inits.size() == 1);
    REQUIRE(inits[0]->value == 258);
}
