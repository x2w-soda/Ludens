#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <Extra/doctest/doctest.h>
#include <Ludens/RenderGraph/RGraph.h>

using namespace LD;

TEST_CASE("RGraph empty")
{
    RGraphInfo graphI{};
    RGraph graph = RGraph::create(graphI);

    Vector<RComponentPass> order;
    graph.debug(order, false);

    CHECK(order.empty());

    RGraph::destroy(graph);
}

TEST_CASE("RGraph basic")
{
    RGraphInfo graphI{};
    RGraph graph = RGraph::create(graphI);

    RComponent c1 = graph.add_component("c1");
    RGraphImage c1in = c1.add_input_image("in", RFORMAT_RGBA8, 512, 512);
    RGraphicsPassInfo gpI{};
    gpI.width = 512;
    gpI.height = 512;
    gpI.name = "gp1";
    gpI.samples = RSAMPLE_COUNT_1_BIT;
    RGraphicsPass c1gp1 = c1.add_graphics_pass(gpI, nullptr, nullptr);
    c1gp1.use_color_attachment(c1in, RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);

    RComponent c2 = graph.add_component("c2");
    RGraphImage c2out = c2.add_output_image("out", RFORMAT_RGBA8, 512, 512, nullptr);
    RGraphicsPass c2gp1 = c2.add_graphics_pass(gpI, nullptr, nullptr);
    c2gp1.use_color_attachment(c2out, RATTACHMENT_LOAD_OP_DONT_CARE, nullptr);

    // establish dependency
    graph.connect_image(c2out, c1in);

    Vector<RComponentPass> order;
    graph.debug(order, false);

    CHECK(order.size() == 2);
    CHECK(order[0] == c2gp1);
    CHECK(order[1] == c1gp1);

    RGraph::destroy(graph);
}